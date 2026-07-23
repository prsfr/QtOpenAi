// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ResponseStreamReply.h"

#include "HttpSupport_p.h"
#include "SseParser_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {

class ResponseStreamReplyPrivate
{
public:
    QNetworkReply *networkReply = nullptr;
    detail::SseParser parser;
    Core::Response response;
    ClientError error;
    RateLimit rateLimit;
    bool sawCompleted = false;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ResponseStreamReply::ResponseStreamReply(QNetworkReply *reply, QObject *parent)
    : QObject(parent)
    , d_ptr(new ResponseStreamReplyPrivate)
{
    Q_D(ResponseStreamReply);
    d->networkReply = reply;
    reply->setParent(this);

    connect(reply, &QNetworkReply::readyRead, this, [this]() {
        Q_D(ResponseStreamReply);
        const QList<QByteArray> payloads = d->parser.feed(d->networkReply->readAll());
        for (const QByteArray &data : payloads) {
            if (data == "[DONE]")
                continue;
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
                continue;
            const QJsonObject object = doc.object();
            const QString type = object.value(QStringLiteral("type")).toString();

            Q_EMIT event(type, object);

            if (type == QLatin1String("response.output_text.delta")) {
                Q_EMIT outputTextDelta(object.value(QStringLiteral("delta")).toString());
            } else if (type == QLatin1String("response.function_call_arguments.delta")) {
                Q_EMIT functionCallArgumentsDelta(object.value(QStringLiteral("delta")).toString());
            } else if (type == QLatin1String("response.completed")) {
                d->response = Core::Response::fromJson(
                        object.value(QStringLiteral("response")).toObject());
                d->sawCompleted = true;
            } else if (type == QLatin1String("response.failed") || type == QLatin1String("error")) {
                // The error may sit at the event root or inside a response object.
                QJsonObject errorObject = object.value(QStringLiteral("error")).toObject();
                if (errorObject.isEmpty()) {
                    const QJsonObject response
                            = object.value(QStringLiteral("response")).toObject();
                    errorObject = response.value(QStringLiteral("error")).toObject();
                }
                const QString message = errorObject.value(QStringLiteral("message")).toString(type);
                d->error = ClientError(ClientError::Kind::Http, message);
                d->error.setType(errorObject.value(QStringLiteral("type")).toString());
                d->error.setCode(errorObject.value(QStringLiteral("code")).toString());
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this]() {
        Q_D(ResponseStreamReply);
        d->finished = true;
        QNetworkReply *reply = d->networkReply;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        d->rateLimit = detail::parseRateLimit(reply);

        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            QString message = reply->errorString();
            ClientError err(status >= 400 ? ClientError::Kind::Http : ClientError::Kind::Network,
                            message, status);
            const QByteArray body = d->parser.buffered() + reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson(body);
            if (doc.isObject()) {
                const QJsonObject errorObject
                        = doc.object().value(QStringLiteral("error")).toObject();
                if (!errorObject.isEmpty()) {
                    message = errorObject.value(QStringLiteral("message")).toString(message);
                    err = ClientError(ClientError::Kind::Http, message, status);
                    err.setType(errorObject.value(QStringLiteral("type")).toString());
                    err.setCode(errorObject.value(QStringLiteral("code")).toString());
                }
            }
            d->success = false;
            d->error = err;
            Q_EMIT failed(d->error);
        } else if (d->sawCompleted) {
            d->success = true;
            Q_EMIT finished(d->response);
        } else {
            d->success = false;
            if (!d->error.isError())
                d->error = ClientError(ClientError::Kind::Parse,
                                       QStringLiteral("stream ended before response.completed"),
                                       status);
            Q_EMIT failed(d->error);
        }

        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ResponseStreamReply::~ResponseStreamReply() = default;

bool ResponseStreamReply::isFinished() const
{
    Q_D(const ResponseStreamReply);
    return d->finished;
}

bool ResponseStreamReply::isSuccess() const
{
    Q_D(const ResponseStreamReply);
    return d->success;
}

Core::Response ResponseStreamReply::response() const
{
    Q_D(const ResponseStreamReply);
    return d->response;
}

ClientError ResponseStreamReply::error() const
{
    Q_D(const ResponseStreamReply);
    return d->error;
}

RateLimit ResponseStreamReply::rateLimit() const
{
    Q_D(const ResponseStreamReply);
    return d->rateLimit;
}

void ResponseStreamReply::setAutoDelete(bool enabled)
{
    Q_D(ResponseStreamReply);
    d->autoDelete = enabled;
}

bool ResponseStreamReply::autoDelete() const
{
    Q_D(const ResponseStreamReply);
    return d->autoDelete;
}

void ResponseStreamReply::abort()
{
    Q_D(ResponseStreamReply);
    if (d->networkReply && d->networkReply->isRunning())
        d->networkReply->abort();
}

} // namespace Client
} // namespace QtOpenAi
