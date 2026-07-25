// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class FileObjectData;

// One stored `file` object (Files API), the currency of every endpoint that
// consumes uploaded data: fine-tuning, batch, assistants, vector stores and the
// Responses file inputs.
//
// The same shape is returned by the deletion acknowledgement of
// DELETE /files/{id}, where only id() and object() ("file.deleted") are set.
class QTOPENAI_CORE_EXPORT FileObject
{
public:
    FileObject();
    FileObject(const FileObject &other);
    FileObject(FileObject &&other) noexcept;
    FileObject &operator=(const FileObject &other);
    FileObject &operator=(FileObject &&other) noexcept;
    ~FileObject();

    void swap(FileObject &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "file" (or "file.deleted" in a delete reply).
    QString object() const;
    void setObject(const QString &object);

    // File size in bytes; 0 when absent.
    qint64 bytes() const;
    void setBytes(qint64 bytes);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Unix expiry timestamp (`expires_at`); 0 when the file does not expire.
    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    QString filename() const;
    void setFilename(const QString &filename);

    // The intended use, e.g. "fine-tune", "assistants", "batch", "user_data",
    // "vision" or "evals". Kept as a string so provider-specific values survive.
    QString purpose() const;
    void setPurpose(const QString &purpose);

    // Processing state ("uploaded", "processed", "error"); deprecated upstream
    // but still returned, so it is surfaced verbatim.
    QString status() const;
    void setStatus(const QString &status);

    // Human-readable detail for a failed upload; empty otherwise.
    QString statusDetails() const;
    void setStatusDetails(const QString &statusDetails);

    QJsonObject toJson() const;
    static FileObject fromJson(const QJsonObject &json);

    bool operator==(const FileObject &other) const;
    bool operator!=(const FileObject &other) const { return !(*this == other); }

private:
    QSharedDataPointer<FileObjectData> d;
};

// A `list` of files (GET /files). Cursor-paginated; reuses the shared list-page
// type.
using FileList = ListPage<FileObject>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::FileObject)
Q_DECLARE_METATYPE(QtOpenAi::Core::FileObject)
Q_DECLARE_METATYPE(QtOpenAi::Core::FileList)
