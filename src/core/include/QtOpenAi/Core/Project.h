// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ProjectData;

// A project within an organization (GET/POST /organization/projects,
// GET/POST /organization/projects/{id}).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// **A project is archived, never deleted.** `POST .../archive` sets `status` to
// "archived" and stamps `archived_at`; there is no DELETE. That is the API's
// design and not an omission here: a project is what usage and cost records
// point at, so removing one would orphan the billing history it explains.
class QTOPENAI_CORE_EXPORT Project
{
public:
    Project();
    Project(const Project &other);
    Project(Project &&other) noexcept;
    Project &operator=(const Project &other);
    Project &operator=(Project &&other) noexcept;
    ~Project();

    void swap(Project &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "organization.project".
    QString object() const;
    void setObject(const QString &object);

    QString name() const;
    void setName(const QString &name);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // Unix timestamp of the archival, or 0 while the project is active. The API
    // sends null for an active project, which reads back as 0 here.
    qint64 archivedAt() const;
    void setArchivedAt(qint64 archivedAt);

    // "active" or "archived". Kept as the string the server sent rather than an
    // enum: this is the one field the administration surface is most likely to
    // grow a new value for, and an unknown status must survive a round trip
    // rather than decay to the first enumerator.
    QString status() const;
    void setStatus(const QString &status);

    bool isArchived() const { return status() == QLatin1String("archived"); }

    QJsonObject toJson() const;
    static Project fromJson(const QJsonObject &json);

    bool operator==(const Project &other) const;
    bool operator!=(const Project &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ProjectData> d;
};

// A `list` of projects (GET /organization/projects). Cursor-paginated; reuses
// the shared list-page type.
using ProjectList = ListPage<Project>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Project)
Q_DECLARE_METATYPE(QtOpenAi::Core::Project)
Q_DECLARE_METATYPE(QtOpenAi::Core::ProjectList)
