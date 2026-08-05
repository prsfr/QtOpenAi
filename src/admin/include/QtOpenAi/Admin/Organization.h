// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/CostsReply.h>
#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Admin/ProjectListReply.h>
#include <QtOpenAi/Admin/UsageQuery.h>
#include <QtOpenAi/Admin/UsageReply.h>
#include <QtOpenAi/Client/ListParams.h>
#include <QtOpenAi/Client/RetryPolicy.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
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
