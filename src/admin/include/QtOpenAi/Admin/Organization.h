// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/CertificateReply.h>
#include <QtOpenAi/Admin/CostsReply.h>
#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Admin/GroupReply.h>
#include <QtOpenAi/Admin/InviteReply.h>
#include <QtOpenAi/Admin/ProjectApiKeyReply.h>
#include <QtOpenAi/Admin/ProjectRateLimitReply.h>
#include <QtOpenAi/Admin/ProjectReply.h>
#include <QtOpenAi/Admin/ProjectServiceAccountReply.h>
#include <QtOpenAi/Admin/RoleReply.h>
#include <QtOpenAi/Admin/RoleScope.h>
#include <QtOpenAi/Admin/UsageQuery.h>
#include <QtOpenAi/Admin/UsageReply.h>
#include <QtOpenAi/Admin/UserReply.h>
#include <QtOpenAi/Client/ListParams.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/CreateInviteRequest.h>
#include <QtOpenAi/Core/RoleRequest.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

namespace QtOpenAi {

namespace Client {
class Client;
class Interceptor;
class RateLimiter;
} // namespace Client

namespace Admin {

class OrganizationPrivate;

// The `/organization` surface: usage and costs, users and invites, projects,
// roles, certificates, admin keys, audit logs.
//
//     Admin::Organization organization(baseUrl, adminKey);
//     Admin::ProjectListReply *reply = organization.listProjects();
//
// **It is a separate object because it takes a separate credential.** The
// administration endpoints use an *admin* API key, which can archive a project
// or revoke a colleague's access; a standard key can only spend money asking
// questions. Hanging these endpoints off Client::Client would have put a
// credential of that reach on the same object an application uses to answer a
// user's question, and nothing would then stop the admin key from being sent to
// /chat/completions. Two objects, two keys, and the compiler keeps them apart.
//
// **It is not a second networking stack.** Requests go through a
// Client::Client this object owns, using that class's documented request path
// (planRequest/adoptReply), so the administration surface gets the same retry
// policy, interceptor chain -- including the credential redaction in
// LoggingInterceptor -- and rate limiter as everything else, from one
// implementation rather than a copy. What this class adds is the endpoints, the
// credential, and the refusal to expose the rest of Client::Client's API.
//
// The inner client is not reachable from here on purpose: handing it out would
// give a caller back exactly the ability this class exists to remove.
class QTOPENAI_ADMIN_EXPORT Organization : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString adminKey READ adminKey WRITE setAdminKey NOTIFY adminKeyChanged)
public:
    explicit Organization(QObject *parent = nullptr);
    // Construct with an API root and an admin key in one step.
    Organization(QUrl baseUrl, QString adminKey, QObject *parent = nullptr);
    ~Organization() override;

    // The API root, e.g. https://api.openai.com/v1. Endpoint paths such as
    // "/organization/projects" are appended to it.
    QUrl baseUrl() const;
    void setBaseUrl(const QUrl &baseUrl);

    // The **admin** key, not a standard one. Sent as `Authorization: Bearer`.
    QString adminKey() const;
    void setAdminKey(const QString &adminKey);

    Client::RetryPolicy retryPolicy() const;
    void setRetryPolicy(const Client::RetryPolicy &policy);

    // The same interceptor and rate-limiter hooks a Client::Client has; neither
    // is owned. Worth installing here in particular: an admin key in a log is a
    // worse accident than a standard one, and LoggingInterceptor already
    // redacts credentials.
    void addInterceptor(Client::Interceptor *interceptor);
    void removeInterceptor(Client::Interceptor *interceptor);
    void setRateLimiter(Client::RateLimiter *limiter);

    // --- Projects (/organization/projects) ---------------------------------
    // One page of projects. `includeArchived` asks the server for archived
    // projects as well as active ones; they are left out by default, which is
    // what an administration UI shows first.
    ProjectListReply *listProjects(const Client::ListParams &params = {},
                                   bool includeArchived = false);

    ProjectReply *getProject(const QString &projectId);
    ProjectReply *createProject(const QString &name);
    ProjectReply *modifyProject(const QString &projectId, const QString &name);

    // **Archiving is a POST, not a DELETE**, and this method is named for what
    // it does rather than for what it looks like. A project is what usage and
    // cost records point at, so the API has no way to remove one: archiving sets
    // `status` to "archived" and stamps `archivedAt`, and the billing history
    // keeps explaining itself. There is deliberately no deleteProject().
    ProjectReply *archiveProject(const QString &projectId);

    // --- Project members (/organization/projects/{id}/users) ----------------
    // A project member is the same six fields as an organization member, so it
    // is the same Core::OrganizationUser -- with one difference worth knowing:
    // `role` here is the *project* role, "owner" or "member", not the
    // organization's "owner" or "reader". Both are free strings, so one class
    // serves both rather than a copy that differs only in a comment.
    UserListReply *listProjectUsers(const QString &projectId,
                                    const Client::ListParams &params = {});

    UserReply *getProjectUser(const QString &projectId, const QString &userId);

    // Add an existing organization member to the project. It cannot invite: the
    // person must already be in the organization, which is what createInvite()
    // is for.
    UserReply *createProjectUser(const QString &projectId, const QString &userId,
                                 const QString &role);

    UserReply *modifyProjectUserRole(const QString &projectId, const QString &userId,
                                     const QString &role);

    // Remove someone from the project. They stay in the organization.
    UserReply *deleteProjectUser(const QString &projectId, const QString &userId);

    // --- Service accounts (/organization/projects/{id}/service_accounts) -----
    ProjectServiceAccountListReply *
    listProjectServiceAccounts(const QString &projectId, const Client::ListParams &params = {});

    ProjectServiceAccountReply *getProjectServiceAccount(const QString &projectId,
                                                         const QString &serviceAccountId);

    // **The reply carries the only copy of the new key's secret.** See
    // Core::ServiceAccountApiKey: no later read returns it.
    ProjectServiceAccountReply *createProjectServiceAccount(const QString &projectId,
                                                            const QString &name);

    ProjectServiceAccountReply *deleteProjectServiceAccount(const QString &projectId,
                                                            const QString &serviceAccountId);

    // --- API keys (/organization/projects/{id}/api_keys) --------------------
    // Read and revoke only. There is no endpoint that creates a project API key
    // here, and what is read back is redacted -- see Core::ProjectApiKey.
    ProjectApiKeyListReply *listProjectApiKeys(const QString &projectId,
                                               const Client::ListParams &params = {});

    ProjectApiKeyReply *getProjectApiKey(const QString &projectId, const QString &keyId);

    ProjectApiKeyReply *deleteProjectApiKey(const QString &projectId, const QString &keyId);

    // --- Rate limits (/organization/projects/{id}/rate_limits) --------------
    ProjectRateLimitListReply *listProjectRateLimits(const QString &projectId,
                                                     const Client::ListParams &params = {});

    // A **partial** update: only the limits set on `limits` are sent, so an
    // unset one is left alone rather than reset. See Core::ProjectRateLimit.
    ProjectRateLimitReply *modifyProjectRateLimit(const QString &projectId,
                                                  const QString &rateLimitId,
                                                  const Core::ProjectRateLimit &limits);

    // --- Roles (/organization/roles, /projects/{id}/roles) ------------------
    // A role is a named set of permissions. Every method here takes a RoleScope
    // saying whether it means the organization's roles or one project's,
    // defaulting to the organization -- **one set of methods, not two**, because
    // the two scopes send and receive the same payloads and differ only in the
    // path. See RoleScope, which also explains why a project's roles are not
    // where a project's other sub-resources are.
    RoleListReply *listRoles(const RoleScope &scope = {}, const Client::ListParams &params = {});

    RoleReply *getRole(const QString &roleId, const RoleScope &scope = {});

    // Create a custom role. `request` needs at least a name and a set of
    // permissions -- see Core::RoleRequest.
    RoleReply *createRole(const Core::RoleRequest &request, const RoleScope &scope = {});

    // A **partial** update: only the fields set on `request` are sent, so an
    // unset one is left alone rather than cleared. Predefined roles cannot be
    // changed; see Core::OrganizationRole::predefinedRole().
    RoleReply *modifyRole(const QString &roleId, const Core::RoleRequest &request,
                          const RoleScope &scope = {});

    // The acknowledgement decodes into the same Core::OrganizationRole,
    // reporting the object as "role.deleted".
    RoleReply *deleteRole(const QString &roleId, const RoleScope &scope = {});

    // --- Role assignments (.../groups/{id}/roles, .../users/{id}/roles) -----
    // Which roles a principal holds, and granting or revoking one. Both
    // principals and both scopes: four families that are one composed path and
    // one set of payloads apart -- see RoleScope and Core::RoleAssignment.
    //
    // A listed role carries its provenance: a role a group gave a user reports
    // that group in Core::OrganizationRole::assignmentSources(), and revoking
    // it from the user does nothing. Check isInherited() before offering the
    // button.
    RoleListReply *listGroupRoles(const QString &groupId, const RoleScope &scope = {},
                                  const Client::ListParams &params = {});

    RoleReply *getGroupRole(const QString &groupId, const QString &roleId,
                            const RoleScope &scope = {});

    RoleAssignmentReply *assignGroupRole(const QString &groupId, const QString &roleId,
                                         const RoleScope &scope = {});

    RoleAssignmentReply *unassignGroupRole(const QString &groupId, const QString &roleId,
                                           const RoleScope &scope = {});

    RoleListReply *listUserRoles(const QString &userId, const RoleScope &scope = {},
                                 const Client::ListParams &params = {});

    RoleReply *getUserRole(const QString &userId, const QString &roleId,
                           const RoleScope &scope = {});

    RoleAssignmentReply *assignUserRole(const QString &userId, const QString &roleId,
                                        const RoleScope &scope = {});

    RoleAssignmentReply *unassignUserRole(const QString &userId, const QString &roleId,
                                          const RoleScope &scope = {});

    // --- Groups (/organization/groups) --------------------------------------
    // Groups exist at organization scope only: there is no project-scoped group
    // catalogue, only the groups a project grants access to (below). That is why
    // these take no RoleScope.
    GroupListReply *listGroups(const Client::ListParams &params = {});

    GroupReply *getGroup(const QString &groupId);

    GroupReply *createGroup(const QString &name);

    GroupReply *modifyGroup(const QString &groupId, const QString &name);

    // Unlike a project, a group really is deleted. The acknowledgement decodes
    // into the same Core::Group, reporting the object as "group.deleted".
    GroupReply *deleteGroup(const QString &groupId);

    // --- Group members (/organization/groups/{id}/users) --------------------
    // A SCIM-managed group's membership comes from an identity provider, and a
    // change made here is undone by the next sync -- check
    // Core::Group::isScimManaged() first.
    GroupMemberListReply *listGroupUsers(const QString &groupId,
                                         const Client::ListParams &params = {});

    GroupMemberReply *getGroupUser(const QString &groupId, const QString &userId);

    // Add an existing organization member to the group. As with a project, it
    // cannot invite: the person must already be in the organization.
    //
    // The reply is an acknowledgement rather than the member -- the API answers
    // with the two ids and nothing else. See Core::GroupMembership.
    GroupMembershipReply *addGroupUser(const QString &groupId, const QString &userId);

    // Remove someone from the group. They stay in the organization.
    GroupMembershipReply *removeGroupUser(const QString &groupId, const QString &userId);

    // --- Project groups (/organization/projects/{id}/groups) ----------------
    // Which groups have access to a project. Note the path: these *are* under
    // /organization/projects, where a project's roles are not.
    ProjectGroupListReply *listProjectGroups(const QString &projectId,
                                             const Client::ListParams &params = {});

    ProjectGroupReply *getProjectGroup(const QString &projectId, const QString &groupId);

    // Grant a group access to a project with a project role. `roleId` is the id
    // of a role from listRoles(RoleScope::project(projectId)), not a role name.
    ProjectGroupReply *addProjectGroup(const QString &projectId, const QString &groupId,
                                       const QString &roleId);

    ProjectGroupReply *removeProjectGroup(const QString &projectId, const QString &groupId);

    // --- Certificates (/organization/certificates) --------------------------
    // Client certificates, uploaded once to the organization and then switched
    // on per scope. **Unlike roles, these are not one set of methods with a
    // scope argument**: only listing and the activation toggles exist at both
    // scopes. A certificate is uploaded, read, renamed and deleted at
    // organization scope alone, so a scope parameter on those would be a
    // parameter with one legal value.
    CertificateListReply *listCertificates(const Client::ListParams &params = {});

    // The certificates a project has switched on. Note the path: these *are*
    // under /organization/projects, where a project's roles are not.
    CertificateListReply *listProjectCertificates(const QString &projectId,
                                                  const Client::ListParams &params = {});

    // Upload a certificate. `pemContent` is the PEM body; `name` is optional
    // and the API accepts a certificate without one.
    CertificateReply *uploadCertificate(const QString &pemContent, const QString &name = {});

    // One certificate by id. `includeContent` asks for the PEM body as well,
    // which is left out by default -- see Core::Certificate::pemContent().
    //
    // The reply's `active` is unset here whatever the certificate's state:
    // activation belongs to a scope and this read has none.
    CertificateReply *getCertificate(const QString &certificateId, bool includeContent = false);

    // Rename a certificate. Named for the one thing the request body can
    // change, as modifyUserRole() is.
    CertificateReply *modifyCertificate(const QString &certificateId, const QString &name);

    // The acknowledgement decodes into the same Core::Certificate, reporting the
    // object as "certificate.deleted".
    CertificateReply *deleteCertificate(const QString &certificateId);

    // --- Activation (.../certificates/activate, .../deactivate) -------------
    // **These take a batch, not a certificate.** Both are a POST to a path
    // ending in the verb, carrying `certificate_ids` -- there is no
    // POST /organization/certificates/{id}/activate, which is the shape the
    // names suggest. The API accepts one to ten ids per call and answers with
    // the certificates it changed, so the reply is a list.
    //
    //     organization.activateCertificates({certificateId});   // a batch of one
    CertificateListReply *activateCertificates(const QStringList &certificateIds);

    CertificateListReply *deactivateCertificates(const QStringList &certificateIds);

    CertificateListReply *activateProjectCertificates(const QString &projectId,
                                                      const QStringList &certificateIds);

    CertificateListReply *deactivateProjectCertificates(const QString &projectId,
                                                        const QStringList &certificateIds);

    // --- Usage and costs (/organization/usage/*, /organization/costs) ------
    // Which usage report to ask for. The ten endpoints under
    // /organization/usage differ only in their last path segment and in which
    // counters a row carries, so they are one method with an enumerator rather
    // than ten identically-shaped methods: the list of endpoints then lives in
    // one place, and adding the next one is an enumerator and a path.
    enum class UsageKind {
        Completions,
        Embeddings,
        Images,
        Moderations,
        AudioSpeeches,
        AudioTranscriptions,
        VectorStores,
        CodeInterpreterSessions,
        FileSearchCalls,
        WebSearchCalls,
    };
    Q_ENUM(UsageKind)

    // One page of usage buckets. `query.startTime` is required by the API; see
    // UsageQuery for which of its filters each report honours.
    UsageReply *usage(UsageKind kind, const UsageQuery &query);

    // One page of cost buckets. Same query type, and it honours `startTime`,
    // `endTime`, `bucketWidth`, `limit`, `page`, `projectIds` and a `groupBy` of
    // "project_id" and/or "line_item".
    CostsReply *costs(const UsageQuery &query);

    // --- Members (/organization/users) -------------------------------------
    // One page of the organization's members. `emails` restricts the page to
    // those addresses, which is how an administration UI answers "is this
    // person already in?" without walking every page.
    UserListReply *listUsers(const Client::ListParams &params = {}, const QStringList &emails = {});

    UserReply *getUser(const QString &userId);

    // Change a member's organization role ("owner" or "reader"). Named for the
    // one thing POST /organization/users/{id} can change, rather than a generic
    // "modify": the request body has exactly one field.
    UserReply *modifyUserRole(const QString &userId, const QString &role);

    // Remove a member from the organization. The acknowledgement decodes into
    // the same OrganizationUser, reporting the object as
    // "organization.user.deleted".
    UserReply *deleteUser(const QString &userId);

    // --- Invitations (/organization/invites) --------------------------------
    InviteListReply *listInvites(const Client::ListParams &params = {});

    InviteReply *getInvite(const QString &inviteId);

    // Invite someone to the organization. Sending an invitation is what adds a
    // member: there is no endpoint that creates an OrganizationUser directly.
    InviteReply *createInvite(const Core::CreateInviteRequest &request);

    // Withdraw a pending invitation. This does not remove a member who has
    // already accepted — that is deleteUser().
    InviteReply *deleteInvite(const QString &inviteId);

Q_SIGNALS:
    void baseUrlChanged();
    void adminKeyChanged();

    // Every reply this object creates, announced the moment it exists — the
    // same hook Client::Client offers, so a MetricsCollector can watch the
    // administration surface too.
    void replyCreated(QObject *reply);

private:
    Q_DECLARE_PRIVATE(Organization)
    QScopedPointer<OrganizationPrivate> d_ptr;
};

} // namespace Admin
} // namespace QtOpenAi
