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

// Whether a `thread.message*` event carries a message that will not change
// again. The event name says so directly; a server that omits the name leaves
// the message's own status as the only signal.
bool isSettledMessage(const QString &type, const Core::ThreadMessage &message)
{
    if (type == QLatin1String("thread.message.completed")
        || type == QLatin1String("thread.message.incomplete"))
        return true;
    return type == QLatin1String("thread.message")
            && message.status() != QLatin1String("in_progress");
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
        const QList<detail::SseEvent> events = d->parser.feed(d->networkReply->readAll());
        for (const detail::SseEvent &sse : events) {
            if (sse.data == "[DONE]")
                continue;
            const QJsonDocument doc = QJsonDocument::fromJson(sse.data);
            if (!doc.isObject())
                continue;
            const QJsonObject object = doc.object();
            // Unlike the other streams, the Assistants events carry their type
            // only in the SSE `event:` field: `thread.message.created` and
            // `thread.message.completed` are both a bare message object, and
            // nothing inside them says which of the two arrived. The payload's
            // own `object` is the fallback for a server that omits the name.
            const QString type = !sse.name.isEmpty()
                    ? QString::fromUtf8(sse.name)
                    : object.value(QStringLiteral("object")).toString();

            Q_EMIT event(type, object);

            if (type == QLatin1String("thread.message.delta")) {
                const QString text = deltaText(object);
                if (!text.isEmpty())
                    Q_EMIT messageDelta(text);
            } else if (type.startsWith(QLatin1String("thread.message"))) {
                // Only the settled message is worth a signal -- `created` and
                // `in_progress` carry the same object with no content yet.
                const Core::ThreadMessage message = Core::ThreadMessage::fromJson(object);
                if (isSettledMessage(type, message))
                    Q_EMIT messageCompleted(message);
            } else if (type == QLatin1String("thread.run")
                       || (type.startsWith(QLatin1String("thread.run."))
                           && !type.startsWith(QLatin1String("thread.run.step")))) {
                d->run = Core::Run::fromJson(object);
                Q_EMIT runChanged(d->run);
                if (d->run.requiresAction()) {
                    d->sawTerminal = true;
                    Q_EMIT requiresAction(d->run);
                } else if (d->run.isTerminal()) {
                    d->sawTerminal = true;
                }
            } else if (type == QLatin1String("error")) {
                // The error event is the one payload that is not an object of
                // the API's own model: a bare {code, message, param, type}.
                d->error = ClientError(ClientError::Kind::Http,
                                       object.value(QStringLiteral("message"))
                                               .toString(QStringLiteral(
                                                       "run stream reported an error")));
                d->error.setType(object.value(QStringLiteral("type")).toString());
                d->error.setCode(object.value(QStringLiteral("code")).toString());
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
