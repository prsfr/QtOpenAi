// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/Client.h"

#include "Multipart_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace QtOpenAi {
namespace Client {

namespace {
constexpr auto kDefaultBaseUrl = "https://api.openai.com/v1";
}

class ClientPrivate
{
public:
    QUrl baseUrl = QUrl(QLatin1String(kDefaultBaseUrl));
    QString apiKey;
    QString organization;
    Client::AuthScheme authScheme = Client::AuthScheme::BearerToken;
    QString apiVersion;
    RetryPolicy retryPolicy;
    int requestTimeoutMs = 0;
    QString userAgent;
    QHash<QByteArray, QByteArray> defaultHeaders;
    QNetworkAccessManager *manager = nullptr;
    bool ownsManager = false;

    // Join the base URL with an endpoint path (tolerating trailing slashes) and
    // append the Azure api-version query parameter when configured.
    QUrl endpointUrl(const QString &path) const
    {
        QString base = baseUrl.toString();
        while (base.endsWith(QLatin1Char('/')))
            base.chop(1);
        QString suffix = path;
        if (!suffix.startsWith(QLatin1Char('/')))
            suffix.prepend(QLatin1Char('/'));
        QUrl url(base + suffix);
        if (!apiVersion.isEmpty()) {
            QUrlQuery query(url);
            query.addQueryItem(QStringLiteral("api-version"), apiVersion);
            url.setQuery(query);
        }
        return url;
    }
};

Client::Client(QObject *parent)
    : QObject(parent)
    , d_ptr(new ClientPrivate)
{ }

Client::Client(QUrl baseUrl, QString apiKey, QObject *parent)
    : QObject(parent)
    , d_ptr(new ClientPrivate)
{
    Q_D(Client);
    d->baseUrl = std::move(baseUrl);
    d->apiKey = std::move(apiKey);
}

Client::~Client()
{
    Q_D(Client);
    if (d->ownsManager)
        delete d->manager;
}

QUrl Client::baseUrl() const
{
    Q_D(const Client);
    return d->baseUrl;
}

void Client::setBaseUrl(const QUrl &baseUrl)
{
    Q_D(Client);
    if (d->baseUrl == baseUrl)
        return;
    d->baseUrl = baseUrl;
    Q_EMIT baseUrlChanged();
}

QString Client::apiKey() const
{
    Q_D(const Client);
    return d->apiKey;
}

void Client::setApiKey(const QString &apiKey)
{
    Q_D(Client);
    if (d->apiKey == apiKey)
        return;
    d->apiKey = apiKey;
    Q_EMIT apiKeyChanged();
}

QString Client::organization() const
{
    Q_D(const Client);
    return d->organization;
}

void Client::setOrganization(const QString &organization)
{
    Q_D(Client);
    if (d->organization == organization)
        return;
    d->organization = organization;
    Q_EMIT organizationChanged();
}

Client::AuthScheme Client::authScheme() const
{
    Q_D(const Client);
    return d->authScheme;
}

void Client::setAuthScheme(AuthScheme scheme)
{
    Q_D(Client);
    d->authScheme = scheme;
}

QString Client::apiVersion() const
{
    Q_D(const Client);
    return d->apiVersion;
}

void Client::setApiVersion(const QString &apiVersion)
{
    Q_D(Client);
    d->apiVersion = apiVersion;
}

RetryPolicy Client::retryPolicy() const
{
    Q_D(const Client);
    return d->retryPolicy;
}

void Client::setRetryPolicy(const RetryPolicy &policy)
{
    Q_D(Client);
    d->retryPolicy = policy;
}

int Client::requestTimeoutMs() const
{
    Q_D(const Client);
    return d->requestTimeoutMs;
}

void Client::setRequestTimeoutMs(int timeoutMs)
{
    Q_D(Client);
    d->requestTimeoutMs = timeoutMs;
}

QString Client::userAgent() const
{
    Q_D(const Client);
    return d->userAgent;
}

void Client::setUserAgent(const QString &userAgent)
{
    Q_D(Client);
    d->userAgent = userAgent;
}

void Client::setDefaultHeader(const QByteArray &name, const QByteArray &value)
{
    Q_D(Client);
    d->defaultHeaders.insert(name, value);
}

void Client::removeDefaultHeader(const QByteArray &name)
{
    Q_D(Client);
    d->defaultHeaders.remove(name);
}

QHash<QByteArray, QByteArray> Client::defaultHeaders() const
{
    Q_D(const Client);
    return d->defaultHeaders;
}

void Client::setNetworkAccessManager(QNetworkAccessManager *manager)
{
    Q_D(Client);
    if (d->manager == manager)
        return;
    if (d->ownsManager)
        delete d->manager;
    d->manager = manager;
    d->ownsManager = false;
}

QNetworkAccessManager *Client::networkAccessManager() const
{
    Q_D(const Client);
    if (!d->manager) {
        // Lazily create an owned manager on first use.
        auto *self = const_cast<ClientPrivate *>(d);
        self->manager = new QNetworkAccessManager(const_cast<Client *>(this));
        self->ownsManager = false; // parented to the Client, freed with it
    }
    return d->manager;
}

namespace {

// Build a network request for an endpoint path (URL + auth/content/custom
// headers + timeout), applying the configured auth scheme.
QNetworkRequest apiRequest(const ClientPrivate *d, const QString &path)
{
    QNetworkRequest networkRequest(d->endpointUrl(path));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             QStringLiteral("application/json"));
    if (!d->apiKey.isEmpty()) {
        if (d->authScheme == Client::AuthScheme::AzureApiKey)
            networkRequest.setRawHeader("api-key", d->apiKey.toUtf8());
        else
            networkRequest.setRawHeader("Authorization",
                                        QByteArray("Bearer ") + d->apiKey.toUtf8());
    }
    if (!d->organization.isEmpty())
        networkRequest.setRawHeader("OpenAI-Organization", d->organization.toUtf8());
    if (!d->userAgent.isEmpty())
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, d->userAgent);
    for (auto it = d->defaultHeaders.constBegin(); it != d->defaultHeaders.constEnd(); ++it)
        networkRequest.setRawHeader(it.key(), it.value());
    if (d->requestTimeoutMs > 0)
        networkRequest.setTransferTimeout(d->requestTimeoutMs);
    return networkRequest;
}

// The /chat/completions request, retaining the original spelling for callers.
QNetworkRequest chatRequest(const ClientPrivate *d)
{
    return apiRequest(d, QStringLiteral("/chat/completions"));
}

// Serialise a list of output/input items to a JSON array.
QJsonArray itemsToArray(const QList<Core::ResponseOutputItem> &items)
{
    QJsonArray array;
    for (const Core::ResponseOutputItem &item : items)
        array.append(item.toJson());
    return array;
}

// Merge extra query items (e.g. pagination) into a built request's URL,
// preserving any already present (such as the Azure api-version parameter).
void applyQuery(QNetworkRequest &request, const QUrlQuery &extra)
{
    if (extra.isEmpty())
        return;
    QUrl url = request.url();
    QUrlQuery query(url);
    const auto items = extra.queryItems();
    for (const auto &item : items)
        query.addQueryItem(item.first, item.second);
    url.setQuery(query);
    request.setUrl(url);
}

// Build a request factory that POSTs a multipart/form-data body. A fresh
// QHttpMultiPart is created per attempt (they are single-use) and parented to
// the reply so it is freed with it. The Content-Type header is set from the
// generated boundary, overriding the JSON default from apiRequest().
std::function<QNetworkReply *()> multipartPostFactory(QNetworkAccessManager *manager,
                                                      QNetworkRequest request,
                                                      QList<QPair<QString, QString>> fields,
                                                      QList<detail::FormFilePart> files)
{
    return [manager, request, fields = std::move(fields), files = std::move(files)]() mutable {
        QHttpMultiPart *multiPart = detail::buildMultipart(fields, files);
        QNetworkRequest req = request;
        req.setHeader(QNetworkRequest::ContentTypeHeader,
                      QByteArray("multipart/form-data; boundary=") + multiPart->boundary());
        QNetworkReply *reply = manager->post(req, multiPart);
        multiPart->setParent(reply);
        return reply;
    };
}

} // namespace

ChatCompletionReply *Client::createChatCompletion(const Core::ChatCompletionRequest &request)
{
    Q_D(Client);
    const QByteArray body = QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    // Capture what a retry needs to re-issue the request.
    auto factory = [manager, req = chatRequest(d), body]() { return manager->post(req, body); };
    return new ChatCompletionReply(std::move(factory), d->retryPolicy);
}

ModerationReply *Client::createModeration(const Core::ModerationRequest &request)
{
    Q_D(Client);
    const QByteArray body = QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req = apiRequest(d, QStringLiteral("/moderations")), body]() {
        return manager->post(req, body);
    };
    return new ModerationReply(std::move(factory), d->retryPolicy);
}

CompletionReply *Client::createCompletion(const Core::CompletionRequest &request)
{
    Q_D(Client);
    const QByteArray body = QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req = apiRequest(d, QStringLiteral("/completions")), body]() {
        return manager->post(req, body);
    };
    return new CompletionReply(std::move(factory), d->retryPolicy);
}

CompletionStreamReply *Client::createCompletionStream(const Core::CompletionRequest &request)
{
    Q_D(Client);

    // Force streaming on a copy so the caller's request is left untouched.
    Core::CompletionRequest streamed = request;
    streamed.setStream(true);

    QNetworkRequest networkRequest = apiRequest(d, QStringLiteral("/completions"));
    networkRequest.setRawHeader("Accept", "text/event-stream");

    const QByteArray body = QJsonDocument(streamed.toJson()).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = networkAccessManager()->post(networkRequest, body);
    return new CompletionStreamReply(reply);
}

ChatCompletionStreamReply *
Client::createChatCompletionStream(const Core::ChatCompletionRequest &request)
{
    Q_D(Client);

    // Force streaming on a copy so the caller's request is left untouched.
    Core::ChatCompletionRequest streamed = request;
    streamed.setStream(true);

    QNetworkRequest networkRequest = chatRequest(d);
    networkRequest.setRawHeader("Accept", "text/event-stream");

    const QByteArray body = QJsonDocument(streamed.toJson()).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = networkAccessManager()->post(networkRequest, body);
    return new ChatCompletionStreamReply(reply);
}

ResponseReply *Client::createResponse(const Core::ResponseRequest &request)
{
    Q_D(Client);
    const QByteArray body = QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req = apiRequest(d, QStringLiteral("/responses")), body]() {
        return manager->post(req, body);
    };
    return new ResponseReply(std::move(factory), d->retryPolicy);
}

ResponseStreamReply *Client::createResponseStream(const Core::ResponseRequest &request)
{
    Q_D(Client);

    // Force streaming on a copy so the caller's request is left untouched.
    Core::ResponseRequest streamed = request;
    streamed.setStream(true);

    QNetworkRequest networkRequest = apiRequest(d, QStringLiteral("/responses"));
    networkRequest.setRawHeader("Accept", "text/event-stream");

    const QByteArray body = QJsonDocument(streamed.toJson()).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = networkAccessManager()->post(networkRequest, body);
    return new ResponseStreamReply(reply);
}

ResponseReply *Client::getResponse(const QString &responseId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/responses/") + responseId;
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->get(req); };
    return new ResponseReply(std::move(factory), d->retryPolicy);
}

ResponseReply *Client::cancelResponse(const QString &responseId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/responses/") + responseId + QStringLiteral("/cancel");
    auto factory
            = [manager, req = apiRequest(d, path)]() { return manager->post(req, QByteArray()); };
    return new ResponseReply(std::move(factory), d->retryPolicy);
}

ResponseReply *Client::deleteResponse(const QString &responseId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/responses/") + responseId;
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->deleteResource(req); };
    return new ResponseReply(std::move(factory), d->retryPolicy);
}

ConversationReply *Client::createConversation(const QJsonObject &metadata,
                                              const QList<Core::ResponseOutputItem> &items)
{
    Q_D(Client);
    QJsonObject bodyObject;
    if (!metadata.isEmpty())
        bodyObject.insert(QStringLiteral("metadata"), metadata);
    if (!items.isEmpty())
        bodyObject.insert(QStringLiteral("items"), itemsToArray(items));
    const QByteArray body = QJsonDocument(bodyObject).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req = apiRequest(d, QStringLiteral("/conversations")), body]() {
        return manager->post(req, body);
    };
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ConversationReply *Client::getConversation(const QString &conversationId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId;
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->get(req); };
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ConversationReply *Client::updateConversation(const QString &conversationId,
                                              const QJsonObject &metadata)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("metadata"), metadata);
    const QByteArray body = QJsonDocument(bodyObject).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId;
    auto factory
            = [manager, req = apiRequest(d, path), body]() { return manager->post(req, body); };
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ConversationReply *Client::deleteConversation(const QString &conversationId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId;
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->deleteResource(req); };
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ConversationItemsReply *Client::listConversationItems(const QString &conversationId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path
            = QStringLiteral("/conversations/") + conversationId + QStringLiteral("/items");
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->get(req); };
    return new ConversationItemsReply(std::move(factory), d->retryPolicy);
}

ConversationItemsReply *
Client::createConversationItems(const QString &conversationId,
                                const QList<Core::ResponseOutputItem> &items)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("items"), itemsToArray(items));
    const QByteArray body = QJsonDocument(bodyObject).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path
            = QStringLiteral("/conversations/") + conversationId + QStringLiteral("/items");
    auto factory
            = [manager, req = apiRequest(d, path), body]() { return manager->post(req, body); };
    return new ConversationItemsReply(std::move(factory), d->retryPolicy);
}

ConversationItemsReply *Client::getConversationItem(const QString &conversationId,
                                                    const QString &itemId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId
                         + QStringLiteral("/items/") + itemId;
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->get(req); };
    return new ConversationItemsReply(std::move(factory), d->retryPolicy);
}

ConversationReply *Client::deleteConversationItem(const QString &conversationId,
                                                  const QString &itemId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId
                         + QStringLiteral("/items/") + itemId;
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->deleteResource(req); };
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ChatCompletionListReply *Client::listChatCompletions(const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, QStringLiteral("/chat/completions"));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req]() { return manager->get(req); };
    return new ChatCompletionListReply(std::move(factory), d->retryPolicy);
}

ChatCompletionReply *Client::getChatCompletion(const QString &completionId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/chat/completions/") + completionId;
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->get(req); };
    return new ChatCompletionReply(std::move(factory), d->retryPolicy);
}

ChatCompletionReply *Client::updateChatCompletion(const QString &completionId,
                                                  const QJsonObject &metadata)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("metadata"), metadata);
    const QByteArray body = QJsonDocument(bodyObject).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/chat/completions/") + completionId;
    auto factory
            = [manager, req = apiRequest(d, path), body]() { return manager->post(req, body); };
    return new ChatCompletionReply(std::move(factory), d->retryPolicy);
}

ChatCompletionReply *Client::deleteChatCompletion(const QString &completionId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/chat/completions/") + completionId;
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->deleteResource(req); };
    return new ChatCompletionReply(std::move(factory), d->retryPolicy);
}

ChatCompletionMessageListReply *Client::listChatCompletionMessages(const QString &completionId,
                                                                   const ListParams &params)
{
    Q_D(Client);
    const QString path
            = QStringLiteral("/chat/completions/") + completionId + QStringLiteral("/messages");
    QNetworkRequest req = apiRequest(d, path);
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req]() { return manager->get(req); };
    return new ChatCompletionMessageListReply(std::move(factory), d->retryPolicy);
}

EmbeddingReply *Client::createEmbeddings(const Core::EmbeddingRequest &request)
{
    Q_D(Client);
    const QByteArray body = QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req = apiRequest(d, QStringLiteral("/embeddings")), body]() {
        return manager->post(req, body);
    };
    return new EmbeddingReply(std::move(factory), d->retryPolicy);
}

TranscriptionReply *Client::createTranscription(const Core::TranscriptionRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"file", request.fileName(), request.fileData()};
    auto factory = multipartPostFactory(networkAccessManager(),
                                        apiRequest(d, QStringLiteral("/audio/transcriptions")),
                                        request.formFields(), {std::move(file)});
    return new TranscriptionReply(std::move(factory), d->retryPolicy);
}

TranscriptionReply *Client::createTranslation(const Core::TranslationRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"file", request.fileName(), request.fileData()};
    auto factory = multipartPostFactory(networkAccessManager(),
                                        apiRequest(d, QStringLiteral("/audio/translations")),
                                        request.formFields(), {std::move(file)});
    return new TranscriptionReply(std::move(factory), d->retryPolicy);
}

ImageReply *Client::createImage(const Core::ImageGenerationRequest &request)
{
    Q_D(Client);
    const QByteArray body = QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req = apiRequest(d, QStringLiteral("/images/generations")), body]() {
        return manager->post(req, body);
    };
    return new ImageReply(std::move(factory), d->retryPolicy);
}

ImageReply *Client::createImageEdit(const Core::ImageEditRequest &request)
{
    Q_D(Client);
    QList<detail::FormFilePart> files;
    const QList<Core::ImageEditRequest::ImageFile> images = request.images();
    // A single image is uploaded as `image`; multiple use the `image[]` form.
    const QByteArray imageField = images.size() > 1 ? QByteArray("image[]") : QByteArray("image");
    for (const auto &image : images)
        files.append({imageField, image.first, image.second});
    if (request.hasMask())
        files.append({"mask", request.maskFileName(), request.maskData()});

    auto factory = multipartPostFactory(networkAccessManager(),
                                        apiRequest(d, QStringLiteral("/images/edits")),
                                        request.formFields(), std::move(files));
    return new ImageReply(std::move(factory), d->retryPolicy);
}

ImageReply *Client::createImageVariation(const Core::ImageVariationRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"image", request.fileName(), request.imageData()};
    auto factory = multipartPostFactory(networkAccessManager(),
                                        apiRequest(d, QStringLiteral("/images/variations")),
                                        request.formFields(), {std::move(file)});
    return new ImageReply(std::move(factory), d->retryPolicy);
}

SpeechReply *Client::createSpeech(const Core::SpeechRequest &request)
{
    Q_D(Client);
    const QByteArray body = QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req = apiRequest(d, QStringLiteral("/audio/speech")), body]() {
        return manager->post(req, body);
    };
    return new SpeechReply(std::move(factory), d->retryPolicy);
}

ModelListReply *Client::listModels()
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = [manager, req = apiRequest(d, QStringLiteral("/models"))]() {
        return manager->get(req);
    };
    return new ModelListReply(std::move(factory), d->retryPolicy);
}

ModelReply *Client::getModel(const QString &modelId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/models/") + modelId;
    auto factory = [manager, req = apiRequest(d, path)]() { return manager->get(req); };
    return new ModelReply(std::move(factory), d->retryPolicy);
}

} // namespace Client
} // namespace QtOpenAi
