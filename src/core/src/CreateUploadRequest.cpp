// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateUploadRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CreateUploadRequestData : public QSharedData
{
public:
    QString filename;
    QString purpose;
    qint64 bytes = 0;
    QString mimeType;
};

CreateUploadRequest::CreateUploadRequest()
    : d(new CreateUploadRequestData)
{ }

CreateUploadRequest::CreateUploadRequest(QString filename, QString purpose, qint64 bytes,
                                         QString mimeType)
    : d(new CreateUploadRequestData)
{
    d->filename = std::move(filename);
    d->purpose = std::move(purpose);
    d->bytes = bytes;
    d->mimeType = std::move(mimeType);
}

CreateUploadRequest::CreateUploadRequest(const CreateUploadRequest &other) = default;
CreateUploadRequest::CreateUploadRequest(CreateUploadRequest &&other) noexcept = default;
CreateUploadRequest &CreateUploadRequest::operator=(const CreateUploadRequest &other) = default;
CreateUploadRequest &CreateUploadRequest::operator=(CreateUploadRequest &&other) noexcept = default;
CreateUploadRequest::~CreateUploadRequest() = default;

QString CreateUploadRequest::filename() const { return d->filename; }
void CreateUploadRequest::setFilename(const QString &filename) { d->filename = filename; }

QString CreateUploadRequest::purpose() const { return d->purpose; }
void CreateUploadRequest::setPurpose(const QString &purpose) { d->purpose = purpose; }

qint64 CreateUploadRequest::bytes() const { return d->bytes; }
void CreateUploadRequest::setBytes(qint64 bytes) { d->bytes = bytes; }

QString CreateUploadRequest::mimeType() const { return d->mimeType; }
void CreateUploadRequest::setMimeType(const QString &mimeType) { d->mimeType = mimeType; }

QJsonObject CreateUploadRequest::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("filename"), d->filename);
    detail::insertIfNotEmpty(json, QStringLiteral("purpose"), d->purpose);
    json.insert(QStringLiteral("bytes"), d->bytes);
    detail::insertIfNotEmpty(json, QStringLiteral("mime_type"), d->mimeType);
    return json;
}

bool CreateUploadRequest::operator==(const CreateUploadRequest &other) const
{
    return d->filename == other.d->filename && d->purpose == other.d->purpose
           && d->bytes == other.d->bytes && d->mimeType == other.d->mimeType;
}

} // namespace Core
} // namespace QtOpenAi
