// SPDX-License-Identifier: MIT
#include "QtOpenAi/Sql/SqliteStore.h"

#include "JsonHelpers_p.h"

#include <QtCore/QAtomicInteger>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

namespace QtOpenAi {
namespace Sql {

using Storage::CachedResponse;
using Storage::ConversationRecord;

// The serialiser every module that stores or sends JSON as text shares; see
// JsonHelpers_p.h. Pulled in by name so the call sites below read as they did.
using Core::detail::compactJsonText;

namespace {

constexpr auto DriverName = "QSQLITE";

// Timestamps are milliseconds since the epoch, UTC. Storing the ISO string
// would be readable in a database browser and would sort correctly, but the
// cache prune compares and orders on this column, and an integer comparison is
// what makes that one index seek.
qint64 toStamp(const QDateTime &when)
{
    return when.isValid() ? when.toUTC().toMSecsSinceEpoch() : 0;
}

QDateTime fromStamp(qint64 stamp)
{
    // toUTC() rather than the time-spec overload of fromMSecsSinceEpoch():
    // that one takes Qt::TimeSpec, which is deprecated in Qt 6.9, and its
    // QTimeZone replacement only arrived in 6.5 -- and this library supports
    // 6.4. The instant is the same either way.
    return stamp > 0 ? QDateTime::fromMSecsSinceEpoch(stamp).toUTC() : QDateTime();
}

QJsonObject parse(const QString &text) { return QJsonDocument::fromJson(text.toUtf8()).object(); }

// What both conversation readers select, so that they select the same five
// columns in the same order: that is what lets them share the row decoding
// below rather than each spell it out, and what keeps a column added here from
// being missed by one of them.
QString conversationSelect()
{
    return QStringLiteral("SELECT id, title, created_at, updated_at, message_count "
                          "FROM conversations");
}

ConversationRecord recordFromRow(const QSqlQuery &query)
{
    ConversationRecord record;
    record.id = query.value(0).toString();
    record.title = query.value(1).toString();
    record.createdAt = fromStamp(query.value(2).toLongLong());
    record.updatedAt = fromStamp(query.value(3).toLongLong());
    record.messageCount = query.value(4).toInt();
    return record;
}

} // namespace

class SqliteStorePrivate
{
public:
    QString filePath;
    QString connectionName;
    bool open = false;
    int schemaVersion = 0;

    // How many beginBatch() calls are outstanding, and whether any of them
    // asked for the batch to be dropped. SQLite has one transaction per
    // connection, so nesting is counted rather than nested: the outermost
    // endBatch() is the one that commits, and an inner endBatch(false) means
    // the whole thing rolls back -- an inner caller that could not finish must
    // not have its half kept by an outer one that does not know.
    int batchDepth = 0;
    bool batchAborted = false;

    QSqlDatabase database() const { return QSqlDatabase::database(connectionName, false); }

    QSqlQuery query() const { return QSqlQuery(database()); }

    // Statements are prepared per call rather than cached. The store is not on
    // a hot path -- it is written once per autosave interval and read once per
    // conversation opened -- and a cache of prepared statements would have to
    // be invalidated on close and guarded per thread for no measurable gain.
    bool exec(QSqlQuery &query, const QString &statement, const QVariantList &values = {})
    {
        if (!query.prepare(statement))
            return false;
        for (const QVariant &value : values)
            query.addBindValue(value);
        return query.exec();
    }

    // Creates the tables and settles the schema version, writing the version
    // found to `version`. Returns an empty string on success, otherwise why it
    // failed.
    //
    // Separate from open() so that the QSqlDatabase and its queries are all
    // destroyed before open() has to call removeDatabase() on the failure
    // path: Qt warns that the connection "is still in use" if any copy of it
    // is alive at that point, and then leaves the connection registered -- so
    // a second attempt would find the name taken.
    QString prepareSchema(int *version);
};

QString SqliteStorePrivate::prepareSchema(int *version)
{
    QSqlDatabase database = QSqlDatabase::database(connectionName, false);
    QSqlQuery query(database);

    const QStringList schema
            = {QStringLiteral("CREATE TABLE IF NOT EXISTS meta ("
                              "key TEXT PRIMARY KEY, value TEXT NOT NULL)"),
               QStringLiteral("CREATE TABLE IF NOT EXISTS conversations ("
                              "id TEXT PRIMARY KEY, title TEXT NOT NULL DEFAULT '', "
                              "created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL, "
                              "message_count INTEGER NOT NULL, transcript TEXT NOT NULL)"),
               // The listing order, so showing a conversation list is a scan of
               // this index rather than a sort of every row.
               QStringLiteral("CREATE INDEX IF NOT EXISTS conversations_updated_at "
                              "ON conversations (updated_at DESC)"),
               QStringLiteral("CREATE TABLE IF NOT EXISTS cache ("
                              "key BLOB PRIMARY KEY, body BLOB NOT NULL, "
                              "stored_at INTEGER NOT NULL)"),
               // Both halves of a prune -- everything older than a cutoff, and
               // everything past a count -- run over this one.
               QStringLiteral("CREATE INDEX IF NOT EXISTS cache_stored_at ON cache (stored_at)"),
               QStringLiteral("CREATE TABLE IF NOT EXISTS metrics ("
                              "id TEXT PRIMARY KEY, snapshot TEXT NOT NULL)")};
    for (const QString &statement : schema) {
        if (!query.exec(statement)) {
            return QStringLiteral("SqliteStore: cannot create the schema: %1")
                    .arg(query.lastError().text());
        }
    }

    int found = 0;
    if (exec(query, QStringLiteral("SELECT value FROM meta WHERE key = 'schema_version'"))
        && query.next()) {
        found = query.value(0).toInt();
    }

    if (found == 0) {
        // A database this build just created; stamping it is what makes it a
        // store rather than a file with tables in it.
        if (!exec(query,
                  QStringLiteral("INSERT OR REPLACE INTO meta (key, value) "
                                 "VALUES ('schema_version', ?)"),
                  {QString::number(SqliteStore::CurrentSchemaVersion)})) {
            return QStringLiteral("SqliteStore: cannot record the schema version: %1")
                    .arg(query.lastError().text());
        }
        found = SqliteStore::CurrentSchemaVersion;
    } else if (found > SqliteStore::CurrentSchemaVersion) {
        // Refused rather than read on a guess: the file belongs to a newer
        // library that knows what it put there, and writing to it with this
        // one's assumptions is how the newer version's data gets lost.
        return QStringLiteral("SqliteStore: %1 has schema version %2, newer than the %3 this "
                              "build writes.")
                .arg(filePath)
                .arg(found)
                .arg(SqliteStore::CurrentSchemaVersion);
    } else if (found < SqliteStore::CurrentSchemaVersion) {
        // Migration room. Version 1 is the first, so there is no step to run
        // yet; the branch is here so the next version has one place to add one,
        // and so an older database is never read as if it were current.
        return QStringLiteral("SqliteStore: no migration from schema version %1.").arg(found);
    }

    *version = found;
    return {};
}

SqliteStore::SqliteStore(const QString &filePath)
    : d(new SqliteStorePrivate)
{
    d->filePath = filePath;
    // Unique per store: two stores sharing a connection name would share a
    // connection, and closing one would close the other's database.
    static QAtomicInteger<quint64> counter;
    d->connectionName = QStringLiteral("qtopenai-sqlite-%1").arg(counter.fetchAndAddRelaxed(1));
}

SqliteStore::~SqliteStore() { close(); }

QString SqliteStore::filePath() const { return d->filePath; }

bool SqliteStore::isAvailable()
{
    return QSqlDatabase::isDriverAvailable(QLatin1String(DriverName));
}

bool SqliteStore::open()
{
    clearError();
    if (d->open)
        return true;
    if (!isAvailable()) {
        return fail(QStringLiteral("SqliteStore: this Qt build has no %1 driver.")
                            .arg(QLatin1String(DriverName)));
    }
    if (d->filePath.isEmpty())
        return fail(QStringLiteral("SqliteStore: no file path given."));

    // ":memory:" is a database, not a file, so it has no directory to create.
    if (d->filePath != QLatin1String(":memory:")) {
        const QDir directory = QFileInfo(d->filePath).absoluteDir();
        if (!directory.exists() && !QDir().mkpath(directory.absolutePath())) {
            return fail(
                    QStringLiteral("SqliteStore: cannot create %1.").arg(directory.absolutePath()));
        }
    }

    // Everything that touches the connection stays inside this scope, so that
    // nothing holds a copy of it when the failure path below removes it.
    int found = 0;
    QString error;
    {
        QSqlDatabase database
                = QSqlDatabase::addDatabase(QLatin1String(DriverName), d->connectionName);
        database.setDatabaseName(d->filePath);
        if (!database.open()) {
            error = QStringLiteral("SqliteStore: cannot open %1: %2")
                            .arg(d->filePath, database.lastError().text());
        } else {
            error = d->prepareSchema(&found);
            if (!error.isEmpty())
                database.close();
        }
    }
    if (!error.isEmpty()) {
        QSqlDatabase::removeDatabase(d->connectionName);
        return fail(error);
    }

    d->schemaVersion = found;
    d->open = true;
    return true;
}

void SqliteStore::close()
{
    if (!d->open)
        return;
    d->open = false;
    d->schemaVersion = 0;
    // Closing inside a batch drops it: SQLite rolls an open transaction back
    // when the connection goes, and the counter must not survive into a
    // reopened store as a batch nothing began.
    d->batchDepth = 0;
    d->batchAborted = false;
    {
        // The connection has to go out of scope before removeDatabase(), which
        // warns about connections still in use -- and then leaves the
        // connection behind, so reopening would find the name taken.
        QSqlDatabase database = QSqlDatabase::database(d->connectionName, false);
        if (database.isValid())
            database.close();
    }
    QSqlDatabase::removeDatabase(d->connectionName);
}

bool SqliteStore::isOpen() const { return d->open; }

int SqliteStore::schemaVersion() const { return d->schemaVersion; }

bool SqliteStore::beginBatch()
{
    // lastError() is deliberately not cleared here or in endBatch(): a batch
    // is ended on the way out of a failure, and clearing would erase the
    // reason the caller is about to read.
    if (!d->open)
        return fail(QStringLiteral("SqliteStore: not open."));
    if (d->batchDepth > 0) {
        ++d->batchDepth;
        return true;
    }

    QSqlDatabase database = d->database();
    if (!database.transaction()) {
        return fail(QStringLiteral("SqliteStore: cannot begin a batch: %1")
                            .arg(database.lastError().text()));
    }
    d->batchDepth = 1;
    d->batchAborted = false;
    return true;
}

bool SqliteStore::endBatch(bool commit)
{
    if (d->batchDepth == 0)
        return fail(QStringLiteral("SqliteStore: no batch to end."));
    if (!commit)
        d->batchAborted = true;
    if (--d->batchDepth > 0)
        return true;

    QSqlDatabase database = d->database();
    const bool dropping = d->batchAborted;
    d->batchAborted = false;
    if (dropping ? database.rollback() : database.commit())
        return true;
    return fail(QStringLiteral("SqliteStore: cannot %1 a batch: %2")
                        .arg(dropping ? QStringLiteral("roll back") : QStringLiteral("commit"),
                             database.lastError().text()));
}

bool SqliteStore::saveConversation(const QString &id, const Chat::Transcript &transcript,
                                   const QString &title)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("SqliteStore: not open."));
    if (id.isEmpty())
        return fail(QStringLiteral("SqliteStore: a conversation needs a non-empty id."));

    QSqlQuery query = d->query();
    QString keptTitle;
    qint64 createdAt = 0;
    if (d->exec(query, QStringLiteral("SELECT title, created_at FROM conversations WHERE id = ?"),
                {id})
        && query.next()) {
        keptTitle = query.value(0).toString();
        createdAt = query.value(1).toLongLong();
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (createdAt == 0)
        createdAt = now;
    // An empty title on an existing conversation keeps the one it has: a save
    // from a code path that does not know the title must not erase it.
    const QString storedTitle = title.isEmpty() ? keptTitle : title;

    if (!d->exec(query,
                 QStringLiteral("INSERT OR REPLACE INTO conversations "
                                "(id, title, created_at, updated_at, message_count, transcript) "
                                "VALUES (?, ?, ?, ?, ?, ?)"),
                 {id, storedTitle, createdAt, now, transcript.count(),
                  compactJsonText(transcript.toJson())})) {
        return fail(QStringLiteral("SqliteStore: cannot save conversation %1: %2")
                            .arg(id, query.lastError().text()));
    }
    return true;
}

std::optional<Chat::Transcript> SqliteStore::loadConversation(const QString &id)
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("SqliteStore: not open."));
        return std::nullopt;
    }
    QSqlQuery query = d->query();
    if (!d->exec(query, QStringLiteral("SELECT transcript FROM conversations WHERE id = ?"), {id})
        || !query.next()) {
        return std::nullopt;
    }
    return Chat::Transcript::fromJson(parse(query.value(0).toString()));
}

std::optional<ConversationRecord> SqliteStore::conversation(const QString &id)
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("SqliteStore: not open."));
        return std::nullopt;
    }
    QSqlQuery query = d->query();
    if (!d->exec(query, conversationSelect() + QStringLiteral(" WHERE id = ?"), {id})
        || !query.next()) {
        return std::nullopt;
    }
    return recordFromRow(query);
}

QList<ConversationRecord> SqliteStore::conversations() { return conversations(-1, 0); }

QList<ConversationRecord> SqliteStore::conversations(int limit, int offset)
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("SqliteStore: not open."));
        return {};
    }
    if (limit == 0)
        return {};

    QSqlQuery query = d->query();
    // Ties broken by id so the order is stable rather than the storage order.
    // The bounds are in the statement rather than applied to the result: this
    // way the fifty rows a sidebar shows are the fifty the index walks, and
    // the other nine thousand nine hundred and fifty are never decoded. SQLite
    // spells "no limit" as a negative one, and wants a LIMIT before an OFFSET.
    if (!d->exec(query,
                 conversationSelect()
                         + QStringLiteral(" ORDER BY updated_at DESC, id ASC LIMIT ? OFFSET ?"),
                 {limit < 0 ? -1 : limit, qMax(0, offset)})) {
        fail(QStringLiteral("SqliteStore: cannot list conversations: %1")
                     .arg(query.lastError().text()));
        return {};
    }

    QList<ConversationRecord> records;
    while (query.next())
        records.append(recordFromRow(query));
    return records;
}

bool SqliteStore::removeConversation(const QString &id)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("SqliteStore: not open."));
    QSqlQuery query = d->query();
    if (!d->exec(query, QStringLiteral("DELETE FROM conversations WHERE id = ?"), {id})) {
        return fail(QStringLiteral("SqliteStore: cannot remove conversation %1: %2")
                            .arg(id, query.lastError().text()));
    }
    return true;
}

bool SqliteStore::saveCachedResponse(const CachedResponse &response)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("SqliteStore: not open."));
    if (response.key.isEmpty())
        return fail(QStringLiteral("SqliteStore: a cached response needs a key."));

    const qint64 storedAt = response.storedAt.isValid() ? toStamp(response.storedAt)
                                                        : QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query = d->query();
    if (!d->exec(query,
                 QStringLiteral("INSERT OR REPLACE INTO cache (key, body, stored_at) "
                                "VALUES (?, ?, ?)"),
                 {response.key, response.body, storedAt})) {
        return fail(QStringLiteral("SqliteStore: cannot save a cached response: %1")
                            .arg(query.lastError().text()));
    }
    return true;
}

std::optional<CachedResponse> SqliteStore::cachedResponse(const QByteArray &key)
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("SqliteStore: not open."));
        return std::nullopt;
    }
    QSqlQuery query = d->query();
    if (!d->exec(query, QStringLiteral("SELECT body, stored_at FROM cache WHERE key = ?"), {key})
        || !query.next()) {
        return std::nullopt;
    }

    CachedResponse response;
    response.key = key;
    response.body = query.value(0).toByteArray();
    response.storedAt = fromStamp(query.value(1).toLongLong());
    return response;
}

bool SqliteStore::removeCachedResponse(const QByteArray &key)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("SqliteStore: not open."));
    QSqlQuery query = d->query();
    if (!d->exec(query, QStringLiteral("DELETE FROM cache WHERE key = ?"), {key})) {
        return fail(QStringLiteral("SqliteStore: cannot remove a cached response: %1")
                            .arg(query.lastError().text()));
    }
    return true;
}

bool SqliteStore::clearCachedResponses()
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("SqliteStore: not open."));
    QSqlQuery query = d->query();
    if (!d->exec(query, QStringLiteral("DELETE FROM cache"))) {
        return fail(QStringLiteral("SqliteStore: cannot clear the cache: %1")
                            .arg(query.lastError().text()));
    }
    return true;
}

int SqliteStore::cachedResponseCount()
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("SqliteStore: not open."));
        return 0;
    }
    QSqlQuery query = d->query();
    if (!d->exec(query, QStringLiteral("SELECT COUNT(*) FROM cache")) || !query.next())
        return 0;
    return query.value(0).toInt();
}

bool SqliteStore::pruneCachedResponses(int maxEntries, const QDateTime &oldest)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("SqliteStore: not open."));

    // The two halves are one batch: two autocommits are two commits, and a
    // prune that expired the old entries and then failed to apply the ceiling
    // is a state no caller asked for. Nested inside the caller's batch when
    // there is one -- PersistentResponseCache::insert() opens one around the
    // save and this prune.
    Batch batch(this);
    QSqlQuery query = d->query();
    if (oldest.isValid()
        && !d->exec(query, QStringLiteral("DELETE FROM cache WHERE stored_at < ?"),
                    {toStamp(oldest)})) {
        batch.abort();
        return fail(QStringLiteral("SqliteStore: cannot expire cached responses: %1")
                            .arg(query.lastError().text()));
    }

    // Everything outside the newest `maxEntries` rows, in one statement over
    // the stored_at index.
    if (maxEntries >= 0
        && !d->exec(query,
                    QStringLiteral("DELETE FROM cache WHERE key NOT IN "
                                   "(SELECT key FROM cache ORDER BY stored_at DESC LIMIT ?)"),
                    {maxEntries})) {
        batch.abort();
        return fail(QStringLiteral("SqliteStore: cannot prune cached responses: %1")
                            .arg(query.lastError().text()));
    }
    return batch.commit();
}

bool SqliteStore::saveMetrics(const QString &id, const Client::MetricsSnapshot &snapshot)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("SqliteStore: not open."));
    if (id.isEmpty())
        return fail(QStringLiteral("SqliteStore: a metrics snapshot needs a non-empty id."));

    QSqlQuery query = d->query();
    if (!d->exec(query,
                 QStringLiteral("INSERT OR REPLACE INTO metrics (id, snapshot) VALUES (?, ?)"),
                 {id, compactJsonText(snapshot.toJson())})) {
        return fail(QStringLiteral("SqliteStore: cannot save metrics %1: %2")
                            .arg(id, query.lastError().text()));
    }
    return true;
}

std::optional<Client::MetricsSnapshot> SqliteStore::loadMetrics(const QString &id)
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("SqliteStore: not open."));
        return std::nullopt;
    }
    QSqlQuery query = d->query();
    if (!d->exec(query, QStringLiteral("SELECT snapshot FROM metrics WHERE id = ?"), {id})
        || !query.next()) {
        return std::nullopt;
    }
    return Client::MetricsSnapshot::fromJson(parse(query.value(0).toString()));
}

bool SqliteStore::removeMetrics(const QString &id)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("SqliteStore: not open."));
    QSqlQuery query = d->query();
    if (!d->exec(query, QStringLiteral("DELETE FROM metrics WHERE id = ?"), {id})) {
        return fail(QStringLiteral("SqliteStore: cannot remove metrics %1: %2")
                            .arg(id, query.lastError().text()));
    }
    return true;
}

QStringList SqliteStore::metricsIds()
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("SqliteStore: not open."));
        return {};
    }
    QSqlQuery query = d->query();
    if (!d->exec(query, QStringLiteral("SELECT id FROM metrics ORDER BY id ASC"))) {
        fail(QStringLiteral("SqliteStore: cannot list metrics: %1").arg(query.lastError().text()));
        return {};
    }
    QStringList ids;
    while (query.next())
        ids.append(query.value(0).toString());
    return ids;
}

} // namespace Sql
} // namespace QtOpenAi
