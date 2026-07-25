// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Upload.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- UploadPart ------------------------------------------------------------

class UploadPartData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString uploadId;
};

UploadPart::UploadPart()
    : d(new UploadPartData)
{ }

UploadPart::UploadPart(const UploadPart &other) = default;
UploadPart::UploadPart(UploadPart &&other) noexcept = default;
UploadPart &UploadPart::operator=(const UploadPart &other) = default;
UploadPart &UploadPart::operator=(UploadPart &&other) noexcept = default;
UploadPart::~UploadPart() = default;

QString UploadPart::id() const { return d->id; }
void UploadPart::setId(const QString &id) { d->id = id; }

QString UploadPart::object() const { return d->object; }
void UploadPart::setObject(const QString &object) { d->object = object; }

qint64 UploadPart::createdAt() const { return d->createdAt; }
void UploadPart::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString UploadPart::uploadId() const { return d->uploadId; }
void UploadPart::setUploadId(const QString &uploadId) { d->uploadId = uploadId; }

QJsonObject UploadPart::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("upload_id"), d->uploadId);
    return json;
}

UploadPart UploadPart::fromJson(const QJsonObject &json)
{
    UploadPart part;
    part.d->id = detail::stringOr(json, QStringLiteral("id"));
    part.d->object = detail::stringOr(json, QStringLiteral("object"));
    part.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    part.d->uploadId = detail::stringOr(json, QStringLiteral("upload_id"));
    return part;
}

bool UploadPart::operator==(const UploadPart &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->uploadId == other.d->uploadId;
}

// --- Upload ----------------------------------------------------------------

class UploadData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString filename;
    qint64 bytes = 0;
    QString purpose;
    UploadStatus status = UploadStatus::Pending;
    qint64 expiresAt = 0;
    std::optional<FileObject> file;
};

Upload::Upload()
    : d(new UploadData)
{ }

Upload::Upload(const Upload &other) = default;
Upload::Upload(Upload &&other) noexcept = default;
Upload &Upload::operator=(const Upload &other) = default;
Upload &Upload::operator=(Upload &&other) noexcept = default;
Upload::~Upload() = default;

QString Upload::id() const { return d->id; }
void Upload::setId(const QString &id) { d->id = id; }

QString Upload::object() const { return d->object; }
void Upload::setObject(const QString &object) { d->object = object; }

qint64 Upload::createdAt() const { return d->createdAt; }
void Upload::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString Upload::filename() const { return d->filename; }
void Upload::setFilename(const QString &filename) { d->filename = filename; }

qint64 Upload::bytes() const { return d->bytes; }
void Upload::setBytes(qint64 bytes) { d->bytes = bytes; }

QString Upload::purpose() const { return d->purpose; }
void Upload::setPurpose(const QString &purpose) { d->purpose = purpose; }

UploadStatus Upload::status() const { return d->status; }
void Upload::setStatus(UploadStatus status) { d->status = status; }

qint64 Upload::expiresAt() const { return d->expiresAt; }
void Upload::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

std::optional<FileObject> Upload::file() const { return d->file; }
void Upload::setFile(const FileObject &file) { d->file = file; }

bool Upload::isTerminal() const { return d->status != UploadStatus::Pending; }

QJsonObject Upload::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("filename"), d->filename);
    detail::insertIfNonZero(json, QStringLiteral("bytes"), d->bytes);
    detail::insertIfNotEmpty(json, QStringLiteral("purpose"), d->purpose);
    json.insert(QStringLiteral("status"), uploadStatusToString(d->status));
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    if (d->file)
        json.insert(QStringLiteral("file"), d->file->toJson());
    return json;
}

Upload Upload::fromJson(const QJsonObject &json)
{
    Upload upload;
    upload.d->id = detail::stringOr(json, QStringLiteral("id"));
    upload.d->object = detail::stringOr(json, QStringLiteral("object"));
    upload.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    upload.d->filename = detail::stringOr(json, QStringLiteral("filename"));
    upload.d->bytes = detail::int64Or(json, QStringLiteral("bytes"));
    upload.d->purpose = detail::stringOr(json, QStringLiteral("purpose"));
    upload.d->status = uploadStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    upload.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    const QJsonValue file = json.value(QStringLiteral("file"));
    if (file.isObject())
        upload.d->file = FileObject::fromJson(file.toObject());
    return upload;
}

bool Upload::operator==(const Upload &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->filename == other.d->filename
           && d->bytes == other.d->bytes && d->purpose == other.d->purpose
           && d->status == other.d->status && d->expiresAt == other.d->expiresAt
           && d->file == other.d->file;
}

} // namespace Core
} // namespace QtOpenAi
