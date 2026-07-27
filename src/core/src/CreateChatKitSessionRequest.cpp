// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateChatKitSessionRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

#include <utility>

namespace QtOpenAi {
namespace Core {

class CreateChatKitSessionRequestData : public QSharedData
{
public:
    QString workflowId;
    QString workflowVersion;
    QJsonObject workflowStateVariables;
    std::optional<bool> workflowTracingEnabled;
    QString user;
    std::optional<qint64> expiresAfterSeconds;
    std::optional<int> maxRequestsPerMinute;
    QJsonObject configuration;
};

CreateChatKitSessionRequest::CreateChatKitSessionRequest()
    : d(new CreateChatKitSessionRequestData)
{ }

CreateChatKitSessionRequest::CreateChatKitSessionRequest(QString workflowId, QString user)
    : d(new CreateChatKitSessionRequestData)
{
    d->workflowId = std::move(workflowId);
    d->user = std::move(user);
}

CreateChatKitSessionRequest::CreateChatKitSessionRequest(const CreateChatKitSessionRequest &other)
        = default;
CreateChatKitSessionRequest::CreateChatKitSessionRequest(
        CreateChatKitSessionRequest &&other) noexcept
        = default;
CreateChatKitSessionRequest &
CreateChatKitSessionRequest::operator=(const CreateChatKitSessionRequest &other)
        = default;
CreateChatKitSessionRequest &
CreateChatKitSessionRequest::operator=(CreateChatKitSessionRequest &&other) noexcept
        = default;
CreateChatKitSessionRequest::~CreateChatKitSessionRequest() = default;

QString CreateChatKitSessionRequest::workflowId() const { return d->workflowId; }
void CreateChatKitSessionRequest::setWorkflowId(const QString &workflowId)
{
    d->workflowId = workflowId;
}

QString CreateChatKitSessionRequest::workflowVersion() const { return d->workflowVersion; }
void CreateChatKitSessionRequest::setWorkflowVersion(const QString &workflowVersion)
{
    d->workflowVersion = workflowVersion;
}

QJsonObject CreateChatKitSessionRequest::workflowStateVariables() const
{
    return d->workflowStateVariables;
}
void CreateChatKitSessionRequest::setWorkflowStateVariables(const QJsonObject &stateVariables)
{
    d->workflowStateVariables = stateVariables;
}

std::optional<bool> CreateChatKitSessionRequest::workflowTracingEnabled() const
{
    return d->workflowTracingEnabled;
}
void CreateChatKitSessionRequest::setWorkflowTracingEnabled(bool enabled)
{
    d->workflowTracingEnabled = enabled;
}

QString CreateChatKitSessionRequest::user() const { return d->user; }
void CreateChatKitSessionRequest::setUser(const QString &user) { d->user = user; }

std::optional<qint64> CreateChatKitSessionRequest::expiresAfterSeconds() const
{
    return d->expiresAfterSeconds;
}
void CreateChatKitSessionRequest::setExpiresAfter(qint64 seconds)
{
    d->expiresAfterSeconds = seconds;
}

std::optional<int> CreateChatKitSessionRequest::maxRequestsPerMinute() const
{
    return d->maxRequestsPerMinute;
}
void CreateChatKitSessionRequest::setMaxRequestsPerMinute(int maxRequestsPerMinute)
{
    d->maxRequestsPerMinute = maxRequestsPerMinute;
}

QJsonObject CreateChatKitSessionRequest::configuration() const { return d->configuration; }
void CreateChatKitSessionRequest::setConfiguration(const QJsonObject &configuration)
{
    d->configuration = configuration;
}

QJsonObject CreateChatKitSessionRequest::toJson() const
{
    QJsonObject workflow;
    detail::insertIfNotEmpty(workflow, QStringLiteral("id"), d->workflowId);
    detail::insertIfNotEmpty(workflow, QStringLiteral("version"), d->workflowVersion);
    if (!d->workflowStateVariables.isEmpty())
        workflow.insert(QStringLiteral("state_variables"), d->workflowStateVariables);
    if (d->workflowTracingEnabled) {
        workflow.insert(QStringLiteral("tracing"),
                        QJsonObject {{QStringLiteral("enabled"), *d->workflowTracingEnabled}});
    }

    QJsonObject json;
    json.insert(QStringLiteral("workflow"), workflow);
    detail::insertIfNotEmpty(json, QStringLiteral("user"), d->user);
    if (d->expiresAfterSeconds) {
        // The anchor is a constant in the API, so it travels with the seconds
        // rather than being a second thing to remember.
        json.insert(QStringLiteral("expires_after"),
                    QJsonObject {{QStringLiteral("anchor"), QStringLiteral("created_at")},
                                 {QStringLiteral("seconds"), *d->expiresAfterSeconds}});
    }
    if (d->maxRequestsPerMinute) {
        json.insert(QStringLiteral("rate_limits"),
                    QJsonObject {{QStringLiteral("max_requests_per_1_minute"),
                                  *d->maxRequestsPerMinute}});
    }
    if (!d->configuration.isEmpty())
        json.insert(QStringLiteral("chatkit_configuration"), d->configuration);
    return json;
}

bool CreateChatKitSessionRequest::operator==(const CreateChatKitSessionRequest &other) const
{
    return d->workflowId == other.d->workflowId && d->workflowVersion == other.d->workflowVersion
           && d->workflowStateVariables == other.d->workflowStateVariables
           && d->workflowTracingEnabled == other.d->workflowTracingEnabled
           && d->user == other.d->user && d->expiresAfterSeconds == other.d->expiresAfterSeconds
           && d->maxRequestsPerMinute == other.d->maxRequestsPerMinute
           && d->configuration == other.d->configuration;
}

} // namespace Core
} // namespace QtOpenAi
