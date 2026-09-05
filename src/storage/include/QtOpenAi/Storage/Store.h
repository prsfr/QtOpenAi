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

    // --- Batching ---------------------------------------------------------
    // Group the writes between these two calls, where the backend can. A
    // backend that cannot does nothing and reports success: the pair is a hint
    // about batching, never a transaction the caller may rely on for
    // atomicity. Sql::SqliteStore maps them onto one transaction, where ten
    // thousand saves are one commit rather than ten thousand of them;
    // JsonFileStore is a directory of files with no cross-file atomicity to
    // offer, so it inherits the no-ops.
    //
    // `endBatch(false)` asks for the batch to be dropped rather than kept,
    // which a backend that has nothing to drop also reports as success. It is
    // one call rather than a separate abortBatch() so that every path out of a
    // batch is the same call, which is what lets Batch below be a guard.
    //
    // Nesting is counted: the outermost endBatch() is the one that commits,
    // and an inner endBatch(false) drops the whole batch rather than the part
    // of it the inner caller opened. That is what lets this library batch its
    // own writes -- a cache insert and the prune it triggers -- inside a batch
    // the application opened.
    //
    // **Neither call clears lastError()**, unlike the rest of the interface:
    // ending a batch is what happens on the way out of a failure, and it must
    // not erase the reason the caller is about to read.
    virtual bool beginBatch();
    virtual bool endBatch(bool commit = true);

    // A scope guard over the two, so that every path out of a batch -- an
    // early return, a thrown exception -- ends it:
    //
    //     Store::Batch batch(&store);
    //     for (const QString &id : ids)
    //         store.saveConversation(id, transcripts.value(id));
    //     if (!batch.commit())
    //         qWarning() << store.lastError();
    //
    // Committed on destruction unless abort() was called; commit() ends it
    // early and says whether the backend managed it. Header-only and not
    // virtual: it is the caller's convenience over the two calls above, not a
    // third thing a backend implements.
    class Batch
    {
    public:
        explicit Batch(Store *store)
            : m_store(store && store->beginBatch() ? store : nullptr)
        { }
        ~Batch()
        {
            if (m_store)
                m_store->endBatch(m_commit);
        }

        // False when there was no store, or when the backend refused to start
        // -- a closed store is the usual reason. The writes that follow are
        // then unbatched rather than lost, so a caller that does not check is
        // slower rather than wrong.
        bool isActive() const { return m_store != nullptr; }

        // Drop the batch instead of keeping it, on this scope's way out.
        void abort() { m_commit = false; }

        // End it now rather than at the closing brace. False when the backend
        // could not, with the reason in lastError(), and false as well when
        // the batch never started -- the failure the constructor could not
        // report.
        bool commit()
        {
            Store *const store = m_store;
            m_store = nullptr;
            return store && store->endBatch(m_commit);
        }

    private:
        Q_DISABLE_COPY(Batch)
        Store *m_store = nullptr;
        bool m_commit = true;
    };

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
    // The same listing, bounded: the newest `limit` records, skipping the
    // first `offset` of them. A conversation sidebar shows fifty and pages
    // from there, and a backend with an index on that order answers it without
    // reading the other nine thousand nine hundred and fifty -- which is the
    // difference between a startup that lists and one that scans. A negative
    // `limit` is no limit, which is what conversations() is; zero is an empty
    // listing and reads nothing at all.
    //
    // The default asks for the full listing and slices it, so a backend that
    // has nothing better to offer need not implement anything. A backend that
    // does -- Sql::SqliteStore -- overrides this one and expresses the other
    // in terms of it.
    virtual QList<ConversationRecord> conversations(int limit, int offset = 0);
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
