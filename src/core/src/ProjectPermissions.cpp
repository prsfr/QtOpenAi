// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ProjectPermissions.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ProjectModelPermissionsData : public QSharedData
{
public:
    QString object;
    QString mode;
    QStringList modelIds;
    bool deleted = false;
};

ProjectModelPermissions::ProjectModelPermissions()
    : d(new ProjectModelPermissionsData)
{ }

ProjectModelPermissions::ProjectModelPermissions(const ProjectModelPermissions &other) = default;
ProjectModelPermissions::ProjectModelPermissions(ProjectModelPermissions &&other) noexcept
        = default;
ProjectModelPermissions &ProjectModelPermissions::operator=(const ProjectModelPermissions &other)
        = default;
ProjectModelPermissions &
ProjectModelPermissions::operator=(ProjectModelPermissions &&other) noexcept
        = default;
ProjectModelPermissions::~ProjectModelPermissions() = default;

QString ProjectModelPermissions::object() const { return d->object; }
void ProjectModelPermissions::setObject(const QString &object) { d->object = object; }

QString ProjectModelPermissions::mode() const { return d->mode; }
void ProjectModelPermissions::setMode(const QString &mode) { d->mode = mode; }

QStringList ProjectModelPermissions::modelIds() const { return d->modelIds; }
void ProjectModelPermissions::setModelIds(const QStringList &modelIds) { d->modelIds = modelIds; }

std::optional<bool> ProjectModelPermissions::allowsModel(const QString &modelId) const
{
    const bool listed = d->modelIds.contains(modelId);
    if (isAllowList())
        return listed;
    if (isDenyList())
        return !listed;
    // A mode this build cannot read. Answering `true` here would fail open on
    // the one question this class exists to answer, so it answers nothing.
    return std::nullopt;
}

bool ProjectModelPermissions::isDeleted() const { return d->deleted; }
void ProjectModelPermissions::setDeleted(bool deleted) { d->deleted = deleted; }

QJsonObject ProjectModelPermissions::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    detail::insertIfNotEmpty(json, QStringLiteral("mode"), d->mode);
    detail::insertIfNotEmpty(json, QStringLiteral("model_ids"), d->modelIds);
    detail::insertIfTrue(json, QStringLiteral("deleted"), d->deleted);
    return json;
}

ProjectModelPermissions ProjectModelPermissions::fromJson(const QJsonObject &json)
{
    ProjectModelPermissions permissions;
    permissions.d->object = detail::stringOr(json, QStringLiteral("object"));
    permissions.d->mode = detail::stringOr(json, QStringLiteral("mode"));
    permissions.d->modelIds = detail::stringListOr(json, QStringLiteral("model_ids"));
    permissions.d->deleted = json.value(QStringLiteral("deleted")).toBool();
    return permissions;
}

bool ProjectModelPermissions::operator==(const ProjectModelPermissions &other) const
{
    return d->object == other.d->object && d->mode == other.d->mode
           && d->modelIds == other.d->modelIds && d->deleted == other.d->deleted;
}

class ProjectHostedToolPermissionsData : public QSharedData
{
public:
    // Ordered rather than hashed, so the body a test asserts is the body every
    // run produces -- the same reason UsageResult keeps its counters in a QMap.
    QMap<QString, bool> permissions;
};

ProjectHostedToolPermissions::ProjectHostedToolPermissions()
    : d(new ProjectHostedToolPermissionsData)
{ }

ProjectHostedToolPermissions::ProjectHostedToolPermissions(
        const ProjectHostedToolPermissions &other)
        = default;
ProjectHostedToolPermissions::ProjectHostedToolPermissions(
        ProjectHostedToolPermissions &&other) noexcept
        = default;
ProjectHostedToolPermissions &
ProjectHostedToolPermissions::operator=(const ProjectHostedToolPermissions &other)
        = default;
ProjectHostedToolPermissions &
ProjectHostedToolPermissions::operator=(ProjectHostedToolPermissions &&other) noexcept
        = default;
ProjectHostedToolPermissions::~ProjectHostedToolPermissions() = default;

std::optional<bool> ProjectHostedToolPermissions::permission(const QString &tool) const
{
    const auto it = d->permissions.constFind(tool);
    if (it == d->permissions.constEnd())
        return std::nullopt;
    return *it;
}

void ProjectHostedToolPermissions::setPermission(const QString &tool, bool enabled)
{
    d->permissions.insert(tool, enabled);
}

QMap<QString, bool> ProjectHostedToolPermissions::permissions() const { return d->permissions; }
void ProjectHostedToolPermissions::setPermissions(const QMap<QString, bool> &permissions)
{
    d->permissions = permissions;
}

QStringList ProjectHostedToolPermissions::knownTools()
{
    return {QStringLiteral("code_interpreter"), QStringLiteral("file_search"),
            QStringLiteral("image_generation"), QStringLiteral("mcp"),
            QStringLiteral("web_search")};
}

QJsonObject ProjectHostedToolPermissions::toJson() const
{
    QJsonObject json;
    // Each switch is its own little object on the wire: `{"enabled": true}`
    // rather than a bare boolean. Only the tools this object carries are
    // written, which is what makes an update partial.
    for (auto it = d->permissions.constBegin(); it != d->permissions.constEnd(); ++it) {
        QJsonObject entry;
        entry.insert(QStringLiteral("enabled"), it.value());
        json.insert(it.key(), entry);
    }
    return json;
}

ProjectHostedToolPermissions ProjectHostedToolPermissions::fromJson(const QJsonObject &json)
{
    ProjectHostedToolPermissions permissions;
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        // Every value is a `{"enabled": ...}` object; anything else is a field
        // that is not a tool, and skipping it keeps a future sibling key from
        // decoding as a switch that is off.
        const QJsonValue enabled = it.value().toObject().value(QStringLiteral("enabled"));
        if (enabled.isBool())
            permissions.d->permissions.insert(it.key(), enabled.toBool());
    }
    return permissions;
}

bool ProjectHostedToolPermissions::operator==(const ProjectHostedToolPermissions &other) const
{
    return d->permissions == other.d->permissions;
}

} // namespace Core
} // namespace QtOpenAi
