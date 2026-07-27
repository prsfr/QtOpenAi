// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/SkillVersion.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class SkillVersionData : public QSharedData
{
public:
    QString id;
    QString object;
    QString skillId;
    QString version;
    qint64 createdAt = 0;
    QString name;
    QString description;
};

SkillVersion::SkillVersion()
    : d(new SkillVersionData)
{ }

SkillVersion::SkillVersion(const SkillVersion &other) = default;
SkillVersion::SkillVersion(SkillVersion &&other) noexcept = default;
SkillVersion &SkillVersion::operator=(const SkillVersion &other) = default;
SkillVersion &SkillVersion::operator=(SkillVersion &&other) noexcept = default;
SkillVersion::~SkillVersion() = default;

QString SkillVersion::id() const { return d->id; }
void SkillVersion::setId(const QString &id) { d->id = id; }

QString SkillVersion::object() const { return d->object; }
void SkillVersion::setObject(const QString &object) { d->object = object; }

QString SkillVersion::skillId() const { return d->skillId; }
void SkillVersion::setSkillId(const QString &skillId) { d->skillId = skillId; }

QString SkillVersion::version() const { return d->version; }
void SkillVersion::setVersion(const QString &version) { d->version = version; }

qint64 SkillVersion::createdAt() const { return d->createdAt; }
void SkillVersion::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString SkillVersion::name() const { return d->name; }
void SkillVersion::setName(const QString &name) { d->name = name; }

QString SkillVersion::description() const { return d->description; }
void SkillVersion::setDescription(const QString &description) { d->description = description; }

QJsonObject SkillVersion::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("skill_id"), d->skillId);
    detail::insertIfNotEmpty(json, QStringLiteral("version"), d->version);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("description"), d->description);
    return json;
}

SkillVersion SkillVersion::fromJson(const QJsonObject &json)
{
    SkillVersion version;
    version.d->id = detail::stringOr(json, QStringLiteral("id"));
    version.d->object = detail::stringOr(json, QStringLiteral("object"));
    version.d->skillId = detail::stringOr(json, QStringLiteral("skill_id"));
    version.d->version = detail::stringOr(json, QStringLiteral("version"));
    version.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    version.d->name = detail::stringOr(json, QStringLiteral("name"));
    version.d->description = detail::stringOr(json, QStringLiteral("description"));
    return version;
}

bool SkillVersion::operator==(const SkillVersion &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->skillId == other.d->skillId
           && d->version == other.d->version && d->createdAt == other.d->createdAt
           && d->name == other.d->name && d->description == other.d->description;
}

} // namespace Core
} // namespace QtOpenAi
