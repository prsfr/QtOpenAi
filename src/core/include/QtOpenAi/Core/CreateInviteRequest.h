// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Invite.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class CreateInviteRequestData;

// The body of a POST /organization/invites request.
//
// `email` and `role` are required by the API; `projects` is optional and grants
// the invited person access to those projects, each with its own project role —
// see InviteProject for why that role is not the organization one.
class QTOPENAI_CORE_EXPORT CreateInviteRequest
{
public:
    CreateInviteRequest();
    CreateInviteRequest(QString email, QString role, QList<InviteProject> projects = {});
    CreateInviteRequest(const CreateInviteRequest &other);
    CreateInviteRequest(CreateInviteRequest &&other) noexcept;
    CreateInviteRequest &operator=(const CreateInviteRequest &other);
    CreateInviteRequest &operator=(CreateInviteRequest &&other) noexcept;
    ~CreateInviteRequest();

    void swap(CreateInviteRequest &other) noexcept { d.swap(other.d); }

    QString email() const;
    void setEmail(const QString &email);

    // The organization role to grant, "owner" or "reader".
    QString role() const;
    void setRole(const QString &role);

    QList<InviteProject> projects() const;
    void setProjects(const QList<InviteProject> &projects);
    void addProject(const QString &projectId, const QString &role);

    QJsonObject toJson() const;

    bool operator==(const CreateInviteRequest &other) const;
    bool operator!=(const CreateInviteRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateInviteRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateInviteRequest)
