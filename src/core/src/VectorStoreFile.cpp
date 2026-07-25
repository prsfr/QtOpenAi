// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/VectorStoreFile.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- VectorStoreFile -------------------------------------------------------

class VectorStoreFileData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 usageBytes = 0;
    qint64 createdAt = 0;
    QString vectorStoreId;
    VectorStoreFileStatus status = VectorStoreFileStatus::InProgress;
    QString lastErrorCode;
    QString lastErrorMessage;
    QJsonObject chunkingStrategy;
    QJsonObject attributes;
};

VectorStoreFile::VectorStoreFile()
    : d(new VectorStoreFileData)
{ }

VectorStoreFile::VectorStoreFile(const VectorStoreFile &other) = default;
VectorStoreFile::VectorStoreFile(VectorStoreFile &&other) noexcept = default;
VectorStoreFile &VectorStoreFile::operator=(const VectorStoreFile &other) = default;
VectorStoreFile &VectorStoreFile::operator=(VectorStoreFile &&other) noexcept = default;
VectorStoreFile::~VectorStoreFile() = default;

QString VectorStoreFile::id() const { return d->id; }
void VectorStoreFile::setId(const QString &id) { d->id = id; }

QString VectorStoreFile::object() const { return d->object; }
void VectorStoreFile::setObject(const QString &object) { d->object = object; }

qint64 VectorStoreFile::usageBytes() const { return d->usageBytes; }
void VectorStoreFile::setUsageBytes(qint64 usageBytes) { d->usageBytes = usageBytes; }

qint64 VectorStoreFile::createdAt() const { return d->createdAt; }
void VectorStoreFile::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString VectorStoreFile::vectorStoreId() const { return d->vectorStoreId; }
void VectorStoreFile::setVectorStoreId(const QString &vectorStoreId)
{
    d->vectorStoreId = vectorStoreId;
}

VectorStoreFileStatus VectorStoreFile::status() const { return d->status; }
void VectorStoreFile::setStatus(VectorStoreFileStatus status) { d->status = status; }

QString VectorStoreFile::lastErrorCode() const { return d->lastErrorCode; }
void VectorStoreFile::setLastErrorCode(const QString &lastErrorCode)
{
    d->lastErrorCode = lastErrorCode;
}

QString VectorStoreFile::lastErrorMessage() const { return d->lastErrorMessage; }
void VectorStoreFile::setLastErrorMessage(const QString &lastErrorMessage)
{
    d->lastErrorMessage = lastErrorMessage;
}

QJsonObject VectorStoreFile::chunkingStrategy() const { return d->chunkingStrategy; }
void VectorStoreFile::setChunkingStrategy(const QJsonObject &chunkingStrategy)
{
    d->chunkingStrategy = chunkingStrategy;
}

QJsonObject VectorStoreFile::attributes() const { return d->attributes; }
void VectorStoreFile::setAttributes(const QJsonObject &attributes) { d->attributes = attributes; }

QJsonObject VectorStoreFile::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("usage_bytes"), d->usageBytes);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("vector_store_id"), d->vectorStoreId);
    json.insert(QStringLiteral("status"), vectorStoreFileStatusToString(d->status));
    if (!d->lastErrorCode.isEmpty() || !d->lastErrorMessage.isEmpty()) {
        QJsonObject lastError;
        detail::insertIfNotEmpty(lastError, QStringLiteral("code"), d->lastErrorCode);
        detail::insertIfNotEmpty(lastError, QStringLiteral("message"), d->lastErrorMessage);
        json.insert(QStringLiteral("last_error"), lastError);
    }
    if (!d->chunkingStrategy.isEmpty())
        json.insert(QStringLiteral("chunking_strategy"), d->chunkingStrategy);
    if (!d->attributes.isEmpty())
        json.insert(QStringLiteral("attributes"), d->attributes);
    return json;
}

VectorStoreFile VectorStoreFile::fromJson(const QJsonObject &json)
{
    VectorStoreFile file;
    file.d->id = detail::stringOr(json, QStringLiteral("id"));
    file.d->object = detail::stringOr(json, QStringLiteral("object"));
    file.d->usageBytes = detail::int64Or(json, QStringLiteral("usage_bytes"));
    file.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    file.d->vectorStoreId = detail::stringOr(json, QStringLiteral("vector_store_id"));
    file.d->status
            = vectorStoreFileStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    const QJsonValue lastError = json.value(QStringLiteral("last_error"));
    if (lastError.isObject()) {
        const QJsonObject object = lastError.toObject();
        file.d->lastErrorCode = detail::stringOr(object, QStringLiteral("code"));
        file.d->lastErrorMessage = detail::stringOr(object, QStringLiteral("message"));
    }
    file.d->chunkingStrategy = json.value(QStringLiteral("chunking_strategy")).toObject();
    file.d->attributes = json.value(QStringLiteral("attributes")).toObject();
    return file;
}

bool VectorStoreFile::operator==(const VectorStoreFile &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->usageBytes == other.d->usageBytes && d->createdAt == other.d->createdAt
           && d->vectorStoreId == other.d->vectorStoreId && d->status == other.d->status
           && d->lastErrorCode == other.d->lastErrorCode
           && d->lastErrorMessage == other.d->lastErrorMessage
           && d->chunkingStrategy == other.d->chunkingStrategy
           && d->attributes == other.d->attributes;
}

// --- VectorStoreFileBatch --------------------------------------------------

class VectorStoreFileBatchData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString vectorStoreId;
    VectorStoreFileStatus status = VectorStoreFileStatus::InProgress;
    VectorStoreFileCounts fileCounts;
};

VectorStoreFileBatch::VectorStoreFileBatch()
    : d(new VectorStoreFileBatchData)
{ }

VectorStoreFileBatch::VectorStoreFileBatch(const VectorStoreFileBatch &other) = default;
VectorStoreFileBatch::VectorStoreFileBatch(VectorStoreFileBatch &&other) noexcept = default;
VectorStoreFileBatch &VectorStoreFileBatch::operator=(const VectorStoreFileBatch &other) = default;
VectorStoreFileBatch &VectorStoreFileBatch::operator=(VectorStoreFileBatch &&other) noexcept
        = default;
VectorStoreFileBatch::~VectorStoreFileBatch() = default;

QString VectorStoreFileBatch::id() const { return d->id; }
void VectorStoreFileBatch::setId(const QString &id) { d->id = id; }

QString VectorStoreFileBatch::object() const { return d->object; }
void VectorStoreFileBatch::setObject(const QString &object) { d->object = object; }

qint64 VectorStoreFileBatch::createdAt() const { return d->createdAt; }
void VectorStoreFileBatch::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString VectorStoreFileBatch::vectorStoreId() const { return d->vectorStoreId; }
void VectorStoreFileBatch::setVectorStoreId(const QString &vectorStoreId)
{
    d->vectorStoreId = vectorStoreId;
}

VectorStoreFileStatus VectorStoreFileBatch::status() const { return d->status; }
void VectorStoreFileBatch::setStatus(VectorStoreFileStatus status) { d->status = status; }

VectorStoreFileCounts VectorStoreFileBatch::fileCounts() const { return d->fileCounts; }
void VectorStoreFileBatch::setFileCounts(const VectorStoreFileCounts &fileCounts)
{
    d->fileCounts = fileCounts;
}

QJsonObject VectorStoreFileBatch::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("vector_store_id"), d->vectorStoreId);
    json.insert(QStringLiteral("status"), vectorStoreFileStatusToString(d->status));
    json.insert(QStringLiteral("file_counts"), d->fileCounts.toJson());
    return json;
}

VectorStoreFileBatch VectorStoreFileBatch::fromJson(const QJsonObject &json)
{
    VectorStoreFileBatch batch;
    batch.d->id = detail::stringOr(json, QStringLiteral("id"));
    batch.d->object = detail::stringOr(json, QStringLiteral("object"));
    batch.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    batch.d->vectorStoreId = detail::stringOr(json, QStringLiteral("vector_store_id"));
    batch.d->status
            = vectorStoreFileStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    batch.d->fileCounts
            = VectorStoreFileCounts::fromJson(json.value(QStringLiteral("file_counts")).toObject());
    return batch;
}

bool VectorStoreFileBatch::operator==(const VectorStoreFileBatch &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->vectorStoreId == other.d->vectorStoreId
           && d->status == other.d->status && d->fileCounts == other.d->fileCounts;
}

} // namespace Core
} // namespace QtOpenAi
