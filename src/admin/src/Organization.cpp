// SPDX-License-Identifier: MIT
#include "QtOpenAi/Admin/Organization.h"

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QJsonDocument>
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

// The sub-resources that hang off a project. Every one of them repeats the same
// list/create/get/delete shape under a project id, so they are segments composed
// onto kProjects rather than eleven more full paths.
constexpr QLatin1String kProjectUsers("/users");
constexpr QLatin1String kServiceAccounts("/service_accounts");
constexpr QLatin1String kApiKeys("/api_keys");
constexpr QLatin1String kRateLimits("/rate_limits");
constexpr QLatin1String kArchive("/archive");

// Compose a resource path: a collection, optionally one member of it, optionally
// a sub-resource below that -- e.g. ("/organization/projects", "proj_1",
// "/api_keys"). The same helper Client.cpp composes its nested paths with, so
// the endpoint methods stay free of string arithmetic and every path is built
// from the constants above rather than retyped.
QString resourcePath(QLatin1String collection, const QString &id, const QString &suffix = {})
{
    QString path(collection);
    if (!id.isEmpty())
        path += QLatin1Char('/') + id;
    return path + suffix;
}

// A member of a project's sub-collection, which is two levels of the above:
// ("proj_1", "/api_keys", "key_1") -> "/organization/projects/proj_1/api_keys/key_1".
QString projectPath(const QString &projectId, QLatin1String collection, const QString &id = {})
{
    return resourcePath(kProjects, projectId, resourcePath(collection, id));
}

// Serialise a request body object into a compact JSON payload, as Client does.
QByteArray compactJson(const QJsonObject &json)
{
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
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
            Client::Client::Verb::Get, projectPath(projectId, kProjectUsers), params.toQuery());
}

UserReply *Organization::getProjectUser(const QString &projectId, const QString &userId)
{
    Q_D(Organization);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Get,
                                             projectPath(projectId, kProjectUsers, userId));
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
                                             projectPath(projectId, kProjectUsers), {},
                                             compactJson(body));
}

UserReply *Organization::modifyProjectUserRole(const QString &projectId, const QString &userId,
                                               const QString &role)
{
    Q_D(Organization);
    QJsonObject body;
    body.insert(QStringLiteral("role"), role);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Post,
                                             projectPath(projectId, kProjectUsers, userId), {},
                                             compactJson(body));
}

UserReply *Organization::deleteProjectUser(const QString &projectId, const QString &userId)
{
    Q_D(Organization);
    return d->client.issueRequest<UserReply>(Client::Client::Verb::Delete,
                                             projectPath(projectId, kProjectUsers, userId));
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
