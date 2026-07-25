// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/VectorStore.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- VectorStoreFileCounts -------------------------------------------------

QJsonObject VectorStoreFileCounts::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("in_progress"), inProgress);
    json.insert(QStringLiteral("completed"), completed);
    json.insert(QStringLiteral("cancelled"), cancelled);
    json.insert(QStringLiteral("failed"), failed);
    json.insert(QStringLiteral("total"), total);
    return json;
}

VectorStoreFileCounts VectorStoreFileCounts::fromJson(const QJsonObject &json)
{
    VectorStoreFileCounts counts;
    counts.inProgress = json.value(QStringLiteral("in_progress")).toInt();
    counts.completed = json.value(QStringLiteral("completed")).toInt();
    counts.cancelled = json.value(QStringLiteral("cancelled")).toInt();
    counts.failed = json.value(QStringLiteral("failed")).toInt();
    counts.total = json.value(QStringLiteral("total")).toInt();
    return counts;
}

// --- VectorStore -----------------------------------------------------------

class VectorStoreData : public QSharedData
{
public:
    QString id;
    QString object;
    qint64 createdAt = 0;
    QString name;
    qint64 usageBytes = 0;
    VectorStoreFileCounts fileCounts;
    VectorStoreStatus status = VectorStoreStatus::InProgress;
    QString expiresAfterAnchor;
    int expiresAfterDays = 0;
    qint64 expiresAt = 0;
    qint64 lastActiveAt = 0;
    QJsonObject metadata;
};

VectorStore::VectorStore()
    : d(new VectorStoreData)
{ }

VectorStore::VectorStore(const VectorStore &other) = default;
VectorStore::VectorStore(VectorStore &&other) noexcept = default;
VectorStore &VectorStore::operator=(const VectorStore &other) = default;
VectorStore &VectorStore::operator=(VectorStore &&other) noexcept = default;
VectorStore::~VectorStore() = default;

QString VectorStore::id() const { return d->id; }
void VectorStore::setId(const QString &id) { d->id = id; }

QString VectorStore::object() const { return d->object; }
void VectorStore::setObject(const QString &object) { d->object = object; }

qint64 VectorStore::createdAt() const { return d->createdAt; }
void VectorStore::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString VectorStore::name() const { return d->name; }
void VectorStore::setName(const QString &name) { d->name = name; }

qint64 VectorStore::usageBytes() const { return d->usageBytes; }
void VectorStore::setUsageBytes(qint64 usageBytes) { d->usageBytes = usageBytes; }

VectorStoreFileCounts VectorStore::fileCounts() const { return d->fileCounts; }
void VectorStore::setFileCounts(const VectorStoreFileCounts &fileCounts)
{
    d->fileCounts = fileCounts;
}

VectorStoreStatus VectorStore::status() const { return d->status; }
void VectorStore::setStatus(VectorStoreStatus status) { d->status = status; }

QString VectorStore::expiresAfterAnchor() const { return d->expiresAfterAnchor; }
int VectorStore::expiresAfterDays() const { return d->expiresAfterDays; }

void VectorStore::setExpiresAfter(const QString &anchor, int days)
{
    d->expiresAfterAnchor = anchor;
    d->expiresAfterDays = days;
}

qint64 VectorStore::expiresAt() const { return d->expiresAt; }
void VectorStore::setExpiresAt(qint64 expiresAt) { d->expiresAt = expiresAt; }

qint64 VectorStore::lastActiveAt() const { return d->lastActiveAt; }
void VectorStore::setLastActiveAt(qint64 lastActiveAt) { d->lastActiveAt = lastActiveAt; }

QJsonObject VectorStore::metadata() const { return d->metadata; }
void VectorStore::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QJsonObject VectorStore::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNonZero(json, QStringLiteral("usage_bytes"), d->usageBytes);
    json.insert(QStringLiteral("file_counts"), d->fileCounts.toJson());
    json.insert(QStringLiteral("status"), vectorStoreStatusToString(d->status));
    if (d->expiresAfterDays > 0) {
        QJsonObject expiresAfter;
        detail::insertIfNotEmpty(expiresAfter, QStringLiteral("anchor"), d->expiresAfterAnchor);
        expiresAfter.insert(QStringLiteral("days"), d->expiresAfterDays);
        json.insert(QStringLiteral("expires_after"), expiresAfter);
    }
    detail::insertIfNonZero(json, QStringLiteral("expires_at"), d->expiresAt);
    detail::insertIfNonZero(json, QStringLiteral("last_active_at"), d->lastActiveAt);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

VectorStore VectorStore::fromJson(const QJsonObject &json)
{
    VectorStore store;
    store.d->id = detail::stringOr(json, QStringLiteral("id"));
    store.d->object = detail::stringOr(json, QStringLiteral("object"));
    store.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    store.d->name = detail::stringOr(json, QStringLiteral("name"));
    store.d->usageBytes = detail::int64Or(json, QStringLiteral("usage_bytes"));
    store.d->fileCounts
            = VectorStoreFileCounts::fromJson(json.value(QStringLiteral("file_counts")).toObject());
    store.d->status = vectorStoreStatusFromString(detail::stringOr(json, QStringLiteral("status")));
    const QJsonValue expiresAfter = json.value(QStringLiteral("expires_after"));
    if (expiresAfter.isObject()) {
        const QJsonObject object = expiresAfter.toObject();
        store.d->expiresAfterAnchor = detail::stringOr(object, QStringLiteral("anchor"));
        store.d->expiresAfterDays = object.value(QStringLiteral("days")).toInt();
    }
    store.d->expiresAt = detail::int64Or(json, QStringLiteral("expires_at"));
    store.d->lastActiveAt = detail::int64Or(json, QStringLiteral("last_active_at"));
    store.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return store;
}

bool VectorStore::operator==(const VectorStore &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->name == other.d->name
           && d->usageBytes == other.d->usageBytes && d->fileCounts == other.d->fileCounts
           && d->status == other.d->status && d->expiresAfterAnchor == other.d->expiresAfterAnchor
           && d->expiresAfterDays == other.d->expiresAfterDays && d->expiresAt == other.d->expiresAt
           && d->lastActiveAt == other.d->lastActiveAt && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
