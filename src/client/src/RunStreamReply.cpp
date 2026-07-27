// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/RunStreamReply.h"

#include "HttpSupport_p.h"
#include "SseParser_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {

namespace {

// The text of a `thread.message.delta` payload: its delta carries the same
// content parts a finished message does, one fragment at a time.
QString deltaText(const QJsonObject &payload)
{
    QString text;
    const QJsonArray parts = payload.value(QStringLiteral("delta"))
                                     .toObject()
                                     .value(QStringLiteral("content"))
                                     .toArray();
    for (const QJsonValue &value : parts) {
        const QJsonObject part = value.toObject();
        if (part.value(QStringLiteral("type")).toString() != QLatin1String("text"))
            continue;
        text += part.value(QStringLiteral("text"))
                        .toObject()
                        .value(QStringLiteral("value"))
                        .toString();
    }
    return text;
}

} // namespace

class RunStreamReplyPrivate
{
public:
    QNetworkReply *networkReply = nullptr;
    detail::SseParser parser;
    Core::Run run;
    ClientError error;
    RateLimit rateLimit;
    bool sawTerminal = false;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

RunStreamReply::RunStreamReply(QNetworkReply *reply, QObject *parent)
    : QObject(parent)
    , d_ptr(new RunStreamReplyPrivate)
{
    Q_D(RunStreamReply);
    d->networkReply = reply;
    reply->setParent(this);

    connect(reply, &QNetworkReply::readyRead, this, [this]() {
        Q_D(RunStreamReply);
        const QList<QByteArray> payloads = d->parser.feed(d->networkReply->readAll());
        for (const QByteArray &data : payloads) {
            if (data == "[DONE]")
                continue;
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
                continue;
            const QJsonObject object = doc.object();
            // Assistants events name their type in the SSE `event:` field, which
            // the framing parser does not surface -- but every payload is a
            // whole Assistants object that names itself, so the routing key is
            // read from the body instead.
            const QString objectType = object.value(QStringLiteral("object")).toString();

            Q_EMIT event(objectType, object);

            if (objectType == QLatin1String("thread.message.delta")) {
                const QString text = deltaText(object);
                if (!text.isEmpty())
                    Q_EMIT messageDelta(text);
            } else if (objectType == QLatin1String("thread.message")) {
                Q_EMIT messageCompleted(Core::ThreadMessage::fromJson(object));
            } else if (objectType == QLatin1String("thread.run")) {
                d->run = Core::Run::fromJson(object);
                Q_EMIT runChanged(d->run);
                if (d->run.requiresAction()) {
                    d->sawTerminal = true;
                    Q_EMIT requiresAction(d->run);
                } else if (d->run.isTerminal()) {
                    d->sawTerminal = true;
                }
            } else if (objectType.isEmpty() && object.contains(QStringLiteral("error"))) {
                // An error event is the one payload that is not an object of the
                // API's own model.
                const QJsonObject errorObject = object.value(QStringLiteral("error")).toObject();
                d->error = ClientError(
                        ClientError::Kind::Http,
                        errorObject.value(QStringLiteral("message"))
                                .toString(QStringLiteral("run stream reported an error")));
                d->error.setType(errorObject.value(QStringLiteral("type")).toString());
                d->error.setCode(errorObject.value(QStringLiteral("code")).toString());
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this]() {
        Q_D(RunStreamReply);
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
        } else if (d->sawTerminal) {
            d->success = true;
            Q_EMIT finished(d->run);
        } else {
            d->success = false;
            if (!d->error.isError())
                d->error = ClientError(ClientError::Kind::Parse,
                                       QStringLiteral("stream ended before the run settled"),
                                       status);
            Q_EMIT failed(d->error);
        }

        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

RunStreamReply::~RunStreamReply() = default;

bool RunStreamReply::isFinished() const
{
    Q_D(const RunStreamReply);
    return d->finished;
}

bool RunStreamReply::isSuccess() const
{
    Q_D(const RunStreamReply);
    return d->success;
}

Core::Run RunStreamReply::run() const
{
    Q_D(const RunStreamReply);
    return d->run;
}

ClientError RunStreamReply::error() const
{
    Q_D(const RunStreamReply);
    return d->error;
}

RateLimit RunStreamReply::rateLimit() const
{
    Q_D(const RunStreamReply);
    return d->rateLimit;
}

void RunStreamReply::setAutoDelete(bool enabled)
{
    Q_D(RunStreamReply);
    d->autoDelete = enabled;
}

bool RunStreamReply::autoDelete() const
{
    Q_D(const RunStreamReply);
    return d->autoDelete;
}

void RunStreamReply::abort()
{
    Q_D(RunStreamReply);
    if (d->networkReply && d->networkReply->isRunning())
        d->networkReply->abort();
}

} // namespace Client
} // namespace QtOpenAi
