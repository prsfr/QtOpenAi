// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/Client.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QUrlQuery>
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

} // namespace Client
} // namespace QtOpenAi
