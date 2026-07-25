// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/FileObject.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class FileObjectData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 bytes = 0;
    qint64 createdAt = 0;
    qint64 expiresAt = 0;
    QString filename;
    QString purpose;
    QString status;
    QString statusDetails;
};

FileObject::FileObject()
    : d(new FileObjectData)
{ }

FileObject::FileObject(const FileObject &other) = default;
FileObject::FileObject(FileObject &&other) noexcept = default;
FileObject &FileObject::operator=(const FileObject &other) = default;
FileObject &FileObject::operator=(FileObject &&other) noexcept = default;
FileObject::~FileObject() = default;

QString FileObject::id() const { return d->id; }
void FileObject::setId(const QString &id) { d->id = id; }

QString FileObject::object() const { return d->object; }
void FileObject::setObject(const QString &object) { d->object = object; }

qint64 FileObject::bytes() const { return d->bytes; }
void FileObject::setBytes(qint64 bytes) { d->bytes = bytes; }

qint64 FileObject::createdAt() const { return d->createdAt; }
void FileObject::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

qint64 FileObject::expiresAt() const { return d->expiresAt; }
void FileObject::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

QString FileObject::filename() const { return d->filename; }
void FileObject::setFilename(const QString &filename) { d->filename = filename; }

QString FileObject::purpose() const { return d->purpose; }
void FileObject::setPurpose(const QString &purpose) { d->purpose = purpose; }

QString FileObject::status() const { return d->status; }
void FileObject::setStatus(const QString &status) { d->status = status; }

QString FileObject::statusDetails() const { return d->statusDetails; }
void FileObject::setStatusDetails(const QString &statusDetails)
{
    d->statusDetails = statusDetails;
}

QJsonObject FileObject::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("bytes"), d->bytes);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    detail::insertIfNotEmpty(json, QStringLiteral("filename"), d->filename);
    detail::insertIfNotEmpty(json, QStringLiteral("purpose"), d->purpose);
    detail::insertIfNotEmpty(json, QStringLiteral("status"), d->status);
    detail::insertIfNotEmpty(json, QStringLiteral("status_details"), d->statusDetails);
    return json;
}

FileObject FileObject::fromJson(const QJsonObject &json)
{
    FileObject file;
    file.d->id = detail::stringOr(json, QStringLiteral("id"));
    file.d->object = detail::stringOr(json, QStringLiteral("object"));
    file.d->bytes = detail::int64Or(json, QStringLiteral("bytes"));
    file.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    file.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    file.d->filename = detail::stringOr(json, QStringLiteral("filename"));
    file.d->purpose = detail::stringOr(json, QStringLiteral("purpose"));
    file.d->status = detail::stringOr(json, QStringLiteral("status"));
    file.d->statusDetails = detail::stringOr(json, QStringLiteral("status_details"));
    return file;
}

bool FileObject::operator==(const FileObject &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->bytes == other.d->bytes
           && d->createdAt == other.d->createdAt && d->expiresAt == other.d->expiresAt
           && d->filename == other.d->filename && d->purpose == other.d->purpose
           && d->status == other.d->status && d->statusDetails == other.d->statusDetails;
}

} // namespace Core
} // namespace QtOpenAi
