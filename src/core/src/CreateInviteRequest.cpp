// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateInviteRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

#include <utility>

namespace QtOpenAi {
namespace Core {

class CreateInviteRequestData : public QSharedData
{
public:
    QString email;
    QString role;
    QList<InviteProject> projects;
};

CreateInviteRequest::CreateInviteRequest()
    : d(new CreateInviteRequestData)
{ }

CreateInviteRequest::CreateInviteRequest(QString email, QString role, QList<InviteProject> projects)
    : d(new CreateInviteRequestData)
{
    d->email = std::move(email);
    d->role = std::move(role);
    d->projects = std::move(projects);
}

CreateInviteRequest::CreateInviteRequest(const CreateInviteRequest &other) = default;
CreateInviteRequest::CreateInviteRequest(CreateInviteRequest &&other) noexcept = default;
CreateInviteRequest &CreateInviteRequest::operator=(const CreateInviteRequest &other) = default;
CreateInviteRequest &CreateInviteRequest::operator=(CreateInviteRequest &&other) noexcept = default;
CreateInviteRequest::~CreateInviteRequest() = default;

QString CreateInviteRequest::email() const { return d->email; }
void CreateInviteRequest::setEmail(const QString &email) { d->email = email; }

QString CreateInviteRequest::role() const { return d->role; }
void CreateInviteRequest::setRole(const QString &role) { d->role = role; }

QList<InviteProject> CreateInviteRequest::projects() const { return d->projects; }
void CreateInviteRequest::setProjects(const QList<InviteProject> &projects)
{
    d->projects = projects;
}

void CreateInviteRequest::addProject(const QString &projectId, const QString &role)
{
    d->projects.append(InviteProject {projectId, role});
}

QJsonObject CreateInviteRequest::toJson() const
{
    QJsonObject json;
    // Written even when empty, unlike the optional fields elsewhere: both are
    // required, and a request that omits one should come back as the server's
    // error about a missing field rather than as a silently different invite.
    json.insert(QStringLiteral("email"), d->email);
    json.insert(QStringLiteral("role"), d->role);
    if (!d->projects.isEmpty()) {
        QJsonArray projects;
        for (const InviteProject &project : d->projects)
            projects.append(project.toJson());
        json.insert(QStringLiteral("projects"), projects);
    }
    return json;
}

bool CreateInviteRequest::operator==(const CreateInviteRequest &other) const
{
    return d->email == other.d->email && d->role == other.d->role
           && d->projects == other.d->projects;
}

} // namespace Core
} // namespace QtOpenAi
