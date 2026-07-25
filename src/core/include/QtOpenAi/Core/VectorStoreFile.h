// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>
#include <QtOpenAi/Core/VectorStore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class VectorStoreFileData;

// One file attached to a vector store: the association between a stored file and
// the index, together with how its chunking and embedding went.
//
// `attributes` is caller-supplied metadata that search filters can match on, and
// `chunkingStrategy` is kept as raw JSON because the API keeps adding strategies.
class QTOPENAI_CORE_EXPORT VectorStoreFile
{
public:
    VectorStoreFile();
    VectorStoreFile(const VectorStoreFile &other);
    VectorStoreFile(VectorStoreFile &&other) noexcept;
    VectorStoreFile &operator=(const VectorStoreFile &other);
    VectorStoreFile &operator=(VectorStoreFile &&other) noexcept;
    ~VectorStoreFile();

    void swap(VectorStoreFile &other) noexcept { d.swap(other.d); }

    // The id of the underlying file (the same id the Files API returned).
    QString id() const;
    void setId(const QString &id);

    // The object type, normally "vector_store.file".
    QString object() const;
    void setObject(const QString &object);

    // Storage this file consumes inside the store, in bytes.
    qint64 usageBytes() const;
    void setUsageBytes(qint64 usageBytes);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString vectorStoreId() const;
    void setVectorStoreId(const QString &vectorStoreId);

    VectorStoreFileStatus status() const;
    void setStatus(VectorStoreFileStatus status);

    // The code/message of the `last_error` object, populated when ingestion
    // failed; both empty otherwise.
    QString lastErrorCode() const;
    void setLastErrorCode(const QString &lastErrorCode);

    QString lastErrorMessage() const;
    void setLastErrorMessage(const QString &lastErrorMessage);

    // The chunking strategy, kept as raw JSON so new strategies pass through.
    QJsonObject chunkingStrategy() const;
    void setChunkingStrategy(const QJsonObject &chunkingStrategy);

    // Caller-defined key/value pairs that search filters can match on.
    QJsonObject attributes() const;
    void setAttributes(const QJsonObject &attributes);

    QJsonObject toJson() const;
    static VectorStoreFile fromJson(const QJsonObject &json);

    bool operator==(const VectorStoreFile &other) const;
    bool operator!=(const VectorStoreFile &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VectorStoreFileData> d;
};

// A `list` of vector-store files. Cursor-paginated; reuses the shared list-page
// type.
using VectorStoreFileList = ListPage<VectorStoreFile>;

class VectorStoreFileBatchData;

// A batch of files added to a vector store in one call. It exists so a bulk
// ingest can be followed with a single poll instead of one per file; its status
// uses the same value set as an individual file.
class QTOPENAI_CORE_EXPORT VectorStoreFileBatch
{
public:
    VectorStoreFileBatch();
    VectorStoreFileBatch(const VectorStoreFileBatch &other);
    VectorStoreFileBatch(VectorStoreFileBatch &&other) noexcept;
    VectorStoreFileBatch &operator=(const VectorStoreFileBatch &other);
    VectorStoreFileBatch &operator=(VectorStoreFileBatch &&other) noexcept;
    ~VectorStoreFileBatch();

    void swap(VectorStoreFileBatch &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "vector_store.files_batch".
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString vectorStoreId() const;
    void setVectorStoreId(const QString &vectorStoreId);

    VectorStoreFileStatus status() const;
    void setStatus(VectorStoreFileStatus status);

    VectorStoreFileCounts fileCounts() const;
    void setFileCounts(const VectorStoreFileCounts &fileCounts);

    QJsonObject toJson() const;
    static VectorStoreFileBatch fromJson(const QJsonObject &json);

    bool operator==(const VectorStoreFileBatch &other) const;
    bool operator!=(const VectorStoreFileBatch &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VectorStoreFileBatchData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::VectorStoreFile)
Q_DECLARE_SHARED(QtOpenAi::Core::VectorStoreFileBatch)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorStoreFile)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorStoreFileList)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorStoreFileBatch)
