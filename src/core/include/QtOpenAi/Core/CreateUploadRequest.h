// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class CreateUploadRequestData;

// The body of a POST /uploads request, which opens a multipart upload.
//
// The total size has to be declared up front so the server can validate the
// parts that follow; ChunkedUploader fills it in from the payload when bytes()
// is left at 0.
class QTOPENAI_CORE_EXPORT CreateUploadRequest
{
public:
    CreateUploadRequest();
    CreateUploadRequest(QString filename, QString purpose, qint64 bytes, QString mimeType);
    CreateUploadRequest(const CreateUploadRequest &other);
    CreateUploadRequest(CreateUploadRequest &&other) noexcept;
    CreateUploadRequest &operator=(const CreateUploadRequest &other);
    CreateUploadRequest &operator=(CreateUploadRequest &&other) noexcept;
    ~CreateUploadRequest();

    void swap(CreateUploadRequest &other) noexcept { d.swap(other.d); }

    // The name the finished file will carry.
    QString filename() const;
    void setFilename(const QString &filename);

    // The intended use, e.g. "fine-tune", "assistants", "batch" or "vision".
    QString purpose() const;
    void setPurpose(const QString &purpose);

    // The total number of bytes that will be uploaded across all parts.
    qint64 bytes() const;
    void setBytes(qint64 bytes);

    // The MIME type of the finished file, e.g. "text/jsonl".
    QString mimeType() const;
    void setMimeType(const QString &mimeType);

    QJsonObject toJson() const;

    bool operator==(const CreateUploadRequest &other) const;
    bool operator!=(const CreateUploadRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateUploadRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateUploadRequest)
