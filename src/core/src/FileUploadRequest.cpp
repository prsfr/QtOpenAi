// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/FileUploadRequest.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class FileUploadRequestData : public QSharedData
{
public:
    QByteArray fileData;
    QString fileName;
    QString purpose;
    QString expiresAfterAnchor;
    std::optional<qint64> expiresAfterSeconds;
};

FileUploadRequest::FileUploadRequest()
    : d(new FileUploadRequestData)
{ }

FileUploadRequest::FileUploadRequest(QByteArray fileData, QString fileName, QString purpose)
    : d(new FileUploadRequestData)
{
    d->fileData = std::move(fileData);
    d->fileName = std::move(fileName);
    d->purpose = std::move(purpose);
}

FileUploadRequest::FileUploadRequest(const FileUploadRequest &other) = default;
FileUploadRequest::FileUploadRequest(FileUploadRequest &&other) noexcept = default;
FileUploadRequest &FileUploadRequest::operator=(const FileUploadRequest &other) = default;
FileUploadRequest &FileUploadRequest::operator=(FileUploadRequest &&other) noexcept = default;
FileUploadRequest::~FileUploadRequest() = default;

QByteArray FileUploadRequest::fileData() const { return d->fileData; }
void FileUploadRequest::setFileData(const QByteArray &fileData) { d->fileData = fileData; }

QString FileUploadRequest::fileName() const { return d->fileName; }
void FileUploadRequest::setFileName(const QString &fileName) { d->fileName = fileName; }

QString FileUploadRequest::purpose() const { return d->purpose; }
void FileUploadRequest::setPurpose(const QString &purpose) { d->purpose = purpose; }

QString FileUploadRequest::expiresAfterAnchor() const { return d->expiresAfterAnchor; }

std::optional<qint64> FileUploadRequest::expiresAfterSeconds() const
{
    return d->expiresAfterSeconds;
}

void FileUploadRequest::setExpiresAfter(const QString &anchor, qint64 seconds)
{
    d->expiresAfterAnchor = anchor;
    d->expiresAfterSeconds = seconds;
}

QList<FileUploadRequest::FormField> FileUploadRequest::formFields() const
{
    QList<FormField> fields;
    fields.append({QStringLiteral("purpose"), d->purpose});
    // Nested objects are sent with the bracket convention multipart uses.
    if (d->expiresAfterSeconds) {
        fields.append({QStringLiteral("expires_after[anchor]"), d->expiresAfterAnchor});
        fields.append({QStringLiteral("expires_after[seconds]"),
                       QString::number(*d->expiresAfterSeconds)});
    }
    return fields;
}

bool FileUploadRequest::operator==(const FileUploadRequest &other) const
{
    return d->fileData == other.d->fileData && d->fileName == other.d->fileName
           && d->purpose == other.d->purpose && d->expiresAfterAnchor == other.d->expiresAfterAnchor
           && d->expiresAfterSeconds == other.d->expiresAfterSeconds;
}

} // namespace Core
} // namespace QtOpenAi
