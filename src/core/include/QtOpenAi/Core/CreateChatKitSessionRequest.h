// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class CreateChatKitSessionRequestData;

// The body of a POST /chatkit/sessions request.
//
// Only the workflow id and the end-user identifier are required; every other
// field overrides a default the API would otherwise fill in (10 minutes of
// validity, 10 requests per minute, history on, uploads off). Unset overrides
// are left out of the body entirely, so a caller cannot pin a default by
// accident.
//
// The workflow's `tracing` and the `chatkit_configuration` tree are the two
// nested shapes: tracing is a single flag and is spelled out, while the
// configuration is a feature tree whose defaults the API resolves, so it is
// passed verbatim rather than half-modelled.
class QTOPENAI_CORE_EXPORT CreateChatKitSessionRequest
{
public:
    CreateChatKitSessionRequest();
    CreateChatKitSessionRequest(QString workflowId, QString user);
    CreateChatKitSessionRequest(const CreateChatKitSessionRequest &other);
    CreateChatKitSessionRequest(CreateChatKitSessionRequest &&other) noexcept;
    CreateChatKitSessionRequest &operator=(const CreateChatKitSessionRequest &other);
    CreateChatKitSessionRequest &operator=(CreateChatKitSessionRequest &&other) noexcept;
    ~CreateChatKitSessionRequest();

    void swap(CreateChatKitSessionRequest &other) noexcept { d.swap(other.d); }

    // The workflow that powers the session (`workflow.id`).
    QString workflowId() const;
    void setWorkflowId(const QString &workflowId);

    // A specific deployed version; the latest one when unset.
    QString workflowVersion() const;
    void setWorkflowVersion(const QString &workflowVersion);

    // Caller-defined state forwarded to the workflow (`state_variables`).
    QJsonObject workflowStateVariables() const;
    void setWorkflowStateVariables(const QJsonObject &stateVariables);

    // Diagnostic tracing for the workflow invocation; enabled when unset.
    std::optional<bool> workflowTracingEnabled() const;
    void setWorkflowTracingEnabled(bool enabled);

    // The end-user identifier the session is scoped to (`user`).
    QString user() const;
    void setUser(const QString &user);

    // Session lifetime in seconds from creation (`expires_after`, 1–600). The
    // anchor is fixed to "created_at" by the API and is sent along with it.
    std::optional<qint64> expiresAfterSeconds() const;
    void setExpiresAfter(qint64 seconds);

    // Per-minute request cap for the session (`rate_limits`).
    std::optional<int> maxRequestsPerMinute() const;
    void setMaxRequestsPerMinute(int maxRequestsPerMinute);

    // ChatKit feature overrides (`chatkit_configuration`), verbatim.
    QJsonObject configuration() const;
    void setConfiguration(const QJsonObject &configuration);

    QJsonObject toJson() const;

    bool operator==(const CreateChatKitSessionRequest &other) const;
    bool operator!=(const CreateChatKitSessionRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateChatKitSessionRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateChatKitSessionRequest)
