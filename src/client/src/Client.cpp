// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/Client.h"

#include "Multipart_p.h"

#include <QtCore/QBuffer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QUrlQuery>
#include <QtCore/QUuid>
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
    bool idempotencyKeys = true;
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

bool Client::idempotencyKeysEnabled() const
{
    Q_D(const Client);
    return d->idempotencyKeys;
}

void Client::setIdempotencyKeysEnabled(bool enabled)
{
    Q_D(Client);
    d->idempotencyKeys = enabled;
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

// Stamp a per-call Idempotency-Key on a request that is about to be POSTed, so
// the automatic retries cannot be charged twice. Generated once per request
// factory rather than per attempt — that is the whole point: every attempt of
// one logical call must carry the same key. GETs are idempotent by definition
// and are left alone.
void applyIdempotencyKey(const ClientPrivate *d, QNetworkRequest &request)
{
    if (!d->idempotencyKeys || request.hasRawHeader("Idempotency-Key"))
        return;
    request.setRawHeader("Idempotency-Key",
                         QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());
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
std::function<QNetworkReply *()> multipartPostFactory(const ClientPrivate *d,
                                                      QNetworkAccessManager *manager,
                                                      QNetworkRequest request,
                                                      QList<QPair<QString, QString>> fields,
                                                      QList<detail::FormFilePart> files)
{
    applyIdempotencyKey(d, request);
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

// Compose a nested resource path: a collection, optionally one member of it,
// optionally a sub-resource below that — e.g. ("/vector_stores", "vs_1",
// "/files/f_1"). Several endpoint families nest three levels deep, so building
// them here keeps the endpoint methods free of string arithmetic.
QString resourcePath(QLatin1String collection, const QString &id, const QString &suffix = {})
{
    QString path(collection);
    if (!id.isEmpty())
        path += QLatin1Char('/') + id;
    return path + suffix;
}

QString vectorStorePath(const QString &vectorStoreId, const QString &suffix = {})
{
    return resourcePath(QLatin1String("/vector_stores"), vectorStoreId, suffix);
}

QString containerPath(const QString &containerId, const QString &suffix = {})
{
    return resourcePath(QLatin1String("/containers"), containerId, suffix);
}

QString fineTuningJobPath(const QString &jobId, const QString &suffix = {})
{
    return resourcePath(QLatin1String("/fine_tuning/jobs"), jobId, suffix);
}

QString fineTuningCheckpointPath(const QString &checkpointId, const QString &suffix = {})
{
    return resourcePath(QLatin1String("/fine_tuning/checkpoints"), checkpointId, suffix);
}

// Serialise a list of ids to a JSON array.
QJsonArray idsToArray(const QStringList &ids)
{
    QJsonArray array;
    for (const QString &id : ids)
        array.append(id);
    return array;
}

// Serialise a request body object into a compact JSON payload.
QByteArray compactJson(const QJsonObject &json)
{
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

// Request factories capturing everything a retry needs to re-issue the call.
// One per HTTP verb; the multipart POST variant lives in multipartPostFactory().
std::function<QNetworkReply *()> getFactory(QNetworkAccessManager *manager, QNetworkRequest request)
{
    return [manager, request = std::move(request)]() { return manager->get(request); };
}

std::function<QNetworkReply *()> deleteFactory(QNetworkAccessManager *manager,
                                               QNetworkRequest request)
{
    return [manager, request = std::move(request)]() { return manager->deleteResource(request); };
}

std::function<QNetworkReply *()> postFactory(const ClientPrivate *d, QNetworkAccessManager *manager,
                                             QNetworkRequest request, QByteArray body = {})
{
    applyIdempotencyKey(d, request);
    return [manager, request = std::move(request), body = std::move(body)]() {
        return manager->post(request, body);
    };
}

} // namespace

ChatCompletionReply *Client::createChatCompletion(const Core::ChatCompletionRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    // Capture what a retry needs to re-issue the request.
    auto factory = postFactory(d, manager, chatRequest(d), body);
    return new ChatCompletionReply(std::move(factory), d->retryPolicy);
}

ModerationReply *Client::createModeration(const Core::ModerationRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, QStringLiteral("/moderations")), body);
    return new ModerationReply(std::move(factory), d->retryPolicy);
}

CompletionReply *Client::createCompletion(const Core::CompletionRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, QStringLiteral("/completions")), body);
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

    const QByteArray body = compactJson(streamed.toJson());
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

    const QByteArray body = compactJson(streamed.toJson());
    QNetworkReply *reply = networkAccessManager()->post(networkRequest, body);
    return new ChatCompletionStreamReply(reply);
}

ResponseReply *Client::createResponse(const Core::ResponseRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, QStringLiteral("/responses")), body);
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

    const QByteArray body = compactJson(streamed.toJson());
    QNetworkReply *reply = networkAccessManager()->post(networkRequest, body);
    return new ResponseStreamReply(reply);
}

ResponseReply *Client::getResponse(const QString &responseId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/responses/") + responseId;
    auto factory = getFactory(manager, apiRequest(d, path));
    return new ResponseReply(std::move(factory), d->retryPolicy);
}

ResponseReply *Client::cancelResponse(const QString &responseId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/responses/") + responseId + QStringLiteral("/cancel");
    auto factory = postFactory(d, manager, apiRequest(d, path));
    return new ResponseReply(std::move(factory), d->retryPolicy);
}

ResponseReply *Client::deleteResponse(const QString &responseId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/responses/") + responseId;
    auto factory = deleteFactory(manager, apiRequest(d, path));
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
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, QStringLiteral("/conversations")), body);
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ConversationReply *Client::getConversation(const QString &conversationId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId;
    auto factory = getFactory(manager, apiRequest(d, path));
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ConversationReply *Client::updateConversation(const QString &conversationId,
                                              const QJsonObject &metadata)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("metadata"), metadata);
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId;
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ConversationReply *Client::deleteConversation(const QString &conversationId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId;
    auto factory = deleteFactory(manager, apiRequest(d, path));
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ConversationItemsReply *Client::listConversationItems(const QString &conversationId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path
            = QStringLiteral("/conversations/") + conversationId + QStringLiteral("/items");
    auto factory = getFactory(manager, apiRequest(d, path));
    return new ConversationItemsReply(std::move(factory), d->retryPolicy);
}

ConversationItemsReply *
Client::createConversationItems(const QString &conversationId,
                                const QList<Core::ResponseOutputItem> &items)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("items"), itemsToArray(items));
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path
            = QStringLiteral("/conversations/") + conversationId + QStringLiteral("/items");
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new ConversationItemsReply(std::move(factory), d->retryPolicy);
}

ConversationItemsReply *Client::getConversationItem(const QString &conversationId,
                                                    const QString &itemId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId
                         + QStringLiteral("/items/") + itemId;
    auto factory = getFactory(manager, apiRequest(d, path));
    return new ConversationItemsReply(std::move(factory), d->retryPolicy);
}

ConversationReply *Client::deleteConversationItem(const QString &conversationId,
                                                  const QString &itemId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/conversations/") + conversationId
                         + QStringLiteral("/items/") + itemId;
    auto factory = deleteFactory(manager, apiRequest(d, path));
    return new ConversationReply(std::move(factory), d->retryPolicy);
}

ChatCompletionListReply *Client::listChatCompletions(const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, QStringLiteral("/chat/completions"));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new ChatCompletionListReply(std::move(factory), d->retryPolicy);
}

ChatCompletionReply *Client::getChatCompletion(const QString &completionId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/chat/completions/") + completionId;
    auto factory = getFactory(manager, apiRequest(d, path));
    return new ChatCompletionReply(std::move(factory), d->retryPolicy);
}

ChatCompletionReply *Client::updateChatCompletion(const QString &completionId,
                                                  const QJsonObject &metadata)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("metadata"), metadata);
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/chat/completions/") + completionId;
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new ChatCompletionReply(std::move(factory), d->retryPolicy);
}

ChatCompletionReply *Client::deleteChatCompletion(const QString &completionId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/chat/completions/") + completionId;
    auto factory = deleteFactory(manager, apiRequest(d, path));
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
    auto factory = getFactory(manager, req);
    return new ChatCompletionMessageListReply(std::move(factory), d->retryPolicy);
}

EmbeddingReply *Client::createEmbeddings(const Core::EmbeddingRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, QStringLiteral("/embeddings")), body);
    return new EmbeddingReply(std::move(factory), d->retryPolicy);
}

TranscriptionReply *Client::createTranscription(const Core::TranscriptionRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"file", request.fileName(), request.fileData()};
    auto factory = multipartPostFactory(d, networkAccessManager(),
                                        apiRequest(d, QStringLiteral("/audio/transcriptions")),
                                        request.formFields(), {std::move(file)});
    return new TranscriptionReply(std::move(factory), d->retryPolicy);
}

TranscriptionReply *Client::createTranslation(const Core::TranslationRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"file", request.fileName(), request.fileData()};
    auto factory = multipartPostFactory(d, networkAccessManager(),
                                        apiRequest(d, QStringLiteral("/audio/translations")),
                                        request.formFields(), {std::move(file)});
    return new TranscriptionReply(std::move(factory), d->retryPolicy);
}

ImageReply *Client::createImage(const Core::ImageGenerationRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory
            = postFactory(d, manager, apiRequest(d, QStringLiteral("/images/generations")), body);
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

    auto factory = multipartPostFactory(d, networkAccessManager(),
                                        apiRequest(d, QStringLiteral("/images/edits")),
                                        request.formFields(), std::move(files));
    return new ImageReply(std::move(factory), d->retryPolicy);
}

ImageReply *Client::createImageVariation(const Core::ImageVariationRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"image", request.fileName(), request.imageData()};
    auto factory = multipartPostFactory(d, networkAccessManager(),
                                        apiRequest(d, QStringLiteral("/images/variations")),
                                        request.formFields(), {std::move(file)});
    return new ImageReply(std::move(factory), d->retryPolicy);
}

VideoReply *Client::createVideo(const Core::CreateVideoRequest &request)
{
    Q_D(Client);
    // A JSON body suffices unless a reference file must be uploaded, in which
    // case the request must go out as multipart/form-data.
    if (request.hasInputReference()) {
        detail::FormFilePart file {"input_reference", request.inputReferenceFileName(),
                                   request.inputReferenceData()};
        auto factory = multipartPostFactory(d, networkAccessManager(),
                                            apiRequest(d, QStringLiteral("/videos")),
                                            request.formFields(), {std::move(file)});
        return new VideoReply(std::move(factory), d->retryPolicy);
    }
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, QStringLiteral("/videos")), body);
    return new VideoReply(std::move(factory), d->retryPolicy);
}

VideoReply *Client::getVideo(const QString &videoId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/videos/") + videoId;
    auto factory = getFactory(manager, apiRequest(d, path));
    return new VideoReply(std::move(factory), d->retryPolicy);
}

VideoListReply *Client::listVideos(const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, QStringLiteral("/videos"));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new VideoListReply(std::move(factory), d->retryPolicy);
}

VideoReply *Client::deleteVideo(const QString &videoId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/videos/") + videoId;
    auto factory = deleteFactory(manager, apiRequest(d, path));
    return new VideoReply(std::move(factory), d->retryPolicy);
}

VideoReply *Client::remixVideo(const QString &videoId, const QString &prompt)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("prompt"), prompt);
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/videos/") + videoId + QStringLiteral("/remix");
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new VideoReply(std::move(factory), d->retryPolicy);
}

VideoContentReply *Client::downloadVideoContent(const QString &videoId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/videos/") + videoId + QStringLiteral("/content");
    auto factory = getFactory(manager, apiRequest(d, path));
    return new VideoContentReply(std::move(factory), d->retryPolicy);
}

VideoPoller *Client::pollVideo(const QString &videoId, int pollIntervalMs)
{
    return new VideoPoller(this, videoId, pollIntervalMs);
}

SpeechReply *Client::createSpeech(const Core::SpeechRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, QStringLiteral("/audio/speech")), body);
    return new SpeechReply(std::move(factory), d->retryPolicy);
}

FileReply *Client::uploadFile(const Core::FileUploadRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"file", request.fileName(), request.fileData()};
    auto factory = multipartPostFactory(d, networkAccessManager(),
                                        apiRequest(d, QStringLiteral("/files")),
                                        request.formFields(), {std::move(file)});
    return new FileReply(std::move(factory), d->retryPolicy);
}

FileListReply *Client::listFiles(const ListParams &params, const QString &purpose)
{
    Q_D(Client);
    QUrlQuery query = params.toQuery();
    if (!purpose.isEmpty())
        query.addQueryItem(QStringLiteral("purpose"), purpose);
    QNetworkRequest req = apiRequest(d, QStringLiteral("/files"));
    applyQuery(req, query);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new FileListReply(std::move(factory), d->retryPolicy);
}

FileReply *Client::getFile(const QString &fileId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/files/") + fileId;
    auto factory = getFactory(manager, apiRequest(d, path));
    return new FileReply(std::move(factory), d->retryPolicy);
}

FileReply *Client::deleteFile(const QString &fileId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/files/") + fileId;
    auto factory = deleteFactory(manager, apiRequest(d, path));
    return new FileReply(std::move(factory), d->retryPolicy);
}

BinaryReply *Client::downloadFileContent(const QString &fileId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/files/") + fileId + QStringLiteral("/content");
    auto factory = getFactory(manager, apiRequest(d, path));
    return new BinaryReply(std::move(factory), d->retryPolicy);
}

UploadReply *Client::createUpload(const Core::CreateUploadRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, QStringLiteral("/uploads")), body);
    return new UploadReply(std::move(factory), d->retryPolicy);
}

UploadPartReply *Client::addUploadPart(const QString &uploadId, const QByteArray &data)
{
    Q_D(Client);
    // The chunk is the multipart `data` part; the filename is cosmetic here.
    detail::FormFilePart part {"data", QStringLiteral("part"), data};
    const QString path = QStringLiteral("/uploads/") + uploadId + QStringLiteral("/parts");
    auto factory = multipartPostFactory(d, networkAccessManager(), apiRequest(d, path), {},
                                        {std::move(part)});
    return new UploadPartReply(std::move(factory), d->retryPolicy);
}

UploadReply *Client::completeUpload(const QString &uploadId, const QStringList &partIds,
                                    const QString &md5)
{
    Q_D(Client);
    QJsonObject bodyObject;
    QJsonArray ids;
    for (const QString &partId : partIds)
        ids.append(partId);
    bodyObject.insert(QStringLiteral("part_ids"), ids);
    if (!md5.isEmpty())
        bodyObject.insert(QStringLiteral("md5"), md5);
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/uploads/") + uploadId + QStringLiteral("/complete");
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new UploadReply(std::move(factory), d->retryPolicy);
}

UploadReply *Client::cancelUpload(const QString &uploadId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/uploads/") + uploadId + QStringLiteral("/cancel");
    auto factory = postFactory(d, manager, apiRequest(d, path));
    return new UploadReply(std::move(factory), d->retryPolicy);
}

ChunkedUploader *Client::uploadInChunks(const Core::CreateUploadRequest &request, QIODevice *source,
                                        qint64 chunkSize)
{
    return new ChunkedUploader(this, request, source, /*ownsSource=*/false, chunkSize);
}

ChunkedUploader *Client::uploadInChunks(const Core::CreateUploadRequest &request,
                                        const QByteArray &data, qint64 chunkSize)
{
    // Wrap the payload so both overloads share one device-based implementation;
    // the buffer is adopted by the uploader and freed with it.
    auto *buffer = new QBuffer;
    buffer->setData(data);
    return new ChunkedUploader(this, request, buffer, /*ownsSource=*/true, chunkSize);
}

VectorStoreReply *Client::createVectorStore(const Core::CreateVectorStoreRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, vectorStorePath({})), body);
    return new VectorStoreReply(std::move(factory), d->retryPolicy);
}

VectorStoreListReply *Client::listVectorStores(const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, vectorStorePath({}));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new VectorStoreListReply(std::move(factory), d->retryPolicy);
}

VectorStoreReply *Client::getVectorStore(const QString &vectorStoreId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, apiRequest(d, vectorStorePath(vectorStoreId)));
    return new VectorStoreReply(std::move(factory), d->retryPolicy);
}

VectorStoreReply *Client::updateVectorStore(const QString &vectorStoreId,
                                            const Core::CreateVectorStoreRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, vectorStorePath(vectorStoreId)), body);
    return new VectorStoreReply(std::move(factory), d->retryPolicy);
}

VectorStoreReply *Client::deleteVectorStore(const QString &vectorStoreId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = deleteFactory(manager, apiRequest(d, vectorStorePath(vectorStoreId)));
    return new VectorStoreReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileReply *Client::createVectorStoreFile(const QString &vectorStoreId,
                                                    const QString &fileId,
                                                    const QJsonObject &chunkingStrategy,
                                                    const QJsonObject &attributes)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("file_id"), fileId);
    if (!chunkingStrategy.isEmpty())
        bodyObject.insert(QStringLiteral("chunking_strategy"), chunkingStrategy);
    if (!attributes.isEmpty())
        bodyObject.insert(QStringLiteral("attributes"), attributes);
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/files"));
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new VectorStoreFileReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileListReply *Client::listVectorStoreFiles(const QString &vectorStoreId,
                                                       const ListParams &params,
                                                       const QString &filter)
{
    Q_D(Client);
    QUrlQuery query = params.toQuery();
    if (!filter.isEmpty())
        query.addQueryItem(QStringLiteral("filter"), filter);
    QNetworkRequest req = apiRequest(d, vectorStorePath(vectorStoreId, QStringLiteral("/files")));
    applyQuery(req, query);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new VectorStoreFileListReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileReply *Client::getVectorStoreFile(const QString &vectorStoreId,
                                                 const QString &fileId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/files/") + fileId);
    auto factory = getFactory(manager, apiRequest(d, path));
    return new VectorStoreFileReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileReply *Client::updateVectorStoreFileAttributes(const QString &vectorStoreId,
                                                              const QString &fileId,
                                                              const QJsonObject &attributes)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("attributes"), attributes);
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/files/") + fileId);
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new VectorStoreFileReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileReply *Client::deleteVectorStoreFile(const QString &vectorStoreId,
                                                    const QString &fileId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/files/") + fileId);
    auto factory = deleteFactory(manager, apiRequest(d, path));
    return new VectorStoreFileReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileContentReply *Client::getVectorStoreFileContent(const QString &vectorStoreId,
                                                               const QString &fileId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/files/") + fileId
                                                                + QStringLiteral("/content"));
    auto factory = getFactory(manager, apiRequest(d, path));
    return new VectorStoreFileContentReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileBatchReply *Client::createVectorStoreFileBatch(const QString &vectorStoreId,
                                                              const QStringList &fileIds,
                                                              const QJsonObject &chunkingStrategy,
                                                              const QJsonObject &attributes)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("file_ids"), idsToArray(fileIds));
    if (!chunkingStrategy.isEmpty())
        bodyObject.insert(QStringLiteral("chunking_strategy"), chunkingStrategy);
    if (!attributes.isEmpty())
        bodyObject.insert(QStringLiteral("attributes"), attributes);
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/file_batches"));
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new VectorStoreFileBatchReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileBatchReply *Client::getVectorStoreFileBatch(const QString &vectorStoreId,
                                                           const QString &batchId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/file_batches/") + batchId);
    auto factory = getFactory(manager, apiRequest(d, path));
    return new VectorStoreFileBatchReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileBatchReply *Client::cancelVectorStoreFileBatch(const QString &vectorStoreId,
                                                              const QString &batchId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/file_batches/") + batchId
                                                                + QStringLiteral("/cancel"));
    auto factory = postFactory(d, manager, apiRequest(d, path));
    return new VectorStoreFileBatchReply(std::move(factory), d->retryPolicy);
}

VectorStoreFileListReply *Client::listVectorStoreFileBatchFiles(const QString &vectorStoreId,
                                                                const QString &batchId,
                                                                const ListParams &params,
                                                                const QString &filter)
{
    Q_D(Client);
    QUrlQuery query = params.toQuery();
    if (!filter.isEmpty())
        query.addQueryItem(QStringLiteral("filter"), filter);
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/file_batches/") + batchId
                                                                + QStringLiteral("/files"));
    QNetworkRequest req = apiRequest(d, path);
    applyQuery(req, query);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new VectorStoreFileListReply(std::move(factory), d->retryPolicy);
}

VectorStoreSearchReply *Client::searchVectorStore(const QString &vectorStoreId,
                                                  const Core::VectorStoreSearchRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = vectorStorePath(vectorStoreId, QStringLiteral("/search"));
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new VectorStoreSearchReply(std::move(factory), d->retryPolicy);
}

ContainerReply *Client::createContainer(const Core::CreateContainerRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, containerPath({})), body);
    return new ContainerReply(std::move(factory), d->retryPolicy);
}

ContainerListReply *Client::listContainers(const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, containerPath({}));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new ContainerListReply(std::move(factory), d->retryPolicy);
}

ContainerReply *Client::getContainer(const QString &containerId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, apiRequest(d, containerPath(containerId)));
    return new ContainerReply(std::move(factory), d->retryPolicy);
}

ContainerReply *Client::deleteContainer(const QString &containerId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = deleteFactory(manager, apiRequest(d, containerPath(containerId)));
    return new ContainerReply(std::move(factory), d->retryPolicy);
}

ContainerFileReply *Client::uploadContainerFile(const QString &containerId, const QString &fileName,
                                                const QByteArray &data)
{
    Q_D(Client);
    detail::FormFilePart file {"file", fileName, data};
    const QString path = containerPath(containerId, QStringLiteral("/files"));
    auto factory = multipartPostFactory(d, networkAccessManager(), apiRequest(d, path), {},
                                        {std::move(file)});
    return new ContainerFileReply(std::move(factory), d->retryPolicy);
}

ContainerFileReply *Client::attachContainerFile(const QString &containerId, const QString &fileId)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("file_id"), fileId);
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = containerPath(containerId, QStringLiteral("/files"));
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new ContainerFileReply(std::move(factory), d->retryPolicy);
}

ContainerFileListReply *Client::listContainerFiles(const QString &containerId,
                                                   const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, containerPath(containerId, QStringLiteral("/files")));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new ContainerFileListReply(std::move(factory), d->retryPolicy);
}

ContainerFileReply *Client::getContainerFile(const QString &containerId, const QString &fileId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = containerPath(containerId, QStringLiteral("/files/") + fileId);
    auto factory = getFactory(manager, apiRequest(d, path));
    return new ContainerFileReply(std::move(factory), d->retryPolicy);
}

ContainerFileReply *Client::deleteContainerFile(const QString &containerId, const QString &fileId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = containerPath(containerId, QStringLiteral("/files/") + fileId);
    auto factory = deleteFactory(manager, apiRequest(d, path));
    return new ContainerFileReply(std::move(factory), d->retryPolicy);
}

BinaryReply *Client::downloadContainerFileContent(const QString &containerId, const QString &fileId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = containerPath(containerId, QStringLiteral("/files/") + fileId
                                                            + QStringLiteral("/content"));
    auto factory = getFactory(manager, apiRequest(d, path));
    return new BinaryReply(std::move(factory), d->retryPolicy);
}

BatchReply *Client::createBatch(const Core::CreateBatchRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, QStringLiteral("/batches")), body);
    return new BatchReply(std::move(factory), d->retryPolicy);
}

BatchListReply *Client::listBatches(const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, QStringLiteral("/batches"));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new BatchListReply(std::move(factory), d->retryPolicy);
}

BatchReply *Client::getBatch(const QString &batchId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/batches/") + batchId;
    auto factory = getFactory(manager, apiRequest(d, path));
    return new BatchReply(std::move(factory), d->retryPolicy);
}

BatchReply *Client::cancelBatch(const QString &batchId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/batches/") + batchId + QStringLiteral("/cancel");
    auto factory = postFactory(d, manager, apiRequest(d, path));
    return new BatchReply(std::move(factory), d->retryPolicy);
}

BatchPoller *Client::pollBatch(const QString &batchId, int pollIntervalMs)
{
    return new BatchPoller(this, batchId, pollIntervalMs);
}

FineTuningJobReply *Client::createFineTuningJob(const Core::CreateFineTuningJobRequest &request)
{
    Q_D(Client);
    const QByteArray body = compactJson(request.toJson());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, fineTuningJobPath({})), body);
    return new FineTuningJobReply(std::move(factory), d->retryPolicy);
}

FineTuningJobListReply *Client::listFineTuningJobs(const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, fineTuningJobPath({}));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new FineTuningJobListReply(std::move(factory), d->retryPolicy);
}

FineTuningJobReply *Client::getFineTuningJob(const QString &jobId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, apiRequest(d, fineTuningJobPath(jobId)));
    return new FineTuningJobReply(std::move(factory), d->retryPolicy);
}

FineTuningJobReply *Client::cancelFineTuningJob(const QString &jobId)
{
    return postFineTuningJobAction(jobId, QStringLiteral("/cancel"));
}

FineTuningJobReply *Client::pauseFineTuningJob(const QString &jobId)
{
    return postFineTuningJobAction(jobId, QStringLiteral("/pause"));
}

FineTuningJobReply *Client::resumeFineTuningJob(const QString &jobId)
{
    return postFineTuningJobAction(jobId, QStringLiteral("/resume"));
}

FineTuningJobReply *Client::postFineTuningJobAction(const QString &jobId, const QString &action)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = postFactory(d, manager, apiRequest(d, fineTuningJobPath(jobId, action)));
    return new FineTuningJobReply(std::move(factory), d->retryPolicy);
}

FineTuningEventListReply *Client::listFineTuningEvents(const QString &jobId,
                                                       const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, fineTuningJobPath(jobId, QStringLiteral("/events")));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new FineTuningEventListReply(std::move(factory), d->retryPolicy);
}

FineTuningCheckpointListReply *Client::listFineTuningCheckpoints(const QString &jobId,
                                                                 const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req = apiRequest(d, fineTuningJobPath(jobId, QStringLiteral("/checkpoints")));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new FineTuningCheckpointListReply(std::move(factory), d->retryPolicy);
}

FineTuningJobPoller *Client::pollFineTuningJob(const QString &jobId, int pollIntervalMs)
{
    return new FineTuningJobPoller(this, jobId, pollIntervalMs);
}

FineTuningPermissionListReply *
Client::listFineTuningCheckpointPermissions(const QString &checkpointId, const ListParams &params)
{
    Q_D(Client);
    QNetworkRequest req
            = apiRequest(d, fineTuningCheckpointPath(checkpointId, QStringLiteral("/permissions")));
    applyQuery(req, params.toQuery());
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, req);
    return new FineTuningPermissionListReply(std::move(factory), d->retryPolicy);
}

FineTuningPermissionListReply *
Client::createFineTuningCheckpointPermissions(const QString &checkpointId,
                                              const QStringList &projectIds)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("project_ids"), idsToArray(projectIds));
    const QByteArray body = compactJson(bodyObject);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = fineTuningCheckpointPath(checkpointId, QStringLiteral("/permissions"));
    auto factory = postFactory(d, manager, apiRequest(d, path), body);
    return new FineTuningPermissionListReply(std::move(factory), d->retryPolicy);
}

FineTuningPermissionReply *Client::deleteFineTuningCheckpointPermission(const QString &checkpointId,
                                                                        const QString &permissionId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = fineTuningCheckpointPath(checkpointId,
                                                  QStringLiteral("/permissions/") + permissionId);
    auto factory = deleteFactory(manager, apiRequest(d, path));
    return new FineTuningPermissionReply(std::move(factory), d->retryPolicy);
}

ModelListReply *Client::listModels()
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    auto factory = getFactory(manager, apiRequest(d, QStringLiteral("/models")));
    return new ModelListReply(std::move(factory), d->retryPolicy);
}

ModelReply *Client::getModel(const QString &modelId)
{
    Q_D(Client);
    QNetworkAccessManager *manager = networkAccessManager();
    const QString path = QStringLiteral("/models/") + modelId;
    auto factory = getFactory(manager, apiRequest(d, path));
    return new ModelReply(std::move(factory), d->retryPolicy);
}

} // namespace Client
} // namespace QtOpenAi
