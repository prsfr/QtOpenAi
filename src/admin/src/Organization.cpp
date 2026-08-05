// SPDX-License-Identifier: MIT
#include "QtOpenAi/Admin/Organization.h"

#include <QtOpenAi/Client/Client.h>

namespace QtOpenAi {
namespace Admin {

namespace {

// The collection this module's first endpoint family hangs off. Spelled once,
// as the endpoint paths in Client are.
constexpr auto kProjects = "/organization/projects";

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

    return d->client.issueRequest<ProjectListReply>(Client::Client::Verb::Get,
                                                    QLatin1String(kProjects), query);
}

} // namespace Admin
} // namespace QtOpenAi
