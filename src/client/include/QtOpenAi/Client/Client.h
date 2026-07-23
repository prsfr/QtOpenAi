// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ChatCompletionListReply.h>
#include <QtOpenAi/Client/ChatCompletionMessageListReply.h>
#include <QtOpenAi/Client/ChatCompletionReply.h>
#include <QtOpenAi/Client/ChatCompletionStreamReply.h>
#include <QtOpenAi/Client/CompletionReply.h>
#include <QtOpenAi/Client/CompletionStreamReply.h>
#include <QtOpenAi/Client/ConversationItemsReply.h>
#include <QtOpenAi/Client/ConversationReply.h>
#include <QtOpenAi/Client/EmbeddingReply.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/ListParams.h>
#include <QtOpenAi/Client/ModelListReply.h>
#include <QtOpenAi/Client/ModelReply.h>
#include <QtOpenAi/Client/ModerationReply.h>
#include <QtOpenAi/Client/ResponseReply.h>
#include <QtOpenAi/Client/ResponseStreamReply.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/ChatCompletionRequest.h>
#include <QtOpenAi/Core/CompletionRequest.h>
#include <QtOpenAi/Core/EmbeddingRequest.h>
#include <QtOpenAi/Core/ModerationRequest.h>
#include <QtOpenAi/Core/ResponseOutputItem.h>
#include <QtOpenAi/Core/ResponseRequest.h>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
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
    Q_PROPERTY(
            QString organization READ organization WRITE setOrganization NOTIFY organizationChanged)
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

    // How the API key is presented. BearerToken (default) sends
    // `Authorization: Bearer <key>`; AzureApiKey sends `api-key: <key>`.
    enum class AuthScheme {
        BearerToken,
        AzureApiKey
    };
    Q_ENUM(AuthScheme)
    AuthScheme authScheme() const;
    void setAuthScheme(AuthScheme scheme);

    // Azure OpenAI `api-version` query parameter (appended to every request when
    // non-empty). Ignored by standard OpenAI endpoints.
    QString apiVersion() const;
    void setApiVersion(const QString &apiVersion);

    // Automatic-retry policy for transient failures (429/5xx/network).
    RetryPolicy retryPolicy() const;
    void setRetryPolicy(const RetryPolicy &policy);

    // Per-request transfer timeout in milliseconds (0 disables; default 0).
    int requestTimeoutMs() const;
    void setRequestTimeoutMs(int timeoutMs);

    // Custom User-Agent (empty leaves Qt's default).
    QString userAgent() const;
    void setUserAgent(const QString &userAgent);

    // Extra headers sent with every request (e.g. provider-specific).
    void setDefaultHeader(const QByteArray &name, const QByteArray &value);
    void removeDefaultHeader(const QByteArray &name);
    QHash<QByteArray, QByteArray> defaultHeaders() const;

    // Inject a custom QNetworkAccessManager (e.g. for proxies or test doubles).
    // The client does not take ownership.
    void setNetworkAccessManager(QNetworkAccessManager *manager);
    QNetworkAccessManager *networkAccessManager() const;

    // Start a chat completion. Ownership of the returned reply follows the
    // reply's auto-delete policy (enabled by default); pass a parent to tie its
    // lifetime elsewhere.
    ChatCompletionReply *createChatCompletion(const Core::ChatCompletionRequest &request);

    // Start a streamed chat completion (Server-Sent Events). Forces the
    // request's `stream` flag to true and returns a ChatCompletionStreamReply
    // that emits incremental deltas. Ownership follows the reply's auto-delete
    // policy (enabled by default).
    ChatCompletionStreamReply *
    createChatCompletionStream(const Core::ChatCompletionRequest &request);

    // Classify text/image input against the moderation policy (POST /moderations).
    ModerationReply *createModeration(const Core::ModerationRequest &request);

    // Legacy text completion (POST /completions). Mainly for OpenAI-compatible
    // servers that only expose the deprecated endpoint. Ownership follows the
    // reply's auto-delete policy (enabled by default).
    CompletionReply *createCompletion(const Core::CompletionRequest &request);

    // Streamed legacy text completion (Server-Sent Events). Forces the request's
    // `stream` flag to true and returns a CompletionStreamReply emitting text
    // deltas. Ownership follows the reply's auto-delete policy.
    CompletionStreamReply *createCompletionStream(const Core::CompletionRequest &request);

    // --- Responses API (POST/GET/DELETE /responses) ------------------------
    // Create a response. Ownership follows the reply's auto-delete policy
    // (enabled by default); pass a parent to tie its lifetime elsewhere.
    ResponseReply *createResponse(const Core::ResponseRequest &request);

    // Streamed response (Server-Sent Events). Forces the request's `stream` flag
    // to true and returns a ResponseStreamReply emitting typed events. Ownership
    // follows the reply's auto-delete policy.
    ResponseStreamReply *createResponseStream(const Core::ResponseRequest &request);

    // Retrieve a previously created (stored) response by id.
    ResponseReply *getResponse(const QString &responseId);

    // Cancel an in-progress background response.
    ResponseReply *cancelResponse(const QString &responseId);

    // Delete a stored response. On success the reply's response() carries the
    // deletion acknowledgement (object "response.deleted").
    ResponseReply *deleteResponse(const QString &responseId);

    // --- Conversations API (/conversations) --------------------------------
    // Create a conversation, optionally seeded with items and metadata.
    ConversationReply *createConversation(const QJsonObject &metadata = {},
                                          const QList<Core::ResponseOutputItem> &items = {});

    ConversationReply *getConversation(const QString &conversationId);

    // Replace the conversation's metadata.
    ConversationReply *updateConversation(const QString &conversationId,
                                          const QJsonObject &metadata);

    ConversationReply *deleteConversation(const QString &conversationId);

    // List the items in a conversation (most-recent-first by default).
    ConversationItemsReply *listConversationItems(const QString &conversationId);

    // Append items to a conversation.
    ConversationItemsReply *createConversationItems(const QString &conversationId,
                                                    const QList<Core::ResponseOutputItem> &items);

    // Fetch a single conversation item (surfaced as a one-item list).
    ConversationItemsReply *getConversationItem(const QString &conversationId,
                                                const QString &itemId);

    // Delete an item; on success the reply carries the updated conversation.
    ConversationReply *deleteConversationItem(const QString &conversationId, const QString &itemId);

    // --- Stored Chat Completions management (/chat/completions/{id}) --------
    // List stored chat completions (created with store: true).
    ChatCompletionListReply *listChatCompletions(const ListParams &params = {});

    // Retrieve a stored chat completion by id.
    ChatCompletionReply *getChatCompletion(const QString &completionId);

    // Replace a stored completion's metadata.
    ChatCompletionReply *updateChatCompletion(const QString &completionId,
                                              const QJsonObject &metadata);

    // Delete a stored chat completion.
    ChatCompletionReply *deleteChatCompletion(const QString &completionId);

    // List the input messages of a stored chat completion.
    ChatCompletionMessageListReply *listChatCompletionMessages(const QString &completionId,
                                                               const ListParams &params = {});

    // --- Embeddings (/embeddings) ------------------------------------------
    // Create embeddings for the request's input.
    EmbeddingReply *createEmbeddings(const Core::EmbeddingRequest &request);

    // --- Models (/models) --------------------------------------------------
    // List the available models.
    ModelListReply *listModels();

    // Retrieve a single model by id.
    ModelReply *getModel(const QString &modelId);

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
