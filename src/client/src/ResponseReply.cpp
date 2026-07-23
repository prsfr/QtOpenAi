// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ResponseReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ResponseReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::Response response;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ResponseReply::ResponseReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                             QObject *parent)
    : QObject(parent)
    , d_ptr(new ResponseReplyPrivate)
{
    Q_D(ResponseReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ResponseReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ResponseReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->response = Core::Response::fromJson(doc.object());
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
        Q_D(ResponseReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ResponseReply::~ResponseReply() = default;

bool ResponseReply::isFinished() const
{
    Q_D(const ResponseReply);
    return d->finished;
}

bool ResponseReply::isSuccess() const
{
    Q_D(const ResponseReply);
    return d->success;
}

Core::Response ResponseReply::response() const
{
    Q_D(const ResponseReply);
    return d->response;
}

ClientError ResponseReply::error() const
{
    Q_D(const ResponseReply);
    return d->error;
}

RateLimit ResponseReply::rateLimit() const
{
    Q_D(const ResponseReply);
    return d->engine->rateLimit();
}

int ResponseReply::retryCount() const
{
    Q_D(const ResponseReply);
    return d->engine->retryCount();
}

void ResponseReply::setAutoDelete(bool enabled)
{
    Q_D(ResponseReply);
    d->autoDelete = enabled;
}

bool ResponseReply::autoDelete() const
{
    Q_D(const ResponseReply);
    return d->autoDelete;
}

void ResponseReply::abort()
{
    Q_D(ResponseReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
