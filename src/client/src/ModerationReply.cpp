// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ModerationReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ModerationReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::ModerationResponse response;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ModerationReply::ModerationReply(std::function<QNetworkReply *()> requestFactory,
                                 RetryPolicy policy, QObject *parent)
    : QObject(parent)
    , d_ptr(new ModerationReplyPrivate)
{
    Q_D(ModerationReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ModerationReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ModerationReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->response = Core::ModerationResponse::fromJson(doc.object());
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
        Q_D(ModerationReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ModerationReply::~ModerationReply() = default;

bool ModerationReply::isFinished() const
{
    Q_D(const ModerationReply);
    return d->finished;
}

bool ModerationReply::isSuccess() const
{
    Q_D(const ModerationReply);
    return d->success;
}

Core::ModerationResponse ModerationReply::response() const
{
    Q_D(const ModerationReply);
    return d->response;
}

ClientError ModerationReply::error() const
{
    Q_D(const ModerationReply);
    return d->error;
}

RateLimit ModerationReply::rateLimit() const
{
    Q_D(const ModerationReply);
    return d->engine->rateLimit();
}

int ModerationReply::retryCount() const
{
    Q_D(const ModerationReply);
    return d->engine->retryCount();
}

void ModerationReply::setAutoDelete(bool enabled)
{
    Q_D(ModerationReply);
    d->autoDelete = enabled;
}

bool ModerationReply::autoDelete() const
{
    Q_D(const ModerationReply);
    return d->autoDelete;
}

void ModerationReply::abort()
{
    Q_D(ModerationReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
