// SPDX-License-Identifier: MIT
#include "QtOpenAi/Storage/JsonFileStore.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>

#include <algorithm>

namespace QtOpenAi {
namespace Storage {

namespace {

// The three collections, as subdirectories of the root.
constexpr auto ConversationsDir = "conversations";
constexpr auto CacheDir = "cache";
constexpr auto MetricsDir = "metrics";
constexpr auto MetaFile = "store.json";

// Ids and cache keys are the application's, so they are not file names. See
// the class comment for why this is hex rather than a sanitised id.
QString fileNameFor(const QByteArray &key)
{
    return QString::fromLatin1(key.toHex()) + QStringLiteral(".json");
}

QString fileNameFor(const QString &id) { return fileNameFor(id.toUtf8()); }

QString isoFromDateTime(const QDateTime &when) { return when.toUTC().toString(Qt::ISODateWithMs); }

QDateTime dateTimeFromIso(const QJsonValue &value)
{
    return QDateTime::fromString(value.toString(), Qt::ISODateWithMs).toUTC();
}

// One stored conversation, decoded. Reading one by id and listing them all need
// exactly this, and a field added to only one of the two is a listing that
// disagrees with the record it lists.
ConversationRecord recordFromJson(const QJsonObject &json)
{
    ConversationRecord record;
    // The id comes from inside the file, so a listing is right even for a file
    // that was renamed or copied in by hand.
    record.id = json.value(QLatin1String("id")).toString();
    record.title = json.value(QLatin1String("title")).toString();
    record.createdAt = dateTimeFromIso(json.value(QLatin1String("created_at")));
    record.updatedAt = dateTimeFromIso(json.value(QLatin1String("updated_at")));
    record.messageCount = json.value(QLatin1String("message_count")).toInt();
    return record;
}

} // namespace

class JsonFileStorePrivate
{
public:
    QString root;
    bool open = false;
    int schemaVersion = 0;

    QDir dir(const char *collection) const
    {
        return QDir(root + QLatin1Char('/') + QLatin1String(collection));
    }

    QString path(const char *collection, const QString &fileName) const
    {
        return root + QLatin1Char('/') + QLatin1String(collection) + QLatin1Char('/') + fileName;
    }

    static std::optional<QJsonObject> read(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return std::nullopt;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject())
            return std::nullopt;
        return document.object();
    }

    // Atomic: a crash mid-write leaves the previous version, not half a file.
    static bool write(const QString &path, const QJsonObject &json)
    {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        file.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
        return file.commit();
    }
};

JsonFileStore::JsonFileStore(const QString &rootPath)
    : d(new JsonFileStorePrivate)
{
    d->root = rootPath;
}

JsonFileStore::~JsonFileStore() = default;

QString JsonFileStore::rootPath() const { return d->root; }

bool JsonFileStore::open()
{
    clearError();
    if (d->open)
        return true;
    if (d->root.isEmpty())
        return fail(QStringLiteral("JsonFileStore: no root path given."));

    QDir root(d->root);
    for (const char *collection : {ConversationsDir, CacheDir, MetricsDir}) {
        if (!root.mkpath(QString::fromLatin1(collection))) {
            return fail(QStringLiteral("JsonFileStore: cannot create %1/%2.")
                                .arg(d->root, QString::fromLatin1(collection)));
        }
    }

    const QString metaPath = d->root + QLatin1Char('/') + QLatin1String(MetaFile);
    const std::optional<QJsonObject> meta = JsonFileStorePrivate::read(metaPath);
    if (!meta) {
        // A fresh store, or one whose meta file is unreadable. Writing the
        // version is what makes it a store rather than a directory.
        if (!JsonFileStorePrivate::write(metaPath, QJsonObject {{QLatin1String("schema_version"),
                                                                 CurrentSchemaVersion}}))
            return fail(QStringLiteral("JsonFileStore: cannot write %1.").arg(metaPath));
        d->schemaVersion = CurrentSchemaVersion;
        d->open = true;
        return true;
    }

    const int found = meta->value(QLatin1String("schema_version")).toInt();
    if (found > CurrentSchemaVersion) {
        // Refused rather than read on a guess: the file belongs to a newer
        // library that knows what it put there, and writing to it with this
        // one's assumptions is how the newer version's data gets lost.
        return fail(QStringLiteral("JsonFileStore: %1 has schema version %2, newer than the %3 "
                                   "this build writes.")
                            .arg(d->root)
                            .arg(found)
                            .arg(CurrentSchemaVersion));
    }
    if (found < 1)
        return fail(QStringLiteral("JsonFileStore: %1 has no usable schema version.").arg(d->root));

    // Migration room. Version 1 is the first, so there is no step to run yet;
    // the branch is here so the next version has one place to add one, and so
    // that an older store is never silently read as if it were current.
    if (found < CurrentSchemaVersion) {
        return fail(
                QStringLiteral("JsonFileStore: no migration from schema version %1.").arg(found));
    }

    d->schemaVersion = found;
    d->open = true;
    return true;
}

void JsonFileStore::close()
{
    d->open = false;
    d->schemaVersion = 0;
}

bool JsonFileStore::isOpen() const { return d->open; }

int JsonFileStore::schemaVersion() const { return d->schemaVersion; }

bool JsonFileStore::saveConversation(const QString &id, const Chat::Transcript &transcript,
                                     const QString &title)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("JsonFileStore: not open."));
    if (id.isEmpty())
        return fail(QStringLiteral("JsonFileStore: a conversation needs a non-empty id."));

    const QString path = d->path(ConversationsDir, fileNameFor(id));
    const std::optional<QJsonObject> existing = JsonFileStorePrivate::read(path);

    const QDateTime now = QDateTime::currentDateTimeUtc();
    QJsonObject json;
    json.insert(QLatin1String("id"), id);
    // An empty title on an existing conversation keeps the one it has: a save
    // from a code path that does not know the title must not erase it.
    json.insert(QLatin1String("title"), title.isEmpty() && existing
                                                ? existing->value(QLatin1String("title")).toString()
                                                : title);
    json.insert(QLatin1String("created_at"),
                existing ? existing->value(QLatin1String("created_at")).toString()
                         : isoFromDateTime(now));
    json.insert(QLatin1String("updated_at"), isoFromDateTime(now));
    json.insert(QLatin1String("message_count"), transcript.count());
    json.insert(QLatin1String("transcript"), transcript.toJson());

    if (!JsonFileStorePrivate::write(path, json))
        return fail(QStringLiteral("JsonFileStore: cannot write %1.").arg(path));
    return true;
}

std::optional<Chat::Transcript> JsonFileStore::loadConversation(const QString &id)
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("JsonFileStore: not open."));
        return std::nullopt;
    }
    const std::optional<QJsonObject> json
            = JsonFileStorePrivate::read(d->path(ConversationsDir, fileNameFor(id)));
    if (!json)
        return std::nullopt;
    return Chat::Transcript::fromJson(json->value(QLatin1String("transcript")).toObject());
}

std::optional<ConversationRecord> JsonFileStore::conversation(const QString &id)
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("JsonFileStore: not open."));
        return std::nullopt;
    }
    const std::optional<QJsonObject> json
            = JsonFileStorePrivate::read(d->path(ConversationsDir, fileNameFor(id)));
    if (!json)
        return std::nullopt;
    return recordFromJson(*json);
}

QList<ConversationRecord> JsonFileStore::conversations()
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("JsonFileStore: not open."));
        return {};
    }

    QList<ConversationRecord> records;
    const QFileInfoList files
            = d->dir(ConversationsDir).entryInfoList({QStringLiteral("*.json")}, QDir::Files);
    for (const QFileInfo &file : files) {
        const std::optional<QJsonObject> json = JsonFileStorePrivate::read(file.absoluteFilePath());
        if (!json)
            continue;
        const ConversationRecord record = recordFromJson(*json);
        if (record.isValid())
            records.append(record);
    }

    std::sort(records.begin(), records.end(),
              [](const ConversationRecord &a, const ConversationRecord &b) {
                  // Most recent first, and ties broken by id so the order is
                  // stable rather than whatever the directory happened to give.
                  if (a.updatedAt != b.updatedAt)
                      return a.updatedAt > b.updatedAt;
                  return a.id < b.id;
              });
    return records;
}

bool JsonFileStore::removeConversation(const QString &id)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("JsonFileStore: not open."));
    const QString path = d->path(ConversationsDir, fileNameFor(id));
    if (!QFile::exists(path))
        return true; // Removing what is not there is the state the caller wanted.
    if (!QFile::remove(path))
        return fail(QStringLiteral("JsonFileStore: cannot remove %1.").arg(path));
    return true;
}

bool JsonFileStore::saveCachedResponse(const CachedResponse &response)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("JsonFileStore: not open."));
    if (response.key.isEmpty())
        return fail(QStringLiteral("JsonFileStore: a cached response needs a key."));

    const QDateTime storedAt
            = response.storedAt.isValid() ? response.storedAt : QDateTime::currentDateTimeUtc();
    // Base64 because a response body is bytes, not text: it may be a JPEG or
    // gzip, and JSON has no way to hold either verbatim.
    const QJsonObject json {{QLatin1String("key"), QString::fromLatin1(response.key.toHex())},
                            {QLatin1String("body"), QString::fromLatin1(response.body.toBase64())},
                            {QLatin1String("stored_at"), isoFromDateTime(storedAt)}};

    const QString path = d->path(CacheDir, fileNameFor(response.key));
    if (!JsonFileStorePrivate::write(path, json))
        return fail(QStringLiteral("JsonFileStore: cannot write %1.").arg(path));
    return true;
}

std::optional<CachedResponse> JsonFileStore::cachedResponse(const QByteArray &key)
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("JsonFileStore: not open."));
        return std::nullopt;
    }
    const std::optional<QJsonObject> json
            = JsonFileStorePrivate::read(d->path(CacheDir, fileNameFor(key)));
    if (!json)
        return std::nullopt;

    CachedResponse response;
    response.key = key;
    response.body
            = QByteArray::fromBase64(json->value(QLatin1String("body")).toString().toLatin1());
    response.storedAt = dateTimeFromIso(json->value(QLatin1String("stored_at")));
    return response;
}

bool JsonFileStore::removeCachedResponse(const QByteArray &key)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("JsonFileStore: not open."));
    const QString path = d->path(CacheDir, fileNameFor(key));
    if (QFile::exists(path) && !QFile::remove(path))
        return fail(QStringLiteral("JsonFileStore: cannot remove %1.").arg(path));
    return true;
}

bool JsonFileStore::clearCachedResponses()
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("JsonFileStore: not open."));
    QDir dir = d->dir(CacheDir);
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.json")}, QDir::Files);
    for (const QFileInfo &file : files) {
        if (!QFile::remove(file.absoluteFilePath())) {
            return fail(QStringLiteral("JsonFileStore: cannot remove %1.")
                                .arg(file.absoluteFilePath()));
        }
    }
    return true;
}

int JsonFileStore::cachedResponseCount()
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("JsonFileStore: not open."));
        return 0;
    }
    return int(d->dir(CacheDir).entryList({QStringLiteral("*.json")}, QDir::Files).size());
}

bool JsonFileStore::pruneCachedResponses(int maxEntries, const QDateTime &oldest)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("JsonFileStore: not open."));

    struct Entry
    {
        QString path;
        QDateTime storedAt;
    };
    QList<Entry> entries;
    const QFileInfoList files
            = d->dir(CacheDir).entryInfoList({QStringLiteral("*.json")}, QDir::Files);
    for (const QFileInfo &file : files) {
        const std::optional<QJsonObject> json = JsonFileStorePrivate::read(file.absoluteFilePath());
        // A file that cannot be parsed is a cache entry that can never be a
        // hit, so pruning is exactly the moment to be rid of it.
        const QDateTime storedAt
                = json ? dateTimeFromIso(json->value(QLatin1String("stored_at"))) : QDateTime();
        entries.append({file.absoluteFilePath(), storedAt});
    }

    const auto drop = [this](const QString &path) {
        return QFile::remove(path)
               || fail(QStringLiteral("JsonFileStore: cannot remove %1.").arg(path));
    };

    if (oldest.isValid()) {
        for (auto it = entries.begin(); it != entries.end();) {
            if (!it->storedAt.isValid() || it->storedAt < oldest) {
                if (!drop(it->path))
                    return false;
                it = entries.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (maxEntries >= 0 && entries.size() > maxEntries) {
        std::sort(entries.begin(), entries.end(),
                  [](const Entry &a, const Entry &b) { return a.storedAt > b.storedAt; });
        for (int i = maxEntries; i < entries.size(); ++i) {
            if (!drop(entries.at(i).path))
                return false;
        }
    }
    return true;
}

bool JsonFileStore::saveMetrics(const QString &id, const Client::MetricsSnapshot &snapshot)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("JsonFileStore: not open."));
    if (id.isEmpty())
        return fail(QStringLiteral("JsonFileStore: a metrics snapshot needs a non-empty id."));

    const QJsonObject json {{QLatin1String("id"), id},
                            {QLatin1String("snapshot"), snapshot.toJson()}};
    const QString path = d->path(MetricsDir, fileNameFor(id));
    if (!JsonFileStorePrivate::write(path, json))
        return fail(QStringLiteral("JsonFileStore: cannot write %1.").arg(path));
    return true;
}

std::optional<Client::MetricsSnapshot> JsonFileStore::loadMetrics(const QString &id)
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("JsonFileStore: not open."));
        return std::nullopt;
    }
    const std::optional<QJsonObject> json
            = JsonFileStorePrivate::read(d->path(MetricsDir, fileNameFor(id)));
    if (!json)
        return std::nullopt;
    return Client::MetricsSnapshot::fromJson(json->value(QLatin1String("snapshot")).toObject());
}

bool JsonFileStore::removeMetrics(const QString &id)
{
    clearError();
    if (!d->open)
        return fail(QStringLiteral("JsonFileStore: not open."));
    const QString path = d->path(MetricsDir, fileNameFor(id));
    if (QFile::exists(path) && !QFile::remove(path))
        return fail(QStringLiteral("JsonFileStore: cannot remove %1.").arg(path));
    return true;
}

QStringList JsonFileStore::metricsIds()
{
    clearError();
    if (!d->open) {
        fail(QStringLiteral("JsonFileStore: not open."));
        return {};
    }
    QStringList ids;
    const QFileInfoList files
            = d->dir(MetricsDir).entryInfoList({QStringLiteral("*.json")}, QDir::Files);
    for (const QFileInfo &file : files) {
        const std::optional<QJsonObject> json = JsonFileStorePrivate::read(file.absoluteFilePath());
        if (json)
            ids.append(json->value(QLatin1String("id")).toString());
    }
    // Sorted by id, not by file name: the names are hex, and a caller reading
    // this list should not have to know that to get the order every backend
    // gives it.
    ids.sort();
    return ids;
}

} // namespace Storage
} // namespace QtOpenAi
