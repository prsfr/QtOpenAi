// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <optional>

namespace QtOpenAi {
namespace Core {

class ProjectModelPermissionsData;

// Which models a project may use (GET/POST/DELETE
// /organization/projects/{project_id}/model_permissions).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// **This is a policy, not a list of grants.** `modelIds()` on its own says
// nothing: the same id means "permitted" under an allow list and "forbidden"
// under a deny list. Reading the ids without the mode is the one mistake this
// endpoint invites, and it fails open — a caller who treats a deny list as an
// allow list hands out every model it was meant to withhold. Prefer
// allowsModel(), which is the two fields answered together.
//
// The type is separate from ProjectHostedToolPermissions for that reason. #110
// asked whether the two are one type with a discriminator, on the reading that
// both are "grant lists keyed by an identifier". They are not: this one is a
// mode plus an open list of model ids, and the other is a fixed record of named
// switches with no mode at all. One shared type would have had to invent a mode
// for the hosted tools, which have none, and per-key booleans for the models,
// which have none — every caller then reading past the half that does not apply.
class QTOPENAI_CORE_EXPORT ProjectModelPermissions
{
public:
    ProjectModelPermissions();
    ProjectModelPermissions(const ProjectModelPermissions &other);
    ProjectModelPermissions(ProjectModelPermissions &&other) noexcept;
    ProjectModelPermissions &operator=(const ProjectModelPermissions &other);
    ProjectModelPermissions &operator=(ProjectModelPermissions &&other) noexcept;
    ~ProjectModelPermissions();

    void swap(ProjectModelPermissions &other) noexcept { d.swap(other.d); }

    // Normally "project.model_permissions" (or
    // "project.model_permissions.deleted").
    QString object() const;
    void setObject(const QString &object);

    // "allow_list" or "deny_list". Kept as the string the server sent rather
    // than an enum, as every other vocabulary on this surface is: a mode this
    // build has never heard of has to survive a round trip rather than decay to
    // the first enumerator — and on *this* field the first enumerator would
    // turn a deny list into an allow list.
    QString mode() const;
    void setMode(const QString &mode);

    bool isAllowList() const { return mode() == QLatin1String("allow_list"); }
    bool isDenyList() const { return mode() == QLatin1String("deny_list"); }

    // The model ids the policy names. What that *means* depends on the mode.
    QStringList modelIds() const;
    void setModelIds(const QStringList &modelIds);

    // Whether `modelId` may be used, mode and list answered together.
    //
    // Unset when the mode is one this build does not recognise: a policy that
    // cannot be read is not the same as a permissive one, and returning `true`
    // there would fail open. Treat an empty answer as "ask the server".
    std::optional<bool> allowsModel(const QString &modelId) const;

    // True in the answer to DELETE .../model_permissions, which reports the
    // object as "project.model_permissions.deleted" and carries no policy at
    // all -- deleting the policy is how a project goes back to the
    // organization's default.
    bool isDeleted() const;
    void setDeleted(bool deleted);

    QJsonObject toJson() const;
    static ProjectModelPermissions fromJson(const QJsonObject &json);

    bool operator==(const ProjectModelPermissions &other) const;
    bool operator!=(const ProjectModelPermissions &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ProjectModelPermissionsData> d;
};

class ProjectHostedToolPermissionsData;

// Which hosted tools a project may use (GET/POST
// /organization/projects/{project_id}/hosted_tool_permissions).
//
// A record of named switches — file search, web search, image generation, MCP,
// code interpreter — and no mode: a tool is on or it is off, with nothing to
// invert the reading. That is why this is not the same type as
// ProjectModelPermissions; see there.
//
// **One type for both directions**, as Core::ProjectRateLimit and
// Core::RoleRequest are. A read fills in every switch the server knows; an
// update sends only the ones the caller set, so an unmentioned tool is left
// alone rather than switched off:
//
//     Core::ProjectHostedToolPermissions permissions;
//     permissions.setWebSearch(false);        // the only tool touched
//     organization.setProjectHostedToolPermissions(projectId, permissions);
//
// **The five documented tools are typed, and the rest still survive.** They are
// convenience wrappers over permission()/setPermission(), the same shape
// Core::UsageResult gives its counters: the names in the document today are
// spelled out so the common path is checked by the compiler, and a tool added
// after this build round trips through the map rather than being dropped.
class QTOPENAI_CORE_EXPORT ProjectHostedToolPermissions
{
public:
    ProjectHostedToolPermissions();
    ProjectHostedToolPermissions(const ProjectHostedToolPermissions &other);
    ProjectHostedToolPermissions(ProjectHostedToolPermissions &&other) noexcept;
    ProjectHostedToolPermissions &operator=(const ProjectHostedToolPermissions &other);
    ProjectHostedToolPermissions &operator=(ProjectHostedToolPermissions &&other) noexcept;
    ~ProjectHostedToolPermissions();

    void swap(ProjectHostedToolPermissions &other) noexcept { d.swap(other.d); }

    // Whether one tool is enabled, by its wire name ("web_search"). Unset when
    // this object says nothing about it — which for an update is the difference
    // between "switch it off" and "leave it as it is".
    std::optional<bool> permission(const QString &tool) const;
    void setPermission(const QString &tool, bool enabled);

    // Every switch this object carries, in wire-name order.
    QMap<QString, bool> permissions() const;
    void setPermissions(const QMap<QString, bool> &permissions);

    bool isEmpty() const { return permissions().isEmpty(); }

    // --- The tools the API documents today -------------------------------
    std::optional<bool> fileSearch() const { return permission(QStringLiteral("file_search")); }
    void setFileSearch(bool enabled) { setPermission(QStringLiteral("file_search"), enabled); }

    std::optional<bool> webSearch() const { return permission(QStringLiteral("web_search")); }
    void setWebSearch(bool enabled) { setPermission(QStringLiteral("web_search"), enabled); }

    std::optional<bool> imageGeneration() const
    {
        return permission(QStringLiteral("image_generation"));
    }
    void setImageGeneration(bool enabled)
    {
        setPermission(QStringLiteral("image_generation"), enabled);
    }

    std::optional<bool> mcp() const { return permission(QStringLiteral("mcp")); }
    void setMcp(bool enabled) { setPermission(QStringLiteral("mcp"), enabled); }

    std::optional<bool> codeInterpreter() const
    {
        return permission(QStringLiteral("code_interpreter"));
    }
    void setCodeInterpreter(bool enabled)
    {
        setPermission(QStringLiteral("code_interpreter"), enabled);
    }

    // The wire names the accessors above cover. Walking permissions() and asking
    // this is how a caller finds a tool the API gained after this build -- the
    // alternative being to hard-code the same five names again at the call site,
    // which then goes stale on its own schedule.
    static QStringList knownTools();
    static bool isKnownTool(const QString &tool) { return knownTools().contains(tool); }

    QJsonObject toJson() const;
    static ProjectHostedToolPermissions fromJson(const QJsonObject &json);

    bool operator==(const ProjectHostedToolPermissions &other) const;
    bool operator!=(const ProjectHostedToolPermissions &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ProjectHostedToolPermissionsData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ProjectModelPermissions)
Q_DECLARE_SHARED(QtOpenAi::Core::ProjectHostedToolPermissions)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectModelPermissions)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectHostedToolPermissions)
