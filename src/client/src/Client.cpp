// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/Client.h"

#include "QtOpenAi/Client/Interceptor.h"
#include "QtOpenAi/Client/ProviderProfile.h"

#include "CannedReply_p.h"
#include "Multipart_p.h"

#include <QtCore/QBuffer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QUrlQuery>
#include <QtCore/QUuid>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <memory>

namespace QtOpenAi {
namespace Client {

namespace {
constexpr auto kDefaultBaseUrl = "https://api.openai.com/v1";
}

class ClientPrivate
{
public:
    explicit ClientPrivate(Client *client)
        : q(client)
    { }

    Client *q;
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
    QList<Interceptor *> interceptors;
    QNetworkAccessManager *manager = nullptr;

    // Issue a request against `path` and wrap it in the reply type the endpoint
    // returns. Every endpoint method below is one of these four calls: they hold
    // the plumbing -- build the request, apply the query, capture a retry
    // factory, hand it the configured RetryPolicy -- that is identical for all
    // ~165 of them and easy to get subtly wrong when spelled out each time.
    // Defined below the request helpers they use.
    //
    // `beta` is the value of the OpenAI-Beta header an endpoint family behind a
    // beta flag has to send (null for the stable majority). It rides along here
    // rather than at the call sites so no endpoint of such a family can forget
    // it.
    template <typename Reply>
    Reply *get(const QString &path, const QUrlQuery &query = {}, const char *beta = nullptr) const;
    template <typename Reply>
    Reply *post(const QString &path, const QByteArray &body = {}, const char *beta = nullptr) const;
    template <typename Reply>
    Reply *postMultipart(const QString &path, QList<QPair<QString, QString>> fields,
                         QList<detail::FormFilePart> files, const char *beta = nullptr) const;
    template <typename Reply>
    Reply *remove(const QString &path, const char *beta = nullptr) const;
    template <typename Reply, typename Request>
    Reply *postStream(const QString &path, Request request, const char *beta = nullptr) const;

    // The interceptor round trip for one non-streaming call, and the single
    // place it happens: run the outgoing chain, take either the network or an
    // interceptor's answer, and hook the returning chain to the reply.
    // `makeFactory` turns the request the chain left behind into the retry
    // factory for the verb in question -- the only part that differs.
    template <typename Reply, typename MakeFactory>
    Reply *issue(const QByteArray &method, QNetworkRequest request, const QByteArray &body,
                 MakeFactory makeFactory) const;

    // The outgoing half on its own, for the streaming path.
    std::optional<InterceptedResponse> runBeforeRequest(InterceptedRequest &request) const;
    // The returning half, hung off the reply that is about to run.
    void runAfterResponse(RestReplyBase *reply, const InterceptedRequest &sent,
                          bool fromCache) const;

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
    , d_ptr(new ClientPrivate(this))
{ }

Client::Client(QUrl baseUrl, QString apiKey, QObject *parent)
    : QObject(parent)
    , d_ptr(new ClientPrivate(this))
{
    Q_D(Client);
    d->baseUrl = std::move(baseUrl);
    d->apiKey = std::move(apiKey);
}

// A manager the Client created is a child of it and dies with it; one the caller
// installed belongs to the caller. Either way there is nothing to free here.
Client::~Client() = default;

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

void Client::setProfile(const ProviderProfile &profile)
{
    // The profile knows what it configures; this is the same call from the
    // side the caller is more likely to be holding.
    profile.applyTo(this);
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

void Client::addInterceptor(Interceptor *interceptor)
{
    Q_D(Client);
    if (!interceptor || d->interceptors.contains(interceptor))
        return;
    d->interceptors.append(interceptor);
    // The caller keeps ownership, so the client has to survive the interceptor
    // outliving its usefulness -- a dangling entry here would be called on the
    // next request.
    connect(interceptor, &QObject::destroyed, this,
            [this, interceptor]() { d_func()->interceptors.removeOne(interceptor); });
}

void Client::removeInterceptor(Interceptor *interceptor)
{
    Q_D(Client);
    if (d->interceptors.removeOne(interceptor))
        disconnect(interceptor, &QObject::destroyed, this, nullptr);
}

QList<Interceptor *> Client::interceptors() const
{
    Q_D(const Client);
    return d->interceptors;
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
    d->manager = manager;
}

QNetworkAccessManager *Client::networkAccessManager() const
{
    Q_D(const Client);
    if (!d->manager) {
        // Lazily create one on first use, parented to the Client so it is freed
        // with it.
        const_cast<ClientPrivate *>(d)->manager
                = new QNetworkAccessManager(const_cast<Client *>(this));
    }
    return d->manager;
}

namespace {

// Build a network request for an endpoint path (URL + auth/content/custom
// headers + timeout), applying the configured auth scheme. `beta` names the
// beta an endpoint family speaks, sent as the OpenAI-Beta header; null for the
// stable endpoints.
QNetworkRequest apiRequest(const ClientPrivate *d, const QString &path, const char *beta = nullptr)
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
    if (beta)
        networkRequest.setRawHeader("OpenAI-Beta", beta);
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
// The collection segment of every endpoint family. Each spelling lives here
// once, so a path is composed rather than re-typed at each call site.
constexpr QLatin1String kChatCompletions("/chat/completions");
constexpr QLatin1String kResponses("/responses");
constexpr QLatin1String kConversations("/conversations");
constexpr QLatin1String kVideos("/videos");
constexpr QLatin1String kFiles("/files");
constexpr QLatin1String kUploads("/uploads");
constexpr QLatin1String kVectorStores("/vector_stores");
constexpr QLatin1String kContainers("/containers");
constexpr QLatin1String kBatches("/batches");
constexpr QLatin1String kFineTuningJobs("/fine_tuning/jobs");
constexpr QLatin1String kFineTuningCheckpoints("/fine_tuning/checkpoints");
constexpr QLatin1String kEvals("/evals");
constexpr QLatin1String kRuns("/runs");
constexpr QLatin1String kAssistants("/assistants");
constexpr QLatin1String kThreads("/threads");
constexpr QLatin1String kMessages("/messages");
constexpr QLatin1String kSteps("/steps");
constexpr QLatin1String kVoiceConsents("/audio/voice_consents");
constexpr QLatin1String kModels("/models");
constexpr QLatin1String kCompletions("/completions");
constexpr QLatin1String kSkills("/skills");
constexpr QLatin1String kRealtime("/realtime");
constexpr QLatin1String kRealtimeCalls("/realtime/calls");
constexpr QLatin1String kRealtimeClientSecrets("/realtime/client_secrets");
constexpr QLatin1String kChatKitSessions("/chatkit/sessions");
constexpr QLatin1String kChatKitThreads("/chatkit/threads");
// The sub-resource the item-bearing collections hang their entries off.
constexpr QLatin1String kItems("/items");
constexpr QLatin1String kVersions("/versions");
// The sub-resource every downloadable object hangs its bytes off.
constexpr QLatin1String kContent("/content");

// The Assistants surface is still a beta of its own, and the API rejects a
// request that does not say which version it speaks.
constexpr auto kAssistantsBeta = "assistants=v2";

// ChatKit is a beta of its own, with its own header value.
constexpr auto kChatKitBeta = "chatkit_beta=v1";

QString resourcePath(QLatin1String collection, const QString &id, const QString &suffix = {})
{
    QString path(collection);
    if (!id.isEmpty())
        path += QLatin1Char('/') + id;
    return path + suffix;
}

// Runs nest below an eval, so their paths compose two levels of resourcePath().
QString evalRunPath(const QString &evalId, const QString &runId, const QString &suffix = {})
{
    return resourcePath(kEvals, evalId, resourcePath(kRuns, runId, suffix));
}

// Assistant runs nest below a thread, the same two levels deep.
QString threadRunPath(const QString &threadId, const QString &runId, const QString &suffix = {})
{
    return resourcePath(kThreads, threadId, resourcePath(kRuns, runId, suffix));
}

// Skill versions nest below a skill. The version is a number the API spells as
// a path segment, not an object id, but it composes exactly the same way.
QString skillVersionPath(const QString &skillId, const QString &version, const QString &suffix = {})
{
    return resourcePath(kSkills, skillId, resourcePath(kVersions, version, suffix));
}

// Realtime SIP call control hangs four verbs off one call.
QString realtimeCallPath(const QString &callId, const QString &suffix)
{
    return resourcePath(kRealtimeCalls, callId, suffix);
}

// The file parts of a skill upload. A bundle is either a single zip or one part
// per file of a directory; both travel under the same `files` field name, which
// is what makes the two forms one request rather than two.
QList<detail::FormFilePart> skillFileParts(const Core::CreateSkillRequest &request)
{
    QList<detail::FormFilePart> parts;
    const QList<Core::CreateSkillRequest::SkillFile> files = request.files();
    parts.reserve(files.size());
    for (const auto &file : files)
        parts.append({"files", file.first, file.second});
    return parts;
}

// The body of a submit_tool_outputs call. Both the blocking and the streamed
// variant post it, and postStream() needs a request object it can flip the
// `stream` flag on -- which is all this is.
class ToolOutputsBody
{
public:
    explicit ToolOutputsBody(const QList<Core::ToolOutput> &outputs)
        : m_outputs(outputs)
    { }

    void setStream(bool stream) { m_stream = stream; }

    QJsonObject toJson() const
    {
        QJsonArray array;
        for (const Core::ToolOutput &output : m_outputs)
            array.append(output.toJson());
        QJsonObject json;
        json.insert(QStringLiteral("tool_outputs"), array);
        if (m_stream)
            json.insert(QStringLiteral("stream"), true);
        return json;
    }

private:
    QList<Core::ToolOutput> m_outputs;
    bool m_stream = false;
};

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

// The body of the several "modify" endpoints whose only field is `metadata`.
// They span four resource families that otherwise share nothing, so the shape
// is spelled here rather than four times over.
QByteArray metadataBody(const QJsonObject &metadata)
{
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("metadata"), metadata);
    return compactJson(bodyObject);
}

// The body both client-secret endpoints take: the session to start from, plus
// an optional lifetime for the secret itself. The anchor is a constant in the
// API, so it travels with the seconds rather than being a second argument.
QByteArray clientSecretBody(const Core::RealtimeSessionConfig &session, qint64 expiresAfterSeconds)
{
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("session"), session.toJson());
    if (expiresAfterSeconds > 0) {
        bodyObject.insert(QStringLiteral("expires_after"),
                          QJsonObject {{QStringLiteral("anchor"), QStringLiteral("created_at")},
                                       {QStringLiteral("seconds"), expiresAfterSeconds}});
    }
    return compactJson(bodyObject);
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

std::optional<InterceptedResponse>
ClientPrivate::runBeforeRequest(InterceptedRequest &request) const
{
    // The one check an installed-nothing client pays for.
    for (Interceptor *interceptor : interceptors) {
        if (std::optional<InterceptedResponse> answer = interceptor->beforeRequest(request)) {
            // An interceptor that answers without saying how is saying "200":
            // the alternative is a reply that decodes a body while reporting
            // that no response arrived.
            if (answer->httpStatus == 0)
                answer->httpStatus = 200;
            answer->request = request;
            return answer;
        }
    }
    return std::nullopt;
}

void ClientPrivate::runAfterResponse(RestReplyBase *reply, const InterceptedRequest &sent,
                                     bool fromCache) const
{
    if (interceptors.isEmpty())
        return;

    // The exchange is assembled across two signals because neither carries all
    // of it: responseReceived() has the raw body, and by done() the error is
    // recorded. Shared state rather than a member, because a Client has many
    // requests in flight and each owns its own timing.
    auto exchange = std::make_shared<InterceptedResponse>();
    exchange->request = sent;
    exchange->fromCache = fromCache;
    auto elapsed = std::make_shared<QElapsedTimer>();
    elapsed->start();

    QObject::connect(reply, &RestReplyBase::responseReceived, q,
                     [exchange](const QByteArray &body, int httpStatus) {
                         exchange->body = body;
                         exchange->httpStatus = httpStatus;
                     });
    QObject::connect(reply, &RestReplyBase::done, q, [this, reply, exchange, elapsed]() {
        exchange->elapsedMs = elapsed->elapsed();
        exchange->error = reply->error();
        // Reverse order, so a chain nests around the exchange: the first
        // interceptor installed is the outermost and sees it last.
        for (auto it = interceptors.crbegin(); it != interceptors.crend(); ++it)
            (*it)->afterResponse(*exchange);
    });
}

template <typename Reply, typename MakeFactory>
Reply *ClientPrivate::issue(const QByteArray &method, QNetworkRequest request,
                            const QByteArray &body, MakeFactory makeFactory) const
{
    InterceptedRequest outgoing {method, std::move(request), body};
    const std::optional<InterceptedResponse> answer = runBeforeRequest(outgoing);

    std::function<QNetworkReply *()> factory;
    if (answer) {
        // Answered locally. It still becomes a QNetworkReply, so everything
        // below -- retry engine, rate-limit parsing, typed decoding -- runs
        // exactly as it does for a real response instead of via a second path
        // that would have to be kept in step with the first.
        factory = [sent = outgoing.request, answered = *answer]() {
            return new CannedReply(sent, answered.body, answered.httpStatus, "application/json");
        };
    } else {
        factory = makeFactory(outgoing.request);
    }

    Reply *reply = Client::makeReply<Reply>(q, std::move(factory), retryPolicy);
    runAfterResponse(reply, outgoing, answer.has_value());
    return reply;
}

template <typename Reply>
Reply *ClientPrivate::get(const QString &path, const QUrlQuery &query, const char *beta) const
{
    QNetworkRequest request = apiRequest(this, path, beta);
    applyQuery(request, query);
    QNetworkAccessManager *manager = q->networkAccessManager();
    return issue<Reply>("GET", std::move(request), {}, [manager](QNetworkRequest sent) {
        return getFactory(manager, std::move(sent));
    });
}

template <typename Reply>
Reply *ClientPrivate::post(const QString &path, const QByteArray &body, const char *beta) const
{
    QNetworkAccessManager *manager = q->networkAccessManager();
    return issue<Reply>("POST", apiRequest(this, path, beta), body,
                        [this, manager, body](QNetworkRequest sent) {
                            return postFactory(this, manager, std::move(sent), body);
                        });
}

template <typename Reply>
Reply *ClientPrivate::postMultipart(const QString &path, QList<QPair<QString, QString>> fields,
                                    QList<detail::FormFilePart> files, const char *beta) const
{
    QNetworkAccessManager *manager = q->networkAccessManager();
    // The body is not offered to the chain here: it is rebuilt per attempt and
    // can be a whole file, so handing it over would mean holding an upload in
    // memory for the benefit of a logger.
    return issue<Reply>("POST", apiRequest(this, path, beta), {},
                        [this, manager, fields = std::move(fields),
                         files = std::move(files)](QNetworkRequest sent) mutable {
                            return multipartPostFactory(this, manager, std::move(sent),
                                                        std::move(fields), std::move(files));
                        });
}

template <typename Reply>
Reply *ClientPrivate::remove(const QString &path, const char *beta) const
{
    QNetworkAccessManager *manager = q->networkAccessManager();
    return issue<Reply>(
            "DELETE", apiRequest(this, path, beta), {},
            [manager](QNetworkRequest sent) { return deleteFactory(manager, std::move(sent)); });
}

// The streaming endpoints deliberately sit outside the retry machinery: an SSE
// response is consumed incrementally, so a failure part-way through cannot be
// replayed from the start. `request` is taken by value because streaming has to
// be forced on -- on our copy, leaving the caller's request untouched.
template <typename Reply, typename Request>
Reply *ClientPrivate::postStream(const QString &path, Request request, const char *beta) const
{
    request.setStream(true);
    QNetworkRequest networkRequest = apiRequest(this, path, beta);
    networkRequest.setRawHeader("Accept", "text/event-stream");
    const QByteArray body = compactJson(request.toJson());

    // Only the outgoing half of the chain runs here, so header injection and
    // logging cover streams too. There is deliberately no returning half and no
    // short-circuit: a stream has no single response body to hand an
    // interceptor, and serving one from a cache would mean fabricating a
    // sequence of events that never happened. An answer returned here is
    // ignored rather than quietly changing what a stream is.
    InterceptedRequest outgoing {"POST", std::move(networkRequest), body};
    runBeforeRequest(outgoing);

    return Client::makeReply<Reply>(q, q->networkAccessManager()->post(outgoing.request, body));
}

ChatCompletionReply *Client::createChatCompletion(const Core::ChatCompletionRequest &request)
{
    Q_D(Client);
    return d->post<ChatCompletionReply>(kChatCompletions, compactJson(request.toJson()));
}

ModerationReply *Client::createModeration(const Core::ModerationRequest &request)
{
    Q_D(Client);
    return d->post<ModerationReply>(QStringLiteral("/moderations"), compactJson(request.toJson()));
}

CompletionReply *Client::createCompletion(const Core::CompletionRequest &request)
{
    Q_D(Client);
    return d->post<CompletionReply>(QStringLiteral("/completions"), compactJson(request.toJson()));
}

CompletionStreamReply *Client::createCompletionStream(const Core::CompletionRequest &request)
{
    Q_D(Client);
    return d->postStream<CompletionStreamReply>(kCompletions, request);
}

ChatCompletionStreamReply *
Client::createChatCompletionStream(const Core::ChatCompletionRequest &request)
{
    Q_D(Client);
    return d->postStream<ChatCompletionStreamReply>(kChatCompletions, request);
}

ResponseReply *Client::createResponse(const Core::ResponseRequest &request)
{
    Q_D(Client);
    return d->post<ResponseReply>(kResponses, compactJson(request.toJson()));
}

ResponseStreamReply *Client::createResponseStream(const Core::ResponseRequest &request)
{
    Q_D(Client);
    return d->postStream<ResponseStreamReply>(kResponses, request);
}

ResponseReply *Client::getResponse(const QString &responseId)
{
    Q_D(Client);
    return d->get<ResponseReply>(resourcePath(kResponses, responseId));
}

ResponseReply *Client::cancelResponse(const QString &responseId)
{
    Q_D(Client);
    return d->post<ResponseReply>(resourcePath(kResponses, responseId, QStringLiteral("/cancel")));
}

ResponseReply *Client::deleteResponse(const QString &responseId)
{
    Q_D(Client);
    return d->remove<ResponseReply>(resourcePath(kResponses, responseId));
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
    return d->post<ConversationReply>(kConversations, compactJson(bodyObject));
}

ConversationReply *Client::getConversation(const QString &conversationId)
{
    Q_D(Client);
    return d->get<ConversationReply>(resourcePath(kConversations, conversationId));
}

ConversationReply *Client::updateConversation(const QString &conversationId,
                                              const QJsonObject &metadata)
{
    Q_D(Client);
    return d->post<ConversationReply>(resourcePath(kConversations, conversationId),
                                      metadataBody(metadata));
}

ConversationReply *Client::deleteConversation(const QString &conversationId)
{
    Q_D(Client);
    return d->remove<ConversationReply>(resourcePath(kConversations, conversationId));
}

ResponseReply *Client::compactResponse(const QString &responseId, const QJsonObject &extra)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("response_id"), responseId);
    // Caller-supplied fields fill in what this library does not model, without
    // overriding what it does.
    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it) {
        if (!bodyObject.contains(it.key()))
            bodyObject.insert(it.key(), it.value());
    }
    return d->post<ResponseReply>(QStringLiteral("/responses/compact"), compactJson(bodyObject));
}

InputTokensReply *Client::countResponseInputTokens(const Core::ResponseRequest &request)
{
    Q_D(Client);
    return d->post<InputTokensReply>(QStringLiteral("/responses/input_tokens"),
                                     compactJson(request.toJson()));
}

ConversationItemsReply *Client::listResponseInputItems(const QString &responseId,
                                                       const ListParams &params)
{
    Q_D(Client);
    return d->get<ConversationItemsReply>(
            resourcePath(kResponses, responseId, QStringLiteral("/input_items")), params.toQuery());
}

ConversationItemsReply *Client::listConversationItems(const QString &conversationId)
{
    Q_D(Client);
    return d->get<ConversationItemsReply>(resourcePath(kConversations, conversationId, kItems));
}

ConversationItemsReply *
Client::createConversationItems(const QString &conversationId,
                                const QList<Core::ResponseOutputItem> &items)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("items"), itemsToArray(items));
    return d->post<ConversationItemsReply>(resourcePath(kConversations, conversationId, kItems),
                                           compactJson(bodyObject));
}

ConversationItemsReply *Client::getConversationItem(const QString &conversationId,
                                                    const QString &itemId)
{
    Q_D(Client);
    return d->get<ConversationItemsReply>(
            resourcePath(kConversations, conversationId, QStringLiteral("/items/") + itemId));
}

ConversationReply *Client::deleteConversationItem(const QString &conversationId,
                                                  const QString &itemId)
{
    Q_D(Client);
    return d->remove<ConversationReply>(
            resourcePath(kConversations, conversationId, QStringLiteral("/items/") + itemId));
}

ChatCompletionListReply *Client::listChatCompletions(const ListParams &params)
{
    Q_D(Client);
    return d->get<ChatCompletionListReply>(kChatCompletions, params.toQuery());
}

ChatCompletionReply *Client::getChatCompletion(const QString &completionId)
{
    Q_D(Client);
    return d->get<ChatCompletionReply>(resourcePath(kChatCompletions, completionId));
}

ChatCompletionReply *Client::updateChatCompletion(const QString &completionId,
                                                  const QJsonObject &metadata)
{
    Q_D(Client);
    return d->post<ChatCompletionReply>(resourcePath(kChatCompletions, completionId),
                                        metadataBody(metadata));
}

ChatCompletionReply *Client::deleteChatCompletion(const QString &completionId)
{
    Q_D(Client);
    return d->remove<ChatCompletionReply>(resourcePath(kChatCompletions, completionId));
}

ChatCompletionMessageListReply *Client::listChatCompletionMessages(const QString &completionId,
                                                                   const ListParams &params)
{
    Q_D(Client);
    return d->get<ChatCompletionMessageListReply>(
            resourcePath(kChatCompletions, completionId, QStringLiteral("/messages")),
            params.toQuery());
}

EmbeddingReply *Client::createEmbeddings(const Core::EmbeddingRequest &request)
{
    Q_D(Client);
    return d->post<EmbeddingReply>(QStringLiteral("/embeddings"), compactJson(request.toJson()));
}

TranscriptionReply *Client::createTranscription(const Core::TranscriptionRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"file", request.fileName(), request.fileData()};
    return d->postMultipart<TranscriptionReply>(QStringLiteral("/audio/transcriptions"),
                                                request.formFields(), {std::move(file)});
}

TranscriptionReply *Client::createTranslation(const Core::TranslationRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"file", request.fileName(), request.fileData()};
    return d->postMultipart<TranscriptionReply>(QStringLiteral("/audio/translations"),
                                                request.formFields(), {std::move(file)});
}

VoiceReply *Client::createVoice(const Core::CreateVoiceRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"audio_sample", request.fileName(), request.audioSample()};
    return d->postMultipart<VoiceReply>(QStringLiteral("/audio/voices"), request.formFields(),
                                        {std::move(file)});
}

VoiceConsentReply *Client::createVoiceConsent(const Core::CreateVoiceConsentRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"recording", request.fileName(), request.recording()};
    return d->postMultipart<VoiceConsentReply>(kVoiceConsents, request.formFields(),
                                               {std::move(file)});
}

VoiceConsentListReply *Client::listVoiceConsents(const ListParams &params)
{
    Q_D(Client);
    return d->get<VoiceConsentListReply>(kVoiceConsents, params.toQuery());
}

VoiceConsentReply *Client::getVoiceConsent(const QString &consentId)
{
    Q_D(Client);
    return d->get<VoiceConsentReply>(resourcePath(kVoiceConsents, consentId));
}

VoiceConsentReply *Client::updateVoiceConsent(const QString &consentId, const QString &name)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("name"), name);
    return d->post<VoiceConsentReply>(resourcePath(kVoiceConsents, consentId),
                                      compactJson(bodyObject));
}

VoiceConsentReply *Client::deleteVoiceConsent(const QString &consentId)
{
    Q_D(Client);
    return d->remove<VoiceConsentReply>(resourcePath(kVoiceConsents, consentId));
}

ImageReply *Client::createImage(const Core::ImageGenerationRequest &request)
{
    Q_D(Client);
    return d->post<ImageReply>(QStringLiteral("/images/generations"),
                               compactJson(request.toJson()));
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

    return d->postMultipart<ImageReply>(QStringLiteral("/images/edits"), request.formFields(),
                                        std::move(files));
}

ImageReply *Client::createImageVariation(const Core::ImageVariationRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"image", request.fileName(), request.imageData()};
    return d->postMultipart<ImageReply>(QStringLiteral("/images/variations"), request.formFields(),
                                        {std::move(file)});
}

VideoReply *Client::createVideo(const Core::CreateVideoRequest &request)
{
    Q_D(Client);
    // A JSON body suffices unless a reference file must be uploaded, in which
    // case the request must go out as multipart/form-data.
    if (request.hasInputReference()) {
        detail::FormFilePart file {"input_reference", request.inputReferenceFileName(),
                                   request.inputReferenceData()};
        return d->postMultipart<VideoReply>(kVideos, request.formFields(), {std::move(file)});
    }
    return d->post<VideoReply>(kVideos, compactJson(request.toJson()));
}

VideoReply *Client::getVideo(const QString &videoId)
{
    Q_D(Client);
    return d->get<VideoReply>(resourcePath(kVideos, videoId));
}

VideoListReply *Client::listVideos(const ListParams &params)
{
    Q_D(Client);
    return d->get<VideoListReply>(kVideos, params.toQuery());
}

VideoReply *Client::deleteVideo(const QString &videoId)
{
    Q_D(Client);
    return d->remove<VideoReply>(resourcePath(kVideos, videoId));
}

VideoReply *Client::remixVideo(const QString &videoId, const QString &prompt)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("prompt"), prompt);
    return d->post<VideoReply>(resourcePath(kVideos, videoId, QStringLiteral("/remix")),
                               compactJson(bodyObject));
}

VideoContentReply *Client::downloadVideoContent(const QString &videoId)
{
    Q_D(Client);
    return d->get<VideoContentReply>(resourcePath(kVideos, videoId, kContent));
}

VideoPoller *Client::pollVideo(const QString &videoId, int pollIntervalMs)
{
    return new VideoPoller(this, videoId, pollIntervalMs);
}

SpeechReply *Client::createSpeech(const Core::SpeechRequest &request)
{
    Q_D(Client);
    return d->post<SpeechReply>(QStringLiteral("/audio/speech"), compactJson(request.toJson()));
}

FileReply *Client::uploadFile(const Core::FileUploadRequest &request)
{
    Q_D(Client);
    detail::FormFilePart file {"file", request.fileName(), request.fileData()};
    return d->postMultipart<FileReply>(kFiles, request.formFields(), {std::move(file)});
}

FileListReply *Client::listFiles(const ListParams &params, const QString &purpose)
{
    Q_D(Client);
    QUrlQuery query = params.toQuery();
    if (!purpose.isEmpty())
        query.addQueryItem(QStringLiteral("purpose"), purpose);
    return d->get<FileListReply>(kFiles, query);
}

FileReply *Client::getFile(const QString &fileId)
{
    Q_D(Client);
    return d->get<FileReply>(resourcePath(kFiles, fileId));
}

FileReply *Client::deleteFile(const QString &fileId)
{
    Q_D(Client);
    return d->remove<FileReply>(resourcePath(kFiles, fileId));
}

BinaryReply *Client::downloadFileContent(const QString &fileId)
{
    Q_D(Client);
    return d->get<BinaryReply>(resourcePath(kFiles, fileId, kContent));
}

UploadReply *Client::createUpload(const Core::CreateUploadRequest &request)
{
    Q_D(Client);
    return d->post<UploadReply>(kUploads, compactJson(request.toJson()));
}

UploadPartReply *Client::addUploadPart(const QString &uploadId, const QByteArray &data)
{
    Q_D(Client);
    // The chunk is the multipart `data` part; the filename is cosmetic here.
    detail::FormFilePart part {"data", QStringLiteral("part"), data};
    return d->postMultipart<UploadPartReply>(
            resourcePath(kUploads, uploadId, QStringLiteral("/parts")), {}, {std::move(part)});
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
    return d->post<UploadReply>(resourcePath(kUploads, uploadId, QStringLiteral("/complete")),
                                compactJson(bodyObject));
}

UploadReply *Client::cancelUpload(const QString &uploadId)
{
    Q_D(Client);
    return d->post<UploadReply>(resourcePath(kUploads, uploadId, QStringLiteral("/cancel")));
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
    return d->post<VectorStoreReply>(kVectorStores, compactJson(request.toJson()));
}

VectorStoreListReply *Client::listVectorStores(const ListParams &params)
{
    Q_D(Client);
    return d->get<VectorStoreListReply>(kVectorStores, params.toQuery());
}

VectorStoreReply *Client::getVectorStore(const QString &vectorStoreId)
{
    Q_D(Client);
    return d->get<VectorStoreReply>(resourcePath(kVectorStores, vectorStoreId));
}

VectorStoreReply *Client::updateVectorStore(const QString &vectorStoreId,
                                            const Core::CreateVectorStoreRequest &request)
{
    Q_D(Client);
    return d->post<VectorStoreReply>(resourcePath(kVectorStores, vectorStoreId),
                                     compactJson(request.toJson()));
}

VectorStoreReply *Client::deleteVectorStore(const QString &vectorStoreId)
{
    Q_D(Client);
    return d->remove<VectorStoreReply>(resourcePath(kVectorStores, vectorStoreId));
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
    return d->post<VectorStoreFileReply>(
            resourcePath(kVectorStores, vectorStoreId, QStringLiteral("/files")),
            compactJson(bodyObject));
}

VectorStoreFileListReply *Client::listVectorStoreFiles(const QString &vectorStoreId,
                                                       const ListParams &params,
                                                       const QString &filter)
{
    Q_D(Client);
    QUrlQuery query = params.toQuery();
    if (!filter.isEmpty())
        query.addQueryItem(QStringLiteral("filter"), filter);
    return d->get<VectorStoreFileListReply>(
            resourcePath(kVectorStores, vectorStoreId, QStringLiteral("/files")), query);
}

VectorStoreFileReply *Client::getVectorStoreFile(const QString &vectorStoreId,
                                                 const QString &fileId)
{
    Q_D(Client);
    return d->get<VectorStoreFileReply>(
            resourcePath(kVectorStores, vectorStoreId, resourcePath(kFiles, fileId)));
}

VectorStoreFileReply *Client::updateVectorStoreFileAttributes(const QString &vectorStoreId,
                                                              const QString &fileId,
                                                              const QJsonObject &attributes)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("attributes"), attributes);
    return d->post<VectorStoreFileReply>(
            resourcePath(kVectorStores, vectorStoreId, resourcePath(kFiles, fileId)),
            compactJson(bodyObject));
}

VectorStoreFileReply *Client::deleteVectorStoreFile(const QString &vectorStoreId,
                                                    const QString &fileId)
{
    Q_D(Client);
    return d->remove<VectorStoreFileReply>(
            resourcePath(kVectorStores, vectorStoreId, resourcePath(kFiles, fileId)));
}

VectorStoreFileContentReply *Client::getVectorStoreFileContent(const QString &vectorStoreId,
                                                               const QString &fileId)
{
    Q_D(Client);
    return d->get<VectorStoreFileContentReply>(
            resourcePath(kVectorStores, vectorStoreId, resourcePath(kFiles, fileId, kContent)));
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
    return d->post<VectorStoreFileBatchReply>(
            resourcePath(kVectorStores, vectorStoreId, QStringLiteral("/file_batches")),
            compactJson(bodyObject));
}

VectorStoreFileBatchReply *Client::getVectorStoreFileBatch(const QString &vectorStoreId,
                                                           const QString &batchId)
{
    Q_D(Client);
    return d->get<VectorStoreFileBatchReply>(
            resourcePath(kVectorStores, vectorStoreId, QStringLiteral("/file_batches/") + batchId));
}

VectorStoreFileBatchReply *Client::cancelVectorStoreFileBatch(const QString &vectorStoreId,
                                                              const QString &batchId)
{
    Q_D(Client);
    return d->post<VectorStoreFileBatchReply>(
            resourcePath(kVectorStores, vectorStoreId,
                         QStringLiteral("/file_batches/") + batchId + QStringLiteral("/cancel")));
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
    return d->get<VectorStoreFileListReply>(
            resourcePath(kVectorStores, vectorStoreId,
                         QStringLiteral("/file_batches/") + batchId + kFiles),
            query);
}

VectorStoreSearchReply *Client::searchVectorStore(const QString &vectorStoreId,
                                                  const Core::VectorStoreSearchRequest &request)
{
    Q_D(Client);
    return d->post<VectorStoreSearchReply>(
            resourcePath(kVectorStores, vectorStoreId, QStringLiteral("/search")),
            compactJson(request.toJson()));
}

ContainerReply *Client::createContainer(const Core::CreateContainerRequest &request)
{
    Q_D(Client);
    return d->post<ContainerReply>(kContainers, compactJson(request.toJson()));
}

ContainerListReply *Client::listContainers(const ListParams &params)
{
    Q_D(Client);
    return d->get<ContainerListReply>(kContainers, params.toQuery());
}

ContainerReply *Client::getContainer(const QString &containerId)
{
    Q_D(Client);
    return d->get<ContainerReply>(resourcePath(kContainers, containerId));
}

ContainerReply *Client::deleteContainer(const QString &containerId)
{
    Q_D(Client);
    return d->remove<ContainerReply>(resourcePath(kContainers, containerId));
}

ContainerFileReply *Client::uploadContainerFile(const QString &containerId, const QString &fileName,
                                                const QByteArray &data)
{
    Q_D(Client);
    detail::FormFilePart file {"file", fileName, data};
    return d->postMultipart<ContainerFileReply>(
            resourcePath(kContainers, containerId, QStringLiteral("/files")), {},
            {std::move(file)});
}

ContainerFileReply *Client::attachContainerFile(const QString &containerId, const QString &fileId)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("file_id"), fileId);
    return d->post<ContainerFileReply>(
            resourcePath(kContainers, containerId, QStringLiteral("/files")),
            compactJson(bodyObject));
}

ContainerFileListReply *Client::listContainerFiles(const QString &containerId,
                                                   const ListParams &params)
{
    Q_D(Client);
    return d->get<ContainerFileListReply>(
            resourcePath(kContainers, containerId, QStringLiteral("/files")), params.toQuery());
}

ContainerFileReply *Client::getContainerFile(const QString &containerId, const QString &fileId)
{
    Q_D(Client);
    return d->get<ContainerFileReply>(
            resourcePath(kContainers, containerId, resourcePath(kFiles, fileId)));
}

ContainerFileReply *Client::deleteContainerFile(const QString &containerId, const QString &fileId)
{
    Q_D(Client);
    return d->remove<ContainerFileReply>(
            resourcePath(kContainers, containerId, resourcePath(kFiles, fileId)));
}

BinaryReply *Client::downloadContainerFileContent(const QString &containerId, const QString &fileId)
{
    Q_D(Client);
    return d->get<BinaryReply>(
            resourcePath(kContainers, containerId, resourcePath(kFiles, fileId, kContent)));
}

BatchReply *Client::createBatch(const Core::CreateBatchRequest &request)
{
    Q_D(Client);
    return d->post<BatchReply>(kBatches, compactJson(request.toJson()));
}

BatchListReply *Client::listBatches(const ListParams &params)
{
    Q_D(Client);
    return d->get<BatchListReply>(kBatches, params.toQuery());
}

BatchReply *Client::getBatch(const QString &batchId)
{
    Q_D(Client);
    return d->get<BatchReply>(resourcePath(kBatches, batchId));
}

BatchReply *Client::cancelBatch(const QString &batchId)
{
    Q_D(Client);
    return d->post<BatchReply>(resourcePath(kBatches, batchId, QStringLiteral("/cancel")));
}

BatchPoller *Client::pollBatch(const QString &batchId, int pollIntervalMs)
{
    return new BatchPoller(this, batchId, pollIntervalMs);
}

FineTuningJobReply *Client::createFineTuningJob(const Core::CreateFineTuningJobRequest &request)
{
    Q_D(Client);
    return d->post<FineTuningJobReply>(kFineTuningJobs, compactJson(request.toJson()));
}

FineTuningJobListReply *Client::listFineTuningJobs(const ListParams &params)
{
    Q_D(Client);
    return d->get<FineTuningJobListReply>(kFineTuningJobs, params.toQuery());
}

FineTuningJobReply *Client::getFineTuningJob(const QString &jobId)
{
    Q_D(Client);
    return d->get<FineTuningJobReply>(resourcePath(kFineTuningJobs, jobId));
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
    return d->post<FineTuningJobReply>(resourcePath(kFineTuningJobs, jobId, action));
}

FineTuningEventListReply *Client::listFineTuningEvents(const QString &jobId,
                                                       const ListParams &params)
{
    Q_D(Client);
    return d->get<FineTuningEventListReply>(
            resourcePath(kFineTuningJobs, jobId, QStringLiteral("/events")), params.toQuery());
}

FineTuningCheckpointListReply *Client::listFineTuningCheckpoints(const QString &jobId,
                                                                 const ListParams &params)
{
    Q_D(Client);
    return d->get<FineTuningCheckpointListReply>(
            resourcePath(kFineTuningJobs, jobId, QStringLiteral("/checkpoints")), params.toQuery());
}

FineTuningJobPoller *Client::pollFineTuningJob(const QString &jobId, int pollIntervalMs)
{
    return new FineTuningJobPoller(this, jobId, pollIntervalMs);
}

FineTuningPermissionListReply *
Client::listFineTuningCheckpointPermissions(const QString &checkpointId, const ListParams &params)
{
    Q_D(Client);
    return d->get<FineTuningPermissionListReply>(
            resourcePath(kFineTuningCheckpoints, checkpointId, QStringLiteral("/permissions")),
            params.toQuery());
}

FineTuningPermissionListReply *
Client::createFineTuningCheckpointPermissions(const QString &checkpointId,
                                              const QStringList &projectIds)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("project_ids"), idsToArray(projectIds));
    return d->post<FineTuningPermissionListReply>(
            resourcePath(kFineTuningCheckpoints, checkpointId, QStringLiteral("/permissions")),
            compactJson(bodyObject));
}

FineTuningPermissionReply *Client::deleteFineTuningCheckpointPermission(const QString &checkpointId,
                                                                        const QString &permissionId)
{
    Q_D(Client);
    return d->remove<FineTuningPermissionReply>(resourcePath(
            kFineTuningCheckpoints, checkpointId, QStringLiteral("/permissions/") + permissionId));
}

EvalReply *Client::createEval(const Core::CreateEvalRequest &request)
{
    Q_D(Client);
    return d->post<EvalReply>(kEvals, compactJson(request.toJson()));
}

EvalListReply *Client::listEvals(const ListParams &params)
{
    Q_D(Client);
    return d->get<EvalListReply>(kEvals, params.toQuery());
}

EvalReply *Client::getEval(const QString &evalId)
{
    Q_D(Client);
    return d->get<EvalReply>(resourcePath(kEvals, evalId));
}

EvalReply *Client::updateEval(const QString &evalId, const QString &name,
                              const QJsonObject &metadata)
{
    Q_D(Client);
    QJsonObject bodyObject;
    if (!name.isEmpty())
        bodyObject.insert(QStringLiteral("name"), name);
    if (!metadata.isEmpty())
        bodyObject.insert(QStringLiteral("metadata"), metadata);
    return d->post<EvalReply>(resourcePath(kEvals, evalId), compactJson(bodyObject));
}

EvalReply *Client::deleteEval(const QString &evalId)
{
    Q_D(Client);
    return d->remove<EvalReply>(resourcePath(kEvals, evalId));
}

EvalRunReply *Client::createEvalRun(const QString &evalId,
                                    const Core::CreateEvalRunRequest &request)
{
    Q_D(Client);
    return d->post<EvalRunReply>(resourcePath(kEvals, evalId, QStringLiteral("/runs")),
                                 compactJson(request.toJson()));
}

EvalRunListReply *Client::listEvalRuns(const QString &evalId, const ListParams &params)
{
    Q_D(Client);
    return d->get<EvalRunListReply>(resourcePath(kEvals, evalId, QStringLiteral("/runs")),
                                    params.toQuery());
}

EvalRunReply *Client::getEvalRun(const QString &evalId, const QString &runId)
{
    Q_D(Client);
    return d->get<EvalRunReply>(evalRunPath(evalId, runId));
}

EvalRunReply *Client::cancelEvalRun(const QString &evalId, const QString &runId)
{
    Q_D(Client);
    // Cancelling is a bare POST to the run itself, not to a /cancel sub-path.
    return d->post<EvalRunReply>(evalRunPath(evalId, runId));
}

EvalRunReply *Client::deleteEvalRun(const QString &evalId, const QString &runId)
{
    Q_D(Client);
    return d->remove<EvalRunReply>(evalRunPath(evalId, runId));
}

EvalRunOutputItemListReply *Client::listEvalRunOutputItems(const QString &evalId,
                                                           const QString &runId,
                                                           const ListParams &params)
{
    Q_D(Client);
    return d->get<EvalRunOutputItemListReply>(
            evalRunPath(evalId, runId, QStringLiteral("/output_items")), params.toQuery());
}

EvalRunOutputItemReply *Client::getEvalRunOutputItem(const QString &evalId, const QString &runId,
                                                     const QString &outputItemId)
{
    Q_D(Client);
    return d->get<EvalRunOutputItemReply>(
            evalRunPath(evalId, runId, QStringLiteral("/output_items/") + outputItemId));
}

EvalRunPoller *Client::pollEvalRun(const QString &evalId, const QString &runId, int pollIntervalMs)
{
    return new EvalRunPoller(this, evalId, runId, pollIntervalMs);
}

AssistantReply *Client::createAssistant(const Core::CreateAssistantRequest &request)
{
    Q_D(Client);
    return d->post<AssistantReply>(kAssistants, compactJson(request.toJson()), kAssistantsBeta);
}

AssistantListReply *Client::listAssistants(const ListParams &params)
{
    Q_D(Client);
    return d->get<AssistantListReply>(kAssistants, params.toQuery(), kAssistantsBeta);
}

AssistantReply *Client::getAssistant(const QString &assistantId)
{
    Q_D(Client);
    return d->get<AssistantReply>(resourcePath(kAssistants, assistantId), {}, kAssistantsBeta);
}

AssistantReply *Client::updateAssistant(const QString &assistantId,
                                        const Core::CreateAssistantRequest &request)
{
    Q_D(Client);
    return d->post<AssistantReply>(resourcePath(kAssistants, assistantId),
                                   compactJson(request.toJson()), kAssistantsBeta);
}

AssistantReply *Client::deleteAssistant(const QString &assistantId)
{
    Q_D(Client);
    return d->remove<AssistantReply>(resourcePath(kAssistants, assistantId), kAssistantsBeta);
}

ThreadReply *Client::createThread(const Core::CreateThreadRequest &request)
{
    Q_D(Client);
    return d->post<ThreadReply>(kThreads, compactJson(request.toJson()), kAssistantsBeta);
}

ThreadReply *Client::getThread(const QString &threadId)
{
    Q_D(Client);
    return d->get<ThreadReply>(resourcePath(kThreads, threadId), {}, kAssistantsBeta);
}

ThreadReply *Client::updateThread(const QString &threadId, const QJsonObject &metadata,
                                  const QJsonObject &toolResources)
{
    Q_D(Client);
    QJsonObject bodyObject;
    if (!metadata.isEmpty())
        bodyObject.insert(QStringLiteral("metadata"), metadata);
    if (!toolResources.isEmpty())
        bodyObject.insert(QStringLiteral("tool_resources"), toolResources);
    return d->post<ThreadReply>(resourcePath(kThreads, threadId), compactJson(bodyObject),
                                kAssistantsBeta);
}

ThreadReply *Client::deleteThread(const QString &threadId)
{
    Q_D(Client);
    return d->remove<ThreadReply>(resourcePath(kThreads, threadId), kAssistantsBeta);
}

ThreadMessageReply *Client::createThreadMessage(const QString &threadId,
                                                const Core::ThreadMessageInput &message)
{
    Q_D(Client);
    return d->post<ThreadMessageReply>(resourcePath(kThreads, threadId, kMessages),
                                       compactJson(message.toJson()), kAssistantsBeta);
}

ThreadMessageListReply *Client::listThreadMessages(const QString &threadId,
                                                   const ListParams &params, const QString &runId)
{
    Q_D(Client);
    QUrlQuery query = params.toQuery();
    if (!runId.isEmpty())
        query.addQueryItem(QStringLiteral("run_id"), runId);
    return d->get<ThreadMessageListReply>(resourcePath(kThreads, threadId, kMessages), query,
                                          kAssistantsBeta);
}

ThreadMessageReply *Client::getThreadMessage(const QString &threadId, const QString &messageId)
{
    Q_D(Client);
    return d->get<ThreadMessageReply>(
            resourcePath(kThreads, threadId, resourcePath(kMessages, messageId)), {},
            kAssistantsBeta);
}

ThreadMessageReply *Client::updateThreadMessage(const QString &threadId, const QString &messageId,
                                                const QJsonObject &metadata)
{
    Q_D(Client);
    return d->post<ThreadMessageReply>(
            resourcePath(kThreads, threadId, resourcePath(kMessages, messageId)),
            metadataBody(metadata), kAssistantsBeta);
}

ThreadMessageReply *Client::deleteThreadMessage(const QString &threadId, const QString &messageId)
{
    Q_D(Client);
    return d->remove<ThreadMessageReply>(
            resourcePath(kThreads, threadId, resourcePath(kMessages, messageId)), kAssistantsBeta);
}

RunReply *Client::createRun(const QString &threadId, const Core::CreateRunRequest &request)
{
    Q_D(Client);
    return d->post<RunReply>(resourcePath(kThreads, threadId, kRuns), compactJson(request.toJson()),
                             kAssistantsBeta);
}

RunStreamReply *Client::createRunStream(const QString &threadId,
                                        const Core::CreateRunRequest &request)
{
    Q_D(Client);
    return d->postStream<RunStreamReply>(resourcePath(kThreads, threadId, kRuns), request,
                                         kAssistantsBeta);
}

RunReply *Client::createThreadAndRun(const Core::CreateRunRequest &request)
{
    Q_D(Client);
    return d->post<RunReply>(QString(kThreads) + kRuns, compactJson(request.toJson()),
                             kAssistantsBeta);
}

RunStreamReply *Client::createThreadAndRunStream(const Core::CreateRunRequest &request)
{
    Q_D(Client);
    return d->postStream<RunStreamReply>(QString(kThreads) + kRuns, request, kAssistantsBeta);
}

RunListReply *Client::listRuns(const QString &threadId, const ListParams &params)
{
    Q_D(Client);
    return d->get<RunListReply>(resourcePath(kThreads, threadId, kRuns), params.toQuery(),
                                kAssistantsBeta);
}

RunReply *Client::getRun(const QString &threadId, const QString &runId)
{
    Q_D(Client);
    return d->get<RunReply>(threadRunPath(threadId, runId), {}, kAssistantsBeta);
}

RunReply *Client::updateRun(const QString &threadId, const QString &runId,
                            const QJsonObject &metadata)
{
    Q_D(Client);
    return d->post<RunReply>(threadRunPath(threadId, runId), metadataBody(metadata),
                             kAssistantsBeta);
}

RunReply *Client::cancelRun(const QString &threadId, const QString &runId)
{
    Q_D(Client);
    return d->post<RunReply>(threadRunPath(threadId, runId, QStringLiteral("/cancel")), {},
                             kAssistantsBeta);
}

RunReply *Client::submitToolOutputs(const QString &threadId, const QString &runId,
                                    const QList<Core::ToolOutput> &outputs)
{
    Q_D(Client);
    return d->post<RunReply>(threadRunPath(threadId, runId, QStringLiteral("/submit_tool_outputs")),
                             compactJson(ToolOutputsBody(outputs).toJson()), kAssistantsBeta);
}

RunStreamReply *Client::submitToolOutputsStream(const QString &threadId, const QString &runId,
                                                const QList<Core::ToolOutput> &outputs)
{
    Q_D(Client);
    return d->postStream<RunStreamReply>(
            threadRunPath(threadId, runId, QStringLiteral("/submit_tool_outputs")),
            ToolOutputsBody(outputs), kAssistantsBeta);
}

RunPoller *Client::pollRun(const QString &threadId, const QString &runId, int pollIntervalMs)
{
    return new RunPoller(this, threadId, runId, pollIntervalMs);
}

RunStepListReply *Client::listRunSteps(const QString &threadId, const QString &runId,
                                       const ListParams &params)
{
    Q_D(Client);
    return d->get<RunStepListReply>(threadRunPath(threadId, runId, kSteps), params.toQuery(),
                                    kAssistantsBeta);
}

RunStepReply *Client::getRunStep(const QString &threadId, const QString &runId,
                                 const QString &stepId)
{
    Q_D(Client);
    return d->get<RunStepReply>(threadRunPath(threadId, runId, resourcePath(kSteps, stepId)), {},
                                kAssistantsBeta);
}

RealtimeClientSecretReply *
Client::createRealtimeClientSecret(const Core::RealtimeSessionConfig &session,
                                   qint64 expiresAfterSeconds)
{
    Q_D(Client);
    return d->post<RealtimeClientSecretReply>(kRealtimeClientSecrets,
                                              clientSecretBody(session, expiresAfterSeconds));
}

RealtimeClientSecretReply *
Client::createRealtimeTranslationClientSecret(const Core::RealtimeSessionConfig &session,
                                              qint64 expiresAfterSeconds)
{
    Q_D(Client);
    return d->post<RealtimeClientSecretReply>(
            QString(kRealtime) + QStringLiteral("/translations/client_secrets"),
            clientSecretBody(session, expiresAfterSeconds));
}

RealtimeSessionReply *Client::createRealtimeSession(const Core::RealtimeSessionConfig &session)
{
    Q_D(Client);
    return d->post<RealtimeSessionReply>(QString(kRealtime) + QStringLiteral("/sessions"),
                                         compactJson(session.toJson()));
}

RealtimeClientSecretReply *
Client::createRealtimeTranscriptionSession(const Core::RealtimeSessionConfig &session)
{
    Q_D(Client);
    return d->post<RealtimeClientSecretReply>(QString(kRealtime)
                                                      + QStringLiteral("/transcription_sessions"),
                                              compactJson(session.toJson()));
}

RealtimeCallReply *Client::acceptRealtimeCall(const QString &callId,
                                              const Core::RealtimeSessionConfig &session)
{
    Q_D(Client);
    return d->post<RealtimeCallReply>(realtimeCallPath(callId, QStringLiteral("/accept")),
                                      compactJson(session.toJson()));
}

RealtimeCallReply *Client::rejectRealtimeCall(const QString &callId, int statusCode)
{
    Q_D(Client);
    QJsonObject bodyObject;
    // Omitting the code lets the API answer its documented default, 603.
    if (statusCode > 0)
        bodyObject.insert(QStringLiteral("status_code"), statusCode);
    return d->post<RealtimeCallReply>(realtimeCallPath(callId, QStringLiteral("/reject")),
                                      compactJson(bodyObject));
}

RealtimeCallReply *Client::hangupRealtimeCall(const QString &callId)
{
    Q_D(Client);
    return d->post<RealtimeCallReply>(realtimeCallPath(callId, QStringLiteral("/hangup")));
}

RealtimeCallReply *Client::referRealtimeCall(const QString &callId, const QString &targetUri)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("target_uri"), targetUri);
    return d->post<RealtimeCallReply>(realtimeCallPath(callId, QStringLiteral("/refer")),
                                      compactJson(bodyObject));
}

ChatKitSessionReply *Client::createChatKitSession(const Core::CreateChatKitSessionRequest &request)
{
    Q_D(Client);
    return d->post<ChatKitSessionReply>(kChatKitSessions, compactJson(request.toJson()),
                                        kChatKitBeta);
}

ChatKitSessionReply *Client::cancelChatKitSession(const QString &sessionId)
{
    Q_D(Client);
    return d->post<ChatKitSessionReply>(
            resourcePath(kChatKitSessions, sessionId, QStringLiteral("/cancel")), {}, kChatKitBeta);
}

ChatKitThreadListReply *Client::listChatKitThreads(const ListParams &params, const QString &user)
{
    Q_D(Client);
    QUrlQuery query = params.toQuery();
    if (!user.isEmpty())
        query.addQueryItem(QStringLiteral("user"), user);
    return d->get<ChatKitThreadListReply>(kChatKitThreads, query, kChatKitBeta);
}

ChatKitThreadReply *Client::getChatKitThread(const QString &threadId)
{
    Q_D(Client);
    return d->get<ChatKitThreadReply>(resourcePath(kChatKitThreads, threadId), {}, kChatKitBeta);
}

ChatKitThreadReply *Client::deleteChatKitThread(const QString &threadId)
{
    Q_D(Client);
    return d->remove<ChatKitThreadReply>(resourcePath(kChatKitThreads, threadId), kChatKitBeta);
}

ChatKitThreadItemListReply *Client::listChatKitThreadItems(const QString &threadId,
                                                           const ListParams &params)
{
    Q_D(Client);
    return d->get<ChatKitThreadItemListReply>(resourcePath(kChatKitThreads, threadId, kItems),
                                              params.toQuery(), kChatKitBeta);
}

SkillReply *Client::createSkill(const Core::CreateSkillRequest &request)
{
    Q_D(Client);
    return d->postMultipart<SkillReply>(kSkills, request.formFields(), skillFileParts(request));
}

SkillListReply *Client::listSkills(const ListParams &params)
{
    Q_D(Client);
    return d->get<SkillListReply>(kSkills, params.toQuery());
}

SkillReply *Client::getSkill(const QString &skillId)
{
    Q_D(Client);
    return d->get<SkillReply>(resourcePath(kSkills, skillId));
}

SkillReply *Client::setDefaultSkillVersion(const QString &skillId, const QString &version)
{
    Q_D(Client);
    QJsonObject bodyObject;
    bodyObject.insert(QStringLiteral("default_version"), version);
    return d->post<SkillReply>(resourcePath(kSkills, skillId), compactJson(bodyObject));
}

SkillReply *Client::deleteSkill(const QString &skillId)
{
    Q_D(Client);
    return d->remove<SkillReply>(resourcePath(kSkills, skillId));
}

BinaryReply *Client::downloadSkillContent(const QString &skillId)
{
    Q_D(Client);
    return d->get<BinaryReply>(resourcePath(kSkills, skillId, kContent));
}

SkillVersionReply *Client::createSkillVersion(const QString &skillId,
                                              const Core::CreateSkillRequest &request)
{
    Q_D(Client);
    return d->postMultipart<SkillVersionReply>(resourcePath(kSkills, skillId, kVersions),
                                               request.formFields(), skillFileParts(request));
}

SkillVersionListReply *Client::listSkillVersions(const QString &skillId, const ListParams &params)
{
    Q_D(Client);
    return d->get<SkillVersionListReply>(resourcePath(kSkills, skillId, kVersions),
                                         params.toQuery());
}

SkillVersionReply *Client::getSkillVersion(const QString &skillId, const QString &version)
{
    Q_D(Client);
    return d->get<SkillVersionReply>(skillVersionPath(skillId, version));
}

SkillVersionReply *Client::deleteSkillVersion(const QString &skillId, const QString &version)
{
    Q_D(Client);
    return d->remove<SkillVersionReply>(skillVersionPath(skillId, version));
}

BinaryReply *Client::downloadSkillVersionContent(const QString &skillId, const QString &version)
{
    Q_D(Client);
    return d->get<BinaryReply>(skillVersionPath(skillId, version, kContent));
}

ModelListReply *Client::listModels()
{
    Q_D(Client);
    return d->get<ModelListReply>(kModels);
}

ModelReply *Client::getModel(const QString &modelId)
{
    Q_D(Client);
    return d->get<ModelReply>(resourcePath(kModels, modelId));
}

} // namespace Client
} // namespace QtOpenAi
