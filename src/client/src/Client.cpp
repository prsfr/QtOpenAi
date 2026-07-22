// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/Client.h"

#include <QtCore/QJsonDocument>
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
    QNetworkAccessManager *manager = nullptr;
    bool ownsManager = false;

    // Join the base URL with an endpoint path, tolerating trailing slashes.
    QUrl endpointUrl(const QString &path) const
    {
        QString base = baseUrl.toString();
        while (base.endsWith(QLatin1Char('/')))
            base.chop(1);
        QString suffix = path;
        if (!suffix.startsWith(QLatin1Char('/')))
            suffix.prepend(QLatin1Char('/'));
        return QUrl(base + suffix);
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

// Build the /chat/completions network request (URL + auth/content headers).
QNetworkRequest chatRequest(const ClientPrivate *d)
{
    QNetworkRequest networkRequest(d->endpointUrl(QStringLiteral("/chat/completions")));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             QStringLiteral("application/json"));
    if (!d->apiKey.isEmpty())
        networkRequest.setRawHeader("Authorization", QByteArray("Bearer ") + d->apiKey.toUtf8());
    if (!d->organization.isEmpty())
        networkRequest.setRawHeader("OpenAI-Organization", d->organization.toUtf8());
    return networkRequest;
}

} // namespace

ChatCompletionReply *Client::createChatCompletion(const Core::ChatCompletionRequest &request)
{
    Q_D(Client);
    const QByteArray body = QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = networkAccessManager()->post(chatRequest(d), body);
    return new ChatCompletionReply(reply);
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

} // namespace Client
} // namespace QtOpenAi
