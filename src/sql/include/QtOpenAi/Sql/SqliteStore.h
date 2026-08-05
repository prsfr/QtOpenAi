// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Sql/GlobalSql.h>
#include <QtOpenAi/Storage/Store.h>

namespace QtOpenAi {
namespace Sql {

class SqliteStorePrivate;

// A Store in a single SQLite file.
//
//     Sql::SqliteStore store(path + QStringLiteral("/history.db"));
//     if (!store.open())
//         qWarning() << store.lastError();
//
// The backend for an application that accumulates: listing conversations is an
// index scan rather than a read of every conversation, pruning the cache is one
// statement rather than one per entry, and the whole history is one file to
// back up. Storage::JsonFileStore is the one to prefer when the data should
// stay legible on disk.
//
// **SQLite specifically, not "a database".** The schema uses `INSERT OR
// REPLACE` and a `LIMIT` inside a `DELETE ... NOT IN` subquery, both of which
// are SQLite's; the file is a file, so there is no server to configure and
// nothing to fail at deploy time; and it is the driver Qt ships built in.
// Taking a QSqlDatabase from the caller instead would have made the class
// nominally portable and actually tested against exactly one engine, which is
// a claim this could not have kept.
//
// Each store owns its own named connection, so several stores -- and several
// threads, each with its own store -- do not share one. Like the rest of Qt's
// SQL classes, one store belongs to the thread that opened it.
//
// A path of `:memory:` gives a database that lives as long as the store, which
// is what the tests use.
class QTOPENAI_SQL_EXPORT SqliteStore : public Storage::Store
{
public:
    // The file is created on open() if it is not there, along with its
    // directory.
    explicit SqliteStore(const QString &filePath);
    ~SqliteStore() override;

    QString filePath() const;

    // False when the QSQLITE driver is missing from the Qt build. Worth
    // asking before open(), which reports the same thing as an error.
    static bool isAvailable();

    bool open() override;
    void close() override;
    bool isOpen() const override;
    int schemaVersion() const override;

    bool saveConversation(const QString &id, const Chat::Transcript &transcript,
                          const QString &title = {}) override;
    std::optional<Chat::Transcript> loadConversation(const QString &id) override;
    std::optional<Storage::ConversationRecord> conversation(const QString &id) override;
    QList<Storage::ConversationRecord> conversations() override;
    bool removeConversation(const QString &id) override;

    bool saveCachedResponse(const Storage::CachedResponse &response) override;
    std::optional<Storage::CachedResponse> cachedResponse(const QByteArray &key) override;
    bool removeCachedResponse(const QByteArray &key) override;
    bool clearCachedResponses() override;
    int cachedResponseCount() override;
    bool pruneCachedResponses(int maxEntries, const QDateTime &oldest) override;

    bool saveMetrics(const QString &id, const Client::MetricsSnapshot &snapshot) override;
    std::optional<Client::MetricsSnapshot> loadMetrics(const QString &id) override;
    bool removeMetrics(const QString &id) override;
    QStringList metricsIds() override;

private:
    QScopedPointer<SqliteStorePrivate> d;
};

} // namespace Sql
} // namespace QtOpenAi
