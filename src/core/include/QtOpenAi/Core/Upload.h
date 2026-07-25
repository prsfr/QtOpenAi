// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/FileObject.h>
#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class UploadPartData;

// One chunk of a multipart upload (POST /uploads/{id}/parts). The server only
// hands back a receipt — the part's id, which is replayed to /complete in the
// order the parts should be concatenated.
class QTOPENAI_CORE_EXPORT UploadPart
{
public:
    UploadPart();
    UploadPart(const UploadPart &other);
    UploadPart(UploadPart &&other) noexcept;
    UploadPart &operator=(const UploadPart &other);
    UploadPart &operator=(UploadPart &&other) noexcept;
    ~UploadPart();

    void swap(UploadPart &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "upload.part".
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // The upload this part belongs to.
    QString uploadId() const;
    void setUploadId(const QString &uploadId);

    QJsonObject toJson() const;
    static UploadPart fromJson(const QJsonObject &json);

    bool operator==(const UploadPart &other) const;
    bool operator!=(const UploadPart &other) const { return !(*this == other); }

private:
    QSharedDataPointer<UploadPartData> d;
};

class UploadData;

// A multipart upload (Uploads API), the route for files larger than the
// single-request limit of POST /files.
//
// The flow is start → add parts → complete: creating an upload declares the
// total size up front, each part carries a slice of the bytes, and completing
// it orders the parts and produces the finished file — surfaced here as file().
// ChunkedUploader drives that whole sequence.
class QTOPENAI_CORE_EXPORT Upload
{
public:
    Upload();
    Upload(const Upload &other);
    Upload(Upload &&other) noexcept;
    Upload &operator=(const Upload &other);
    Upload &operator=(Upload &&other) noexcept;
    ~Upload();

    void swap(Upload &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "upload".
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString filename() const;
    void setFilename(const QString &filename);

    // The total size declared when the upload was created.
    qint64 bytes() const;
    void setBytes(qint64 bytes);

    // The purpose the resulting file will carry, e.g. "fine-tune" or "batch".
    QString purpose() const;
    void setPurpose(const QString &purpose);

    UploadStatus status() const;
    void setStatus(UploadStatus status);

    // Unix timestamp after which an incomplete upload is discarded; 0 if absent.
    qint64 expiresAt() const;
    void setExpiresAt(qint64 expiresAt);

    // The assembled file, present once the upload has been completed.
    std::optional<FileObject> file() const;
    void setFile(const FileObject &file);

    // True once the upload can no longer take parts (Completed, Cancelled or
    // Expired).
    bool isTerminal() const;

    QJsonObject toJson() const;
    static Upload fromJson(const QJsonObject &json);

    bool operator==(const Upload &other) const;
    bool operator!=(const Upload &other) const { return !(*this == other); }

private:
    QSharedDataPointer<UploadData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::UploadPart)
Q_DECLARE_SHARED(QtOpenAi::Core::Upload)
Q_DECLARE_METATYPE(QtOpenAi::Core::UploadPart)
Q_DECLARE_METATYPE(QtOpenAi::Core::Upload)
