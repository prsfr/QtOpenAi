// SPDX-License-Identifier: MIT
#include "QtOpenAi/Admin/Organization.h"

#include <QtOpenAi/Client/Client.h>

#include "JsonHelpers_p.h"
#include "RestPath_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Admin {

namespace {

// The collections this module's endpoint families hang off. Spelled once, as
// the endpoint paths in Client are.
constexpr QLatin1String kProjects("/organization/projects");
constexpr QLatin1String kUsage("/organization/usage/");
constexpr QLatin1String kCosts("/organization/costs");
constexpr QLatin1String kUsers("/organization/users");
constexpr QLatin1String kInvites("/organization/invites");
constexpr QLatin1String kAdminApiKeys("/organization/admin_api_keys");
constexpr QLatin1String kAuditLogs("/organization/audit_logs");
constexpr QLatin1String kSpendAlerts("/organization/spend_alerts");
constexpr QLatin1String kDataRetention("/organization/data_retention");
constexpr QLatin1String kSpendLimit("/organization/spend_limit");

// The sub-resources that hang off a project, a group, or a scope. Every one of
// them repeats the same list/create/get/delete shape under an id, so they are
// segments composed onto a collection rather than a full path each.
constexpr QLatin1String kUsersSegment("/users");
constexpr QLatin1String kGroupsSegment("/groups");
constexpr QLatin1String kRolesSegment("/roles");
constexpr QLatin1String kCertificatesSegment("/certificates");
constexpr QLatin1String kServiceAccounts("/service_accounts");
constexpr QLatin1String kApiKeys("/api_keys");
constexpr QLatin1String kRateLimits("/rate_limits");
constexpr QLatin1String kModelPermissions("/model_permissions");
constexpr QLatin1String kHostedToolPermissions("/hosted_tool_permissions");
constexpr QLatin1String kSpendAlertsSegment("/spend_alerts");
constexpr QLatin1String kDataRetentionSegment("/data_retention");
constexpr QLatin1String kSpendLimitSegment("/spend_limit");
constexpr QLatin1String kArchive("/archive");

// The activation toggles are path segments rather than a verb on one
// certificate -- see Organization::activateCertificates().
constexpr QLatin1String kActivate("/activate");
constexpr QLatin1String kDeactivate("/deactivate");

// The two roots a role path hangs off. **A project's roles are not under
// /organization/projects**, where its groups, users and keys are: the API serves
// them from /projects/{id}/roles, in its path table and in its own curl examples
// alike. Spelling the two roots here is what keeps that quirk in one place --
// see Admin::RoleScope.
constexpr QLatin1String kOrganizationRoot("/organization");
constexpr QLatin1String kProjectsRoot("/projects");

// The same helper Client.cpp composes its nested paths with -- literally the
// same one now; see RestPath_p.h. The endpoint methods below stay free of
// string arithmetic, and every path is built from the constants above rather
// than retyped.
using Rest::resourcePath;

// And the request-body serialiser Client uses; see JsonHelpers_p.h.
using Core::detail::compactJson;

// A member of a project's sub-collection, which is two levels of the above:
// ("proj_1", "/api_keys", "key_1") -> "/organization/projects/proj_1/api_keys/key_1".
QString projectPath(const QString &projectId, QLatin1String collection, const QString &id = {})
{
    return resourcePath(kProjects, projectId, resourcePath(collection, id));
}

// A member of the organization's group collection, and optionally something
// below it: ("group_1", "/users", "user_1") ->
// "/organization/groups/group_1/users/user_1".
QString groupPath(const QString &groupId = {}, QLatin1String collection = {},
                  const QString &id = {})
{
    return QString(kOrganizationRoot)
           + resourcePath(kGroupsSegment, groupId, resourcePath(collection, id));
}

// Where a role family hangs off: the organization, or one project.
QString scopeRoot(const RoleScope &scope)
{
    return scope.isOrganization() ? QString(kOrganizationRoot)
                                  : resourcePath(kProjectsRoot, scope.projectId());
}

// A scope's role catalogue, and one role of it.
QString rolePath(const RoleScope &scope, const QString &roleId = {})
{
    return scopeRoot(scope) + resourcePath(kRolesSegment, roleId);
}

// The roles a principal holds within a scope, and one of those. `principals` is
// kGroupsSegment or kUsersSegment: the four assignment families -- two
// principals times two scopes -- differ in nothing but those two choices, so one
// rule composes every path they have between them.
QString assignedRolePath(const RoleScope &scope, QLatin1String principals,
                         const QString &principalId, const QString &roleId = {})
{
    return scopeRoot(scope) + resourcePath(principals, principalId)
           + resourcePath(kRolesSegment, roleId);
}

// The organization's certificate collection, and one certificate or one of the
// activation verbs below it.
QString certificatePath(const QString &id = {}, const QString &suffix = {})
{
    return QString(kOrganizationRoot) + resourcePath(kCertificatesSegment, id, suffix);
}

// The batch body both activation toggles take, at either scope: a list of ids,
// never a single one.
QJsonObject certificateIdsBody(const QStringList &certificateIds)
{
    QJsonObject body;
    body.insert(QStringLiteral("certificate_ids"), QJsonArray::fromStringList(certificateIds));
    return body;
}

// The last path segment of each usage report. A switch rather than a table
// indexed by the enumerator, so that adding an enumerator without a path is a
// compiler warning rather than a lookup past the end of an array. Completions
// falls out of the switch, as the default verb does in Client::planRequest.
QLatin1String usageSegment(Organization::UsageKind kind)
{
    switch (kind) {
    case Organization::UsageKind::Completions:
        break;
    case Organization::UsageKind::Embeddings:
        return QLatin1String("embeddings");
    case Organization::UsageKind::Images:
        return QLatin1String("images");
    case Organization::UsageKind::Moderations:
        return QLatin1String("moderations");
    case Organization::UsageKind::AudioSpeeches:
        return QLatin1String("audio_speeches");
    case Organization::UsageKind::AudioTranscriptions:
        return QLatin1String("audio_transcriptions");
    case Organization::UsageKind::VectorStores:
        return QLatin1String("vector_stores");
    case Organization::UsageKind::CodeInterpreterSessions:
        return QLatin1String("code_interpreter_sessions");
    case Organization::UsageKind::FileSearchCalls:
        return QLatin1String("file_search_calls");
    case Organization::UsageKind::WebSearchCalls:
        return QLatin1String("web_search_calls");
    }
    return QLatin1String("completions");
}

} // namespace

class OrganizationPrivate
{
public:
    // Owned outright rather than taken from the caller: an Organization that
    // shared a client with the application would share its credential, which is
    // the one thing this class exists to prevent.
    Client::Client client;
};

Organization::Organization(QObject *parent)
    : QObject(parent)
    , d_ptr(new OrganizationPrivate)
{
    Q_D(Organization);
    // Forwarded rather than re-emitted from each endpoint method, so a reply
    // added later is announced without anyone remembering to do it.
    connect(&d->client, &Client::Client::replyCreated, this, &Organization::replyCreated);
}

Organization::Organization(QUrl baseUrl, QString adminKey, QObject *parent)
    : Organization(parent)
{
    setBaseUrl(baseUrl);
    setAdminKey(adminKey);
}

Organization::~Organization() = default;

QUrl Organization::baseUrl() const
{
    Q_D(const Organization);
    return d->client.baseUrl();
}

void Organization::setBaseUrl(const QUrl &baseUrl)
{
    Q_D(Organization);
    if (d->client.baseUrl() == baseUrl)
        return;
    d->client.setBaseUrl(baseUrl);
    Q_EMIT baseUrlChanged();
}

QString Organization::adminKey() const
{
    Q_D(const Organization);
    return d->client.apiKey();
}

void Organization::setAdminKey(const QString &adminKey)
{
    Q_D(Organization);
    if (d->client.apiKey() == adminKey)
        return;
    d->client.setApiKey(adminKey);
    Q_EMIT adminKeyChanged();
}

Client::RetryPolicy Organization::retryPolicy() const
{
    Q_D(const Organization);
    return d->client.retryPolicy();
}

void Organization::setRetryPolicy(const Client::RetryPolicy &policy)
{
    Q_D(Organization);
    d->client.setRetryPolicy(policy);
}

void Organization::addInterceptor(Client::Interceptor *interceptor)
{
    Q_D(Organization);
    d->client.addInterceptor(interceptor);
}

void Organization::removeInterceptor(Client::Interceptor *interceptor)
{
    Q_D(Organization);
    d->client.removeInterceptor(interceptor);
}

void Organization::setRateLimiter(Client::RateLimiter *limiter)
{
    Q_D(Organization);
    d->client.setRateLimiter(limiter);
}

ProjectListReply *Organization::listProjects(const Client::ListParams &params, bool includeArchived)
{
    Q_D(Organization);
    QUrlQuery query = params.toQuery();
    // Sent only when asked for: the parameter's absence and `false` mean the
    // same thing to the server, and a query string that says nothing extra is
    // easier to read in a log.
    if (includeArchived)
        query.addQueryItem(QStringLiteral("include_archived"), QStringLiteral("true"));

    return d->client.issueRequest<ProjectListReply>(Client::Client::Verb::Get, kProjects, query);
}

ProjectReply *Organization::getProject(const QString &projectId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectReply>(Client::Client::Verb::Get,
                                                resourcePath(kProjects, projectId));
}

ProjectReply *Organization::createProject(const QString &name)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("name"), name);
    return d->client.issueRequest<ProjectReply>(Client::Client::Verb::Post, kProjects, {},
                                                compactJson(body));
}

ProjectReply *Organization::modifyProject(const QString &projectId, const QString &name)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("name"), name);
    return d->client.issueRequest<ProjectReply>(
            Client::Client::Verb::Post, resourcePath(kProjects, projectId), {}, compactJson(body));
}

ProjectReply *Organization::archiveProject(const QString &projectId)
{
    Q_D(Organization);
    // A POST with no body, not a DELETE: archiving changes the project's status
    // rather than removing it, because usage and cost records point at it. See
    // the declaration.
    return d->client.issueRequest<ProjectReply>(Client::Client::Verb::Post,
                                                resourcePath(kProjects, projectId, kArchive));
}

UserListReply *Organization::listProjectUsers(const QString &projectId,
                                              const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<UserListReply>(
            Client::Client::Verb::Get, projectPath(projectId, kUsersSegment), params.toQuery());
}

UserReply *Organization::getProjectUser(const QString &projectId, const QString &userId)
{
    Q_D(Organization);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Get,
                                             projectPath(projectId, kUsersSegment, userId));
}

UserReply *Organization::createProjectUser(const QString &projectId, const QString &userId,
                                           const QString &role)
{
    Q_D(Organization);
    // The id goes in the body rather than the path: this adds an existing
    // organization member to the project, it does not create a person.
    QJsonObject body;
    body.insert(QStringLiteral("user_id"), userId);
    body.insert(QStringLiteral("role"), role);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Post,
                                             projectPath(projectId, kUsersSegment), {},
                                             compactJson(body));
}

UserReply *Organization::modifyProjectUserRole(const QString &projectId, const QString &userId,
                                               const QString &role)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("role"), role);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Post,
                                             projectPath(projectId, kUsersSegment, userId), {},
                                             compactJson(body));
}

UserReply *Organization::deleteProjectUser(const QString &projectId, const QString &userId)
{
    Q_D(Organization);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Delete,
                                             projectPath(projectId, kUsersSegment, userId));
}

ProjectServiceAccountListReply *
Organization::listProjectServiceAccounts(const QString &projectId, const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectServiceAccountListReply>(
            Client::Client::Verb::Get, projectPath(projectId, kServiceAccounts), params.toQuery());
}

ProjectServiceAccountReply *Organization::getProjectServiceAccount(const QString &projectId,
                                                                   const QString &serviceAccountId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectServiceAccountReply>(
            Client::Client::Verb::Get, projectPath(projectId, kServiceAccounts, serviceAccountId));
}

ProjectServiceAccountReply *Organization::createProjectServiceAccount(const QString &projectId,
                                                                      const QString &name)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("name"), name);
    return d->client.issueRequest<ProjectServiceAccountReply>(
            Client::Client::Verb::Post, projectPath(projectId, kServiceAccounts), {},
            compactJson(body));
}

ProjectServiceAccountReply *
Organization::deleteProjectServiceAccount(const QString &projectId, const QString &serviceAccountId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectServiceAccountReply>(
            Client::Client::Verb::Delete,
            projectPath(projectId, kServiceAccounts, serviceAccountId));
}

ProjectApiKeyListReply *Organization::listProjectApiKeys(const QString &projectId,
                                                         const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectApiKeyListReply>(
            Client::Client::Verb::Get, projectPath(projectId, kApiKeys), params.toQuery());
}

ProjectApiKeyReply *Organization::getProjectApiKey(const QString &projectId, const QString &keyId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectApiKeyReply>(Client::Client::Verb::Get,
                                                      projectPath(projectId, kApiKeys, keyId));
}

ProjectApiKeyReply *Organization::deleteProjectApiKey(const QString &projectId,
                                                      const QString &keyId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectApiKeyReply>(Client::Client::Verb::Delete,
                                                      projectPath(projectId, kApiKeys, keyId));
}

ProjectRateLimitListReply *Organization::listProjectRateLimits(const QString &projectId,
                                                               const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectRateLimitListReply>(
            Client::Client::Verb::Get, projectPath(projectId, kRateLimits), params.toQuery());
}

ProjectRateLimitReply *Organization::modifyProjectRateLimit(const QString &projectId,
                                                            const QString &rateLimitId,
                                                            const Core::ProjectRateLimit &limits)
{
    Q_D(Organization);
    // Only the limits the caller set: ProjectRateLimit::toJson() leaves the rest
    // out, so an unmentioned limit is untouched rather than zeroed. The id,
    // object and model it may carry from a previous read are dropped here --
    // they identify the limit rather than change it, and the id is already in
    // the path.
    QJsonObject body = limits.toJson();
    body.remove(QStringLiteral("id"));
    body.remove(QStringLiteral("object"));
    body.remove(QStringLiteral("model"));
    return d->client.issueRequest<ProjectRateLimitReply>(
            Client::Client::Verb::Post, projectPath(projectId, kRateLimits, rateLimitId), {},
            compactJson(body));
}

RoleListReply *Organization::listRoles(const RoleScope &scope, const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleListReply>(Client::Client::Verb::Get, rolePath(scope),
                                                 params.toQuery());
}

RoleReply *Organization::getRole(const QString &roleId, const RoleScope &scope)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleReply>(Client::Client::Verb::Get, rolePath(scope, roleId));
}

RoleReply *Organization::createRole(const Core::RoleRequest &request, const RoleScope &scope)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleReply>(Client::Client::Verb::Post, rolePath(scope), {},
                                             compactJson(request.toJson()));
}

RoleReply *Organization::modifyRole(const QString &roleId, const Core::RoleRequest &request,
                                    const RoleScope &scope)
{
    Q_D(Organization);
    // Only what the caller set: RoleRequest::toJson() leaves the rest out, so an
    // unmentioned field is untouched rather than cleared.
    return d->client.issueRequest<RoleReply>(Client::Client::Verb::Post, rolePath(scope, roleId),
                                             {}, compactJson(request.toJson()));
}

RoleReply *Organization::deleteRole(const QString &roleId, const RoleScope &scope)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleReply>(Client::Client::Verb::Delete, rolePath(scope, roleId));
}

RoleListReply *Organization::listGroupRoles(const QString &groupId, const RoleScope &scope,
                                            const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleListReply>(Client::Client::Verb::Get,
                                                 assignedRolePath(scope, kGroupsSegment, groupId),
                                                 params.toQuery());
}

RoleReply *Organization::getGroupRole(const QString &groupId, const QString &roleId,
                                      const RoleScope &scope)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleReply>(
            Client::Client::Verb::Get, assignedRolePath(scope, kGroupsSegment, groupId, roleId));
}

RoleAssignmentReply *Organization::assignGroupRole(const QString &groupId, const QString &roleId,
                                                   const RoleScope &scope)
{
    Q_D(Organization);
    // The role id goes in the body rather than the path: this creates the
    // assignment, and the path names the principal it is being created for.
    QJsonObject body;
    body.insert(QStringLiteral("role_id"), roleId);
    return d->client.issueRequest<RoleAssignmentReply>(
            Client::Client::Verb::Post, assignedRolePath(scope, kGroupsSegment, groupId), {},
            compactJson(body));
}

RoleAssignmentReply *Organization::unassignGroupRole(const QString &groupId, const QString &roleId,
                                                     const RoleScope &scope)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleAssignmentReply>(
            Client::Client::Verb::Delete, assignedRolePath(scope, kGroupsSegment, groupId, roleId));
}

RoleListReply *Organization::listUserRoles(const QString &userId, const RoleScope &scope,
                                           const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleListReply>(Client::Client::Verb::Get,
                                                 assignedRolePath(scope, kUsersSegment, userId),
                                                 params.toQuery());
}

RoleReply *Organization::getUserRole(const QString &userId, const QString &roleId,
                                     const RoleScope &scope)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleReply>(
            Client::Client::Verb::Get, assignedRolePath(scope, kUsersSegment, userId, roleId));
}

RoleAssignmentReply *Organization::assignUserRole(const QString &userId, const QString &roleId,
                                                  const RoleScope &scope)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("role_id"), roleId);
    return d->client.issueRequest<RoleAssignmentReply>(
            Client::Client::Verb::Post, assignedRolePath(scope, kUsersSegment, userId), {},
            compactJson(body));
}

RoleAssignmentReply *Organization::unassignUserRole(const QString &userId, const QString &roleId,
                                                    const RoleScope &scope)
{
    Q_D(Organization);
    return d->client.issueRequest<RoleAssignmentReply>(
            Client::Client::Verb::Delete, assignedRolePath(scope, kUsersSegment, userId, roleId));
}

GroupListReply *Organization::listGroups(const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<GroupListReply>(Client::Client::Verb::Get, groupPath(),
                                                  params.toQuery());
}

GroupReply *Organization::getGroup(const QString &groupId)
{
    Q_D(Organization);
    return d->client.issueRequest<GroupReply>(Client::Client::Verb::Get, groupPath(groupId));
}

GroupReply *Organization::createGroup(const QString &name)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("name"), name);
    return d->client.issueRequest<GroupReply>(Client::Client::Verb::Post, groupPath(), {},
                                              compactJson(body));
}

GroupReply *Organization::modifyGroup(const QString &groupId, const QString &name)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("name"), name);
    return d->client.issueRequest<GroupReply>(Client::Client::Verb::Post, groupPath(groupId), {},
                                              compactJson(body));
}

GroupReply *Organization::deleteGroup(const QString &groupId)
{
    Q_D(Organization);
    return d->client.issueRequest<GroupReply>(Client::Client::Verb::Delete, groupPath(groupId));
}

GroupMemberListReply *Organization::listGroupUsers(const QString &groupId,
                                                   const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<GroupMemberListReply>(
            Client::Client::Verb::Get, groupPath(groupId, kUsersSegment), params.toQuery());
}

GroupMemberReply *Organization::getGroupUser(const QString &groupId, const QString &userId)
{
    Q_D(Organization);
    return d->client.issueRequest<GroupMemberReply>(Client::Client::Verb::Get,
                                                    groupPath(groupId, kUsersSegment, userId));
}

GroupMembershipReply *Organization::addGroupUser(const QString &groupId, const QString &userId)
{
    Q_D(Organization);
    // The id goes in the body rather than the path, as adding someone to a
    // project does: this adds an existing organization member to the group, it
    // does not create a person.
    QJsonObject body;
    body.insert(QStringLiteral("user_id"), userId);
    return d->client.issueRequest<GroupMembershipReply>(
            Client::Client::Verb::Post, groupPath(groupId, kUsersSegment), {}, compactJson(body));
}

GroupMembershipReply *Organization::removeGroupUser(const QString &groupId, const QString &userId)
{
    Q_D(Organization);
    return d->client.issueRequest<GroupMembershipReply>(Client::Client::Verb::Delete,
                                                        groupPath(groupId, kUsersSegment, userId));
}

ProjectGroupListReply *Organization::listProjectGroups(const QString &projectId,
                                                       const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectGroupListReply>(
            Client::Client::Verb::Get, projectPath(projectId, kGroupsSegment), params.toQuery());
}

ProjectGroupReply *Organization::getProjectGroup(const QString &projectId, const QString &groupId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectGroupReply>(
            Client::Client::Verb::Get, projectPath(projectId, kGroupsSegment, groupId));
}

ProjectGroupReply *Organization::addProjectGroup(const QString &projectId, const QString &groupId,
                                                 const QString &roleId)
{
    Q_D(Organization);
    // `role` carries a role *id*, not a role name -- the API's field name is the
    // shorter one, and sending a name here is a 404 on a role that does not
    // exist rather than a validation error.
    QJsonObject body;
    body.insert(QStringLiteral("group_id"), groupId);
    body.insert(QStringLiteral("role"), roleId);
    return d->client.issueRequest<ProjectGroupReply>(Client::Client::Verb::Post,
                                                     projectPath(projectId, kGroupsSegment), {},
                                                     compactJson(body));
}

ProjectGroupReply *Organization::removeProjectGroup(const QString &projectId,
                                                    const QString &groupId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectGroupReply>(
            Client::Client::Verb::Delete, projectPath(projectId, kGroupsSegment, groupId));
}

CertificateListReply *Organization::listCertificates(const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<CertificateListReply>(Client::Client::Verb::Get,
                                                        certificatePath(), params.toQuery());
}

CertificateListReply *Organization::listProjectCertificates(const QString &projectId,
                                                            const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<CertificateListReply>(
            Client::Client::Verb::Get, projectPath(projectId, kCertificatesSegment),
            params.toQuery());
}

CertificateReply *Organization::uploadCertificate(const QString &pemContent, const QString &name)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("certificate"), pemContent);
    // Only when the caller supplied one: the API accepts a certificate with no
    // name, and an empty string is not the same request as an absent field.
    if (!name.isEmpty())
        body.insert(QStringLiteral("name"), name);
    return d->client.issueRequest<CertificateReply>(Client::Client::Verb::Post, certificatePath(),
                                                    {}, compactJson(body));
}

CertificateReply *Organization::getCertificate(const QString &certificateId, bool includeContent)
{
    Q_D(Organization);
    QUrlQuery query;
    // Sent only when asked for. The PEM body is the largest thing a certificate
    // carries and the endpoint leaves it out unless `include` names it.
    if (includeContent)
        query.addQueryItem(QStringLiteral("include[]"), QStringLiteral("content"));

    return d->client.issueRequest<CertificateReply>(Client::Client::Verb::Get,
                                                    certificatePath(certificateId), query);
}

CertificateReply *Organization::modifyCertificate(const QString &certificateId, const QString &name)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("name"), name);
    return d->client.issueRequest<CertificateReply>(
            Client::Client::Verb::Post, certificatePath(certificateId), {}, compactJson(body));
}

CertificateReply *Organization::deleteCertificate(const QString &certificateId)
{
    Q_D(Organization);
    return d->client.issueRequest<CertificateReply>(Client::Client::Verb::Delete,
                                                    certificatePath(certificateId));
}

CertificateListReply *Organization::activateCertificates(const QStringList &certificateIds)
{
    Q_D(Organization);
    // The verb is a path segment on the collection and the ids are the body --
    // there is no per-certificate activate endpoint. See the declaration.
    return d->client.issueRequest<CertificateListReply>(
            Client::Client::Verb::Post, certificatePath({}, kActivate), {},
            compactJson(certificateIdsBody(certificateIds)));
}

CertificateListReply *Organization::deactivateCertificates(const QStringList &certificateIds)
{
    Q_D(Organization);
    return d->client.issueRequest<CertificateListReply>(
            Client::Client::Verb::Post, certificatePath({}, kDeactivate), {},
            compactJson(certificateIdsBody(certificateIds)));
}

CertificateListReply *Organization::activateProjectCertificates(const QString &projectId,
                                                                const QStringList &certificateIds)
{
    Q_D(Organization);
    return d->client.issueRequest<CertificateListReply>(
            Client::Client::Verb::Post, projectPath(projectId, kCertificatesSegment) + kActivate,
            {}, compactJson(certificateIdsBody(certificateIds)));
}

CertificateListReply *Organization::deactivateProjectCertificates(const QString &projectId,
                                                                  const QStringList &certificateIds)
{
    Q_D(Organization);
    return d->client.issueRequest<CertificateListReply>(
            Client::Client::Verb::Post, projectPath(projectId, kCertificatesSegment) + kDeactivate,
            {}, compactJson(certificateIdsBody(certificateIds)));
}

ProjectModelPermissionsReply *Organization::getProjectModelPermissions(const QString &projectId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectModelPermissionsReply>(
            Client::Client::Verb::Get, projectPath(projectId, kModelPermissions));
}

ProjectModelPermissionsReply *
Organization::setProjectModelPermissions(const QString &projectId,
                                         const Core::ProjectModelPermissions &permissions)
{
    Q_D(Organization);
    // The policy is replaced whole, so both fields go every time -- unlike the
    // rate limits above, an omitted `mode` here is not "leave it alone" but a
    // request the server rejects. The object and the deletion flag a read may
    // have left on the value identify it rather than change it, so they are
    // dropped.
    QJsonObject body = permissions.toJson();
    body.remove(QStringLiteral("object"));
    body.remove(QStringLiteral("deleted"));
    return d->client.issueRequest<ProjectModelPermissionsReply>(
            Client::Client::Verb::Post, projectPath(projectId, kModelPermissions), {},
            compactJson(body));
}

ProjectModelPermissionsReply *Organization::deleteProjectModelPermissions(const QString &projectId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectModelPermissionsReply>(
            Client::Client::Verb::Delete, projectPath(projectId, kModelPermissions));
}

ProjectHostedToolPermissionsReply *
Organization::getProjectHostedToolPermissions(const QString &projectId)
{
    Q_D(Organization);
    return d->client.issueRequest<ProjectHostedToolPermissionsReply>(
            Client::Client::Verb::Get, projectPath(projectId, kHostedToolPermissions));
}

ProjectHostedToolPermissionsReply *
Organization::setProjectHostedToolPermissions(const QString &projectId,
                                              const Core::ProjectHostedToolPermissions &permissions)
{
    Q_D(Organization);
    // Only the tools the caller set: ProjectHostedToolPermissions::toJson()
    // writes the ones it carries and no others, so an unmentioned tool keeps
    // whatever it had.
    return d->client.issueRequest<ProjectHostedToolPermissionsReply>(
            Client::Client::Verb::Post, projectPath(projectId, kHostedToolPermissions), {},
            compactJson(permissions.toJson()));
}

AdminApiKeyListReply *Organization::listAdminApiKeys(const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<AdminApiKeyListReply>(Client::Client::Verb::Get,
                                                        QString(kAdminApiKeys), params.toQuery());
}

AdminApiKeyReply *Organization::createAdminApiKey(const QString &name, int expiresInSeconds)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("name"), name);
    // Omitted rather than sent as zero, which is the difference between "never
    // expires" and a key that expired the moment it was made.
    if (expiresInSeconds > 0)
        body.insert(QStringLiteral("expires_in_seconds"), expiresInSeconds);
    return d->client.issueRequest<AdminApiKeyReply>(Client::Client::Verb::Post,
                                                    QString(kAdminApiKeys), {}, compactJson(body));
}

AdminApiKeyReply *Organization::getAdminApiKey(const QString &keyId)
{
    Q_D(Organization);
    return d->client.issueRequest<AdminApiKeyReply>(Client::Client::Verb::Get,
                                                    resourcePath(kAdminApiKeys, keyId));
}

AdminApiKeyReply *Organization::deleteAdminApiKey(const QString &keyId)
{
    Q_D(Organization);
    return d->client.issueRequest<AdminApiKeyReply>(Client::Client::Verb::Delete,
                                                    resourcePath(kAdminApiKeys, keyId));
}

namespace {

// The body both the create and the update take: the same four required fields,
// which is why updateSpendAlert() replaces rather than patches.
QJsonObject spendAlertBody(const Core::SpendAlert &alert)
{
    QJsonObject body = alert.toJson();
    // The server assigns these; sending them back would be describing the
    // resource rather than requesting it.
    body.remove(QStringLiteral("id"));
    body.remove(QStringLiteral("object"));
    body.remove(QStringLiteral("deleted"));
    return body;
}

// The update body of either data-retention endpoint. The field is
// `retention_type` here and `type` on the resource -- see Core::DataRetention.
QJsonObject dataRetentionBody(const QString &retentionType)
{
    QJsonObject body;
    body.insert(QStringLiteral("retention_type"), retentionType);
    return body;
}

} // namespace

SpendAlertListReply *Organization::listSpendAlerts(const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertListReply>(Client::Client::Verb::Get,
                                                       QString(kSpendAlerts), params.toQuery());
}

SpendAlertReply *Organization::getSpendAlert(const QString &alertId)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertReply>(Client::Client::Verb::Get,
                                                   resourcePath(kSpendAlerts, alertId));
}

SpendAlertReply *Organization::createSpendAlert(const Core::SpendAlert &alert)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertReply>(Client::Client::Verb::Post,
                                                   QString(kSpendAlerts), {},
                                                   compactJson(spendAlertBody(alert)));
}

SpendAlertReply *Organization::updateSpendAlert(const QString &alertId,
                                                const Core::SpendAlert &alert)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertReply>(Client::Client::Verb::Post,
                                                   resourcePath(kSpendAlerts, alertId), {},
                                                   compactJson(spendAlertBody(alert)));
}

SpendAlertReply *Organization::deleteSpendAlert(const QString &alertId)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertReply>(Client::Client::Verb::Delete,
                                                   resourcePath(kSpendAlerts, alertId));
}

SpendAlertListReply *Organization::listProjectSpendAlerts(const QString &projectId,
                                                          const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertListReply>(Client::Client::Verb::Get,
                                                       projectPath(projectId, kSpendAlertsSegment),
                                                       params.toQuery());
}

SpendAlertReply *Organization::getProjectSpendAlert(const QString &projectId,
                                                    const QString &alertId)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertReply>(
            Client::Client::Verb::Get, projectPath(projectId, kSpendAlertsSegment, alertId));
}

SpendAlertReply *Organization::createProjectSpendAlert(const QString &projectId,
                                                       const Core::SpendAlert &alert)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertReply>(Client::Client::Verb::Post,
                                                   projectPath(projectId, kSpendAlertsSegment), {},
                                                   compactJson(spendAlertBody(alert)));
}

SpendAlertReply *Organization::updateProjectSpendAlert(const QString &projectId,
                                                       const QString &alertId,
                                                       const Core::SpendAlert &alert)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertReply>(
            Client::Client::Verb::Post, projectPath(projectId, kSpendAlertsSegment, alertId), {},
            compactJson(spendAlertBody(alert)));
}

SpendAlertReply *Organization::deleteProjectSpendAlert(const QString &projectId,
                                                       const QString &alertId)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendAlertReply>(
            Client::Client::Verb::Delete, projectPath(projectId, kSpendAlertsSegment, alertId));
}

namespace {

// The body both spend-limit writes take. `enforcement` is the server's report
// of whether the limit is biting, not a setting, so it never goes out -- and
// nor does the object or the deletion flag.
QJsonObject spendLimitBody(const Core::SpendLimit &limit)
{
    QJsonObject body = limit.toJson();
    body.remove(QStringLiteral("object"));
    body.remove(QStringLiteral("enforcement"));
    body.remove(QStringLiteral("deleted"));
    return body;
}

} // namespace

SpendLimitReply *Organization::getSpendLimit()
{
    Q_D(Organization);
    return d->client.issueRequest<SpendLimitReply>(Client::Client::Verb::Get, QString(kSpendLimit));
}

SpendLimitReply *Organization::setSpendLimit(const Core::SpendLimit &limit)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendLimitReply>(Client::Client::Verb::Post, QString(kSpendLimit),
                                                   {}, compactJson(spendLimitBody(limit)));
}

SpendLimitReply *Organization::deleteSpendLimit()
{
    Q_D(Organization);
    return d->client.issueRequest<SpendLimitReply>(Client::Client::Verb::Delete,
                                                   QString(kSpendLimit));
}

SpendLimitReply *Organization::getProjectSpendLimit(const QString &projectId)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendLimitReply>(Client::Client::Verb::Get,
                                                   projectPath(projectId, kSpendLimitSegment));
}

SpendLimitReply *Organization::setProjectSpendLimit(const QString &projectId,
                                                    const Core::SpendLimit &limit)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendLimitReply>(Client::Client::Verb::Post,
                                                   projectPath(projectId, kSpendLimitSegment), {},
                                                   compactJson(spendLimitBody(limit)));
}

SpendLimitReply *Organization::deleteProjectSpendLimit(const QString &projectId)
{
    Q_D(Organization);
    return d->client.issueRequest<SpendLimitReply>(Client::Client::Verb::Delete,
                                                   projectPath(projectId, kSpendLimitSegment));
}

DataRetentionReply *Organization::getDataRetention()
{
    Q_D(Organization);
    return d->client.issueRequest<DataRetentionReply>(Client::Client::Verb::Get,
                                                      QString(kDataRetention));
}

DataRetentionReply *Organization::setDataRetention(const QString &retentionType)
{
    Q_D(Organization);
    return d->client.issueRequest<DataRetentionReply>(
            Client::Client::Verb::Post, QString(kDataRetention), {},
            compactJson(dataRetentionBody(retentionType)));
}

DataRetentionReply *Organization::getProjectDataRetention(const QString &projectId)
{
    Q_D(Organization);
    return d->client.issueRequest<DataRetentionReply>(
            Client::Client::Verb::Get, projectPath(projectId, kDataRetentionSegment));
}

DataRetentionReply *Organization::setProjectDataRetention(const QString &projectId,
                                                          const QString &retentionType)
{
    Q_D(Organization);
    return d->client.issueRequest<DataRetentionReply>(
            Client::Client::Verb::Post, projectPath(projectId, kDataRetentionSegment), {},
            compactJson(dataRetentionBody(retentionType)));
}

AuditLogListReply *Organization::listAuditLogs(const AuditLogQuery &query)
{
    Q_D(Organization);
    return d->client.issueRequest<AuditLogListReply>(Client::Client::Verb::Get, QString(kAuditLogs),
                                                     query.toQuery());
}

UsageReply *Organization::usage(UsageKind kind, const UsageQuery &query)
{
    Q_D(Organization);
    QString path(kUsage);
    path += usageSegment(kind);
    return d->client.issueRequest<UsageReply>(Client::Client::Verb::Get, path, query.toQuery());
}

CostsReply *Organization::costs(const UsageQuery &query)
{
    Q_D(Organization);
    return d->client.issueRequest<CostsReply>(Client::Client::Verb::Get, kCosts, query.toQuery());
}

UserListReply *Organization::listUsers(const Client::ListParams &params, const QStringList &emails)
{
    Q_D(Organization);
    QUrlQuery query = params.toQuery();
    // Repeated items rather than comma-joined, the same convention UsageQuery
    // sends its array parameters with -- and an address is free to contain a
    // comma in the quoted form the RFC allows.
    for (const QString &email : emails)
        query.addQueryItem(QStringLiteral("emails"), email);

    return d->client.issueRequest<UserListReply>(Client::Client::Verb::Get, kUsers, query);
}

UserReply *Organization::getUser(const QString &userId)
{
    Q_D(Organization);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Get,
                                             resourcePath(kUsers, userId));
}

UserReply *Organization::modifyUserRole(const QString &userId, const QString &role)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("role"), role);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Post,
                                             resourcePath(kUsers, userId), {}, compactJson(body));
}

UserReply *Organization::deleteUser(const QString &userId)
{
    Q_D(Organization);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Delete,
                                             resourcePath(kUsers, userId));
}

InviteListReply *Organization::listInvites(const Client::ListParams &params)
{
    Q_D(Organization);
    return d->client.issueRequest<InviteListReply>(Client::Client::Verb::Get, kInvites,
                                                   params.toQuery());
}

InviteReply *Organization::getInvite(const QString &inviteId)
{
    Q_D(Organization);
    return d->client.issueRequest<InviteReply>(Client::Client::Verb::Get,
                                               resourcePath(kInvites, inviteId));
}

InviteReply *Organization::createInvite(const Core::CreateInviteRequest &request)
{
    Q_D(Organization);
    return d->client.issueRequest<InviteReply>(Client::Client::Verb::Post, kInvites, {},
                                               compactJson(request.toJson()));
}

InviteReply *Organization::deleteInvite(const QString &inviteId)
{
    Q_D(Organization);
    return d->client.issueRequest<InviteReply>(Client::Client::Verb::Delete,
                                               resourcePath(kInvites, inviteId));
}

} // namespace Admin
} // namespace QtOpenAi
