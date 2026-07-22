// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ChatCompletionReply.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/ChatCompletionRequest.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>

class QNetworkAccessManager;

namespace QtOpenAi {
namespace Client {

class ClientPrivate;

// The entry point for talking to an OpenAI-compatible chat API.
//
// Configure a base URL and API key, then call createChatCompletion() to obtain
// an asynchronous ChatCompletionReply. The client works with any endpoint that
// speaks the OpenAI /chat/completions protocol (OpenAI, Azure OpenAI, Ollama,
// vLLM, LM Studio, ...).
class QTOPENAI_CLIENT_EXPORT Client : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString organization READ organization WRITE setOrganization NOTIFY organizationChanged)
public:
    explicit Client(QObject *parent = nullptr);
    // Construct with a base URL and key in one step.
    Client(QUrl baseUrl, QString apiKey, QObject *parent = nullptr);
    ~Client() override;

    // The API root, e.g. https://api.openai.com/v1. Endpoint paths such as
    // "/chat/completions" are appended to it. Defaults to the OpenAI root.
    QUrl baseUrl() const;
    void setBaseUrl(const QUrl &baseUrl);

    QString apiKey() const;
    void setApiKey(const QString &apiKey);

    // Optional OpenAI organization / project header value.
    QString organization() const;
    void setOrganization(const QString &organization);

    // Inject a custom QNetworkAccessManager (e.g. for proxies or test doubles).
    // The client does not take ownership.
    void setNetworkAccessManager(QNetworkAccessManager *manager);
    QNetworkAccessManager *networkAccessManager() const;

    // Start a chat completion. Ownership of the returned reply follows the
    // reply's auto-delete policy (enabled by default); pass a parent to tie its
    // lifetime elsewhere.
    ChatCompletionReply *createChatCompletion(const Core::ChatCompletionRequest &request);

Q_SIGNALS:
    void baseUrlChanged();
    void apiKeyChanged();
    void organizationChanged();

private:
    Q_DECLARE_PRIVATE(Client)
    QScopedPointer<ClientPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
