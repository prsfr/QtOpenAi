// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Storage/GlobalStorage.h>
#include <QtOpenAi/Storage/Store.h>

namespace QtOpenAi {
namespace Storage {

class JsonFileStorePrivate;

// A Store that is a directory of JSON files.
//
//     Storage::JsonFileStore store(QStandardPaths::writableLocation(
//         QStandardPaths::AppDataLocation) + QStringLiteral("/history"));
//     store.open();
//
// The layout under the root is
//
//     store.json                 the schema version, and nothing else
//     conversations/<name>.json  one conversation, with its record
//     cache/<name>.json          one cached response body
//     metrics/<name>.json        one metrics snapshot
//
// This is the backend to reach for when the data should stay legible: it is
// readable in an editor, diffable in a version-control system, copyable with
// `cp -r`, and recoverable one conversation at a time when something goes
// wrong. Sql::SqliteStore is the one to reach for when there are thousands of
// conversations, because listing here reads every conversation file.
//
// **Every write goes through QSaveFile**, so a crash or a full disk during a
// save leaves the previous version intact rather than a half-written file. A
// store that loses old data while failing to write new data would be worse
// than one that never saved at all.
//
// **File names are the hex of the id's UTF-8**, not the id itself. An id is
// the application's string and may contain a slash, a colon or a newline; two
// ids differing only in case are two conversations on Linux and one on macOS.
// Hex is unreadable but it is reversible, cannot escape the directory, and
// cannot collide -- and the id is stored inside the file as well, so a listing
// never has to decode a name to be correct. Sanitising the id instead would
// have kept the names readable and made two different conversations share a
// file, which is the one outcome a store must not have.
class QTOPENAI_STORAGE_EXPORT JsonFileStore : public Store
{
public:
    // The directory is created on open() if it does not exist.
    explicit JsonFileStore(const QString &rootPath);
    ~JsonFileStore() override;

    QString rootPath() const;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    int schemaVersion() const override;

    bool saveConversation(const QString &id, const Chat::Transcript &transcript,
                          const QString &title = {}) override;
    std::optional<Chat::Transcript> loadConversation(const QString &id) override;
    std::optional<ConversationRecord> conversation(const QString &id) override;
    QList<ConversationRecord> conversations() override;
    bool removeConversation(const QString &id) override;

    bool saveCachedResponse(const CachedResponse &response) override;
    std::optional<CachedResponse> cachedResponse(const QByteArray &key) override;
    bool removeCachedResponse(const QByteArray &key) override;
    bool clearCachedResponses() override;
    int cachedResponseCount() override;
    bool pruneCachedResponses(int maxEntries, const QDateTime &oldest) override;

    bool saveMetrics(const QString &id, const Client::MetricsSnapshot &snapshot) override;
    std::optional<Client::MetricsSnapshot> loadMetrics(const QString &id) override;
    bool removeMetrics(const QString &id) override;
    QStringList metricsIds() override;

private:
    QScopedPointer<JsonFileStorePrivate> d;
};

} // namespace Storage
} // namespace QtOpenAi
