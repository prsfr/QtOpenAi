// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ModelListReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ModelListReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::ModelList models;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ModelListReply::ModelListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : QObject(parent)
    , d_ptr(new ModelListReplyPrivate)
{
    Q_D(ModelListReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ModelListReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ModelListReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->models = Core::ModelList::fromJson(doc.object());
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->models);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(ModelListReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ModelListReply::~ModelListReply() = default;

bool ModelListReply::isFinished() const
{
    Q_D(const ModelListReply);
    return d->finished;
}

bool ModelListReply::isSuccess() const
{
    Q_D(const ModelListReply);
    return d->success;
}

Core::ModelList ModelListReply::models() const
{
    Q_D(const ModelListReply);
    return d->models;
}

ClientError ModelListReply::error() const
{
    Q_D(const ModelListReply);
    return d->error;
}

RateLimit ModelListReply::rateLimit() const
{
    Q_D(const ModelListReply);
    return d->engine->rateLimit();
}

int ModelListReply::retryCount() const
{
    Q_D(const ModelListReply);
    return d->engine->retryCount();
}

void ModelListReply::setAutoDelete(bool enabled)
{
    Q_D(ModelListReply);
    d->autoDelete = enabled;
}

bool ModelListReply::autoDelete() const
{
    Q_D(const ModelListReply);
    return d->autoDelete;
}

void ModelListReply::abort()
{
    Q_D(ModelListReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
