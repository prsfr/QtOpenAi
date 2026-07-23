// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/CompletionReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class CompletionReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::CompletionResponse response;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

CompletionReply::CompletionReply(std::function<QNetworkReply *()> requestFactory,
                                 RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new CompletionReplyPrivate)
{
    Q_D(CompletionReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &CompletionReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(CompletionReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->response = Core::CompletionResponse::fromJson(doc.object());
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->response);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(CompletionReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

CompletionReply::~CompletionReply() = default;

bool CompletionReply::isFinished() const
{
    Q_D(const CompletionReply);
    return d->finished;
}

bool CompletionReply::isSuccess() const
{
    Q_D(const CompletionReply);
    return d->success;
}

Core::CompletionResponse CompletionReply::response() const
{
    Q_D(const CompletionReply);
    return d->response;
}

ClientError CompletionReply::error() const
{
    Q_D(const CompletionReply);
    return d->error;
}

RateLimit CompletionReply::rateLimit() const
{
    Q_D(const CompletionReply);
    return d->engine->rateLimit();
}

int CompletionReply::retryCount() const
{
    Q_D(const CompletionReply);
    return d->engine->retryCount();
}

void CompletionReply::setAutoDelete(bool enabled)
{
    Q_D(CompletionReply);
    d->autoDelete = enabled;
}

bool CompletionReply::autoDelete() const
{
    Q_D(const CompletionReply);
    return d->autoDelete;
}

void CompletionReply::abort()
{
    Q_D(CompletionReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
