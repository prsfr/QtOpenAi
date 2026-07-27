// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Skill.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class SkillData : public QSharedData
{
public:
    QString id;
    QString object;
    QString name;
    QString description;
    qint64 createdAt = 0;
    QString defaultVersion;
    QString latestVersion;
};

Skill::Skill()
    : d(new SkillData)
{ }

Skill::Skill(const Skill &other) = default;
Skill::Skill(Skill &&other) noexcept = default;
Skill &Skill::operator=(const Skill &other) = default;
Skill &Skill::operator=(Skill &&other) noexcept = default;
Skill::~Skill() = default;

QString Skill::id() const { return d->id; }
void Skill::setId(const QString &id) { d->id = id; }

QString Skill::object() const { return d->object; }
void Skill::setObject(const QString &object) { d->object = object; }

QString Skill::name() const { return d->name; }
void Skill::setName(const QString &name) { d->name = name; }

QString Skill::description() const { return d->description; }
void Skill::setDescription(const QString &description) { d->description = description; }

qint64 Skill::createdAt() const { return d->createdAt; }
void Skill::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString Skill::defaultVersion() const { return d->defaultVersion; }
void Skill::setDefaultVersion(const QString &defaultVersion) { d->defaultVersion = defaultVersion; }

QString Skill::latestVersion() const { return d->latestVersion; }
void Skill::setLatestVersion(const QString &latestVersion) { d->latestVersion = latestVersion; }

QJsonObject Skill::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("description"), d->description);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("default_version"), d->defaultVersion);
    detail::insertIfNotEmpty(json, QStringLiteral("latest_version"), d->latestVersion);
    return json;
}

Skill Skill::fromJson(const QJsonObject &json)
{
    Skill skill;
    skill.d->id = detail::stringOr(json, QStringLiteral("id"));
    skill.d->object = detail::stringOr(json, QStringLiteral("object"));
    skill.d->name = detail::stringOr(json, QStringLiteral("name"));
    skill.d->description = detail::stringOr(json, QStringLiteral("description"));
    skill.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    skill.d->defaultVersion = detail::stringOr(json, QStringLiteral("default_version"));
    skill.d->latestVersion = detail::stringOr(json, QStringLiteral("latest_version"));
    return skill;
}

bool Skill::operator==(const Skill &other) const
{
    return d->id == other.d->id && d->object == other.d->object && d->name == other.d->name
           && d->description == other.d->description && d->createdAt == other.d->createdAt
           && d->defaultVersion == other.d->defaultVersion
           && d->latestVersion == other.d->latestVersion;
}

} // namespace Core
} // namespace QtOpenAi
