// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Chat/Transcript.h>
#include <QtOpenAi/Client/Metrics.h>
#include <QtOpenAi/Storage/GlobalStorage.h>

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <optional>

namespace QtOpenAi {
namespace Storage {

// What a store knows about a conversation without loading it.
//
// Listing is the operation a chat application performs on every start, for
// every conversation it has; loading is the one it performs for the single
// conversation the user picks. Separating them is what keeps the first cheap.
struct QTOPENAI_STORAGE_EXPORT ConversationRecord
{
    QString id;
    QString title;
    QDateTime createdAt;
    QDateTime updatedAt;
    // Nodes in the tree, not messages on the active path: a branch the user is
    // not currently on is still stored, and still theirs.
    int messageCount = 0;

    bool isValid() const { return !id.isEmpty(); }
};

// One cached response body and when it was stored.
//
// Expiry is not in here because it is not a property of the entry: the same
// body is fresh for a caller who allows an hour and stale for one who allows a
// minute. The store keeps the timestamp; PersistentResponseCache decides.
struct QTOPENAI_STORAGE_EXPORT CachedResponse
{
    QByteArray key;
    QByteArray body;
    QDateTime storedAt;
};

class StorePrivate;

// Where a conversation, a response cache and a metrics snapshot survive a
// restart.
//
// An interface rather than a class, for the same reason ResponseCache is one:
// the useful backends differ in kind. JsonFileStore is a directory of readable
// files that a user can copy, diff or delete by hand; Sql::SqliteStore is one
// indexed file that stays fast with ten thousand conversations in it. Both
// implement exactly this, so an application chooses at the one line that
// constructs the store.
//
//     Storage::JsonFileStore store(path);
//     if (!store.open())
//         qWarning() << store.lastError();
//     store.saveConversation(QStringLiteral("chat-1"), transcript, tr("Sky colours"));
//
// **Nothing here is const**, including the queries. A lookup is allowed to
// change the store -- an SQL backend prepares and caches statements, any
// backend records why the last call failed -- and a const method that has to
// cast the promise away is a worse interface than one that never made it.
//
// A failed call returns false (or an empty optional) and leaves the reason in
// lastError(). Failures here are filesystem and database conditions -- a
// read-only directory, a full disk, a corrupted file -- so they are reported
// rather than thrown or asserted: the application that has just lost its disk
// still has a conversation on screen to tell the user about.
class QTOPENAI_STORAGE_EXPORT Store
{
public:
    // The layout this build writes. A store opened at a *lower* version is
    // migrated on open; one at a higher version is refused rather than read on
    // a guess, because the file belongs to a newer library that knows what it
    // put there and this one does not.
    static constexpr int CurrentSchemaVersion = 1;

    Store();
    virtual ~Store();

    // Creates the store if it is not there, migrates it if it is older, and
    // fails if it is newer or unreadable. Opening an open store is a no-op.
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // The version found in the store, or 0 when it is not open.
    virtual int schemaVersion() const = 0;

    // Why the last call failed. Cleared by the next call that succeeds.
    QString lastError() const;

    // --- Conversations ----------------------------------------------------
    // Insert or replace. `title` is the application's label for the
    // conversation -- an empty one on an existing conversation keeps the title
    // it already had, so a save from a code path that does not know the title
    // does not erase it.
    virtual bool saveConversation(const QString &id, const Chat::Transcript &transcript,
                                  const QString &title = {})
            = 0;
    // Nothing when there is no such conversation, which is not an error.
    virtual std::optional<Chat::Transcript> loadConversation(const QString &id) = 0;
    virtual std::optional<ConversationRecord> conversation(const QString &id) = 0;
    // Most recently updated first: the order a conversation list is shown in.
    virtual QList<ConversationRecord> conversations() = 0;
    virtual bool removeConversation(const QString &id) = 0;

    // --- Cached responses -------------------------------------------------
    // Keys are opaque; CachingInterceptor::cacheKey() is what produces them.
    virtual bool saveCachedResponse(const CachedResponse &response) = 0;
    virtual std::optional<CachedResponse> cachedResponse(const QByteArray &key) = 0;
    virtual bool removeCachedResponse(const QByteArray &key) = 0;
    virtual bool clearCachedResponses() = 0;
    virtual int cachedResponseCount() = 0;
    // Drop everything stored before `oldest`, then the oldest entries beyond
    // `maxEntries`. Both bounds in one call because a backend applies them in
    // one statement over an index, where the caller doing it entry by entry
    // would be a query per entry. A negative `maxEntries` or an invalid
    // `oldest` is no bound.
    virtual bool pruneCachedResponses(int maxEntries, const QDateTime &oldest) = 0;

    // --- Metrics ----------------------------------------------------------
    // Keyed, rather than one snapshot per store, because "this month" and "all
    // time" are two counters an application may well keep side by side.
    virtual bool saveMetrics(const QString &id, const Client::MetricsSnapshot &snapshot) = 0;
    virtual std::optional<Client::MetricsSnapshot> loadMetrics(const QString &id) = 0;
    virtual bool removeMetrics(const QString &id) = 0;
    virtual QStringList metricsIds() = 0;

protected:
    // For backends: record why a call failed and return false, so the failure
    // path is one line at each call site.
    bool fail(const QString &message);
    void clearError();

private:
    Q_DISABLE_COPY(Store)
    QScopedPointer<StorePrivate> d;
};

} // namespace Storage
} // namespace QtOpenAi
