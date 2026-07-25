// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// How far a vector store (or a file batch) has got through ingesting its files.
// A lightweight value aggregate like RetryPolicy/ListPage rather than a
// d-pointer type: it is five counters with no growth path.
struct QTOPENAI_CORE_EXPORT VectorStoreFileCounts
{
    int inProgress = 0;
    int completed = 0;
    int cancelled = 0;
    int failed = 0;
    int total = 0;

    QJsonObject toJson() const;
    static VectorStoreFileCounts fromJson(const QJsonObject &json);

    bool operator==(const VectorStoreFileCounts &other) const
    {
        return inProgress == other.inProgress && completed == other.completed
               && cancelled == other.cancelled && failed == other.failed && total == other.total;
    }
    bool operator!=(const VectorStoreFileCounts &other) const { return !(*this == other); }
};

class VectorStoreData;

// A managed vector store (Vector Stores API): the server-side index that backs
// file search, both for the Responses `file_search` tool and for Assistants.
//
// Files are added by id (they are uploaded through the Files API first), chunked
// and embedded asynchronously — so a freshly created store starts InProgress and
// becomes searchable once fileCounts().completed catches up.
class QTOPENAI_CORE_EXPORT VectorStore
{
public:
    VectorStore();
    VectorStore(const VectorStore &other);
    VectorStore(VectorStore &&other) noexcept;
    VectorStore &operator=(const VectorStore &other);
    VectorStore &operator=(VectorStore &&other) noexcept;
    ~VectorStore();

    void swap(VectorStore &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "vector_store" (or "vector_store.deleted").
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString name() const;
    void setName(const QString &name);

    // Total storage consumed by the store's files, in bytes.
    qint64 usageBytes() const;
    void setUsageBytes(qint64 usageBytes);

    VectorStoreFileCounts fileCounts() const;
    void setFileCounts(const VectorStoreFileCounts &fileCounts);

    VectorStoreStatus status() const;
    void setStatus(VectorStoreStatus status);

    // Optional expiry policy (`expires_after`): an anchor — currently only
    // "last_active_at" — plus a lifetime in days. Days is 0 when unset.
    QString expiresAfterAnchor() const;
    int expiresAfterDays() const;
    void setExpiresAfter(const QString &anchor, int days);

    // Unix timestamp at which the store expires; 0 when it does not.
    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    // Unix timestamp of the last activity, which drives the expiry anchor.
    qint64 lastActiveAt() const;
    void setLastActiveAt(qint64 lastActiveAt);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QJsonObject toJson() const;
    static VectorStore fromJson(const QJsonObject &json);

    bool operator==(const VectorStore &other) const;
    bool operator!=(const VectorStore &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VectorStoreData> d;
};

// A `list` of vector stores (GET /vector_stores). Cursor-paginated; reuses the
// shared list-page type.
using VectorStoreList = ListPage<VectorStore>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::VectorStore)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorStore)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorStoreList)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorStoreFileCounts)
