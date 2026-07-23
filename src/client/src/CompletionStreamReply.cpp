// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/CompletionStreamReply.h"

#include "HttpSupport_p.h"
#include "SseParser_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>

namespace QtOpenAi {
namespace Client {

class CompletionStreamReplyPrivate
{
public:
    QNetworkReply *networkReply = nullptr;
    detail::SseParser parser;
    ClientError error;
    RateLimit rateLimit;
    // Accumulated fields reassembled from the streamed chunks.
    QString id;
    QString model;
    QString text;
    QString finishReason;
    bool finished = false;
    bool success = false;
    bool sawDone = false;
    bool autoDelete = true;

    Core::CompletionResponse response() const
    {
        Core::CompletionResponse response;
        response.setId(id);
        response.setModel(model);
        Core::CompletionChoice choice;
        choice.setText(text);
        choice.setIndex(0);
        choice.setFinishReason(finishReason);
        response.setChoices({choice});
        return response;
    }
};

CompletionStreamReply::CompletionStreamReply(QNetworkReply *reply, QObject *parent)
    : QObject(parent)
    , d_ptr(new CompletionStreamReplyPrivate)
{
    Q_D(CompletionStreamReply);
    d->networkReply = reply;
    reply->setParent(this);

    connect(reply, &QNetworkReply::readyRead, this, [this]() {
        Q_D(CompletionStreamReply);
        const QList<QByteArray> payloads = d->parser.feed(d->networkReply->readAll());
        for (const QByteArray &data : payloads) {
            if (data == "[DONE]") {
                d->sawDone = true;
                continue;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
                continue;
            const QJsonObject object = doc.object();

            if (const QString id = object.value(QStringLiteral("id")).toString(); !id.isEmpty())
                d->id = id;
            if (const QString model = object.value(QStringLiteral("model")).toString();
                !model.isEmpty())
                d->model = model;

            const QJsonArray choices = object.value(QStringLiteral("choices")).toArray();
            if (choices.isEmpty())
                continue;
            const QJsonObject choice = choices.first().toObject();
            const QString fragment = choice.value(QStringLiteral("text")).toString();
            if (const QString reason = choice.value(QStringLiteral("finish_reason")).toString();
                !reason.isEmpty())
                d->finishReason = reason;
            if (!fragment.isEmpty()) {
                d->text += fragment;
                Q_EMIT textDelta(fragment);
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this]() {
        Q_D(CompletionStreamReply);
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
        } else {
            d->success = true;
            Q_EMIT finished(d->response());
        }

        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

CompletionStreamReply::~CompletionStreamReply() = default;

bool CompletionStreamReply::isFinished() const
{
    Q_D(const CompletionStreamReply);
    return d->finished;
}

bool CompletionStreamReply::isSuccess() const
{
    Q_D(const CompletionStreamReply);
    return d->success;
}

Core::CompletionResponse CompletionStreamReply::response() const
{
    Q_D(const CompletionStreamReply);
    return d->response();
}

ClientError CompletionStreamReply::error() const
{
    Q_D(const CompletionStreamReply);
    return d->error;
}

RateLimit CompletionStreamReply::rateLimit() const
{
    Q_D(const CompletionStreamReply);
    return d->rateLimit;
}

void CompletionStreamReply::setAutoDelete(bool enabled)
{
    Q_D(CompletionStreamReply);
    d->autoDelete = enabled;
}

bool CompletionStreamReply::autoDelete() const
{
    Q_D(const CompletionStreamReply);
    return d->autoDelete;
}

void CompletionStreamReply::abort()
{
    Q_D(CompletionStreamReply);
    if (d->networkReply && d->networkReply->isRunning())
        d->networkReply->abort();
}

} // namespace Client
} // namespace QtOpenAi
