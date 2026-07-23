// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ModelReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ModelReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::Model model;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ModelReply::ModelReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : QObject(parent)
    , d_ptr(new ModelReplyPrivate)
{
    Q_D(ModelReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ModelReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ModelReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->model = Core::Model::fromJson(doc.object());
            d->success = true;
        }
        d->finished = true;
        if (d->success)
            Q_EMIT finished(d->model);
        else
            Q_EMIT failed(d->error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });

    connect(d->engine, &RestReply::failed, this, [this](const ClientError &error) {
        Q_D(ModelReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ModelReply::~ModelReply() = default;

bool ModelReply::isFinished() const
{
    Q_D(const ModelReply);
    return d->finished;
}

bool ModelReply::isSuccess() const
{
    Q_D(const ModelReply);
    return d->success;
}

Core::Model ModelReply::model() const
{
    Q_D(const ModelReply);
    return d->model;
}

ClientError ModelReply::error() const
{
    Q_D(const ModelReply);
    return d->error;
}

RateLimit ModelReply::rateLimit() const
{
    Q_D(const ModelReply);
    return d->engine->rateLimit();
}

int ModelReply::retryCount() const
{
    Q_D(const ModelReply);
    return d->engine->retryCount();
}

void ModelReply::setAutoDelete(bool enabled)
{
    Q_D(ModelReply);
    d->autoDelete = enabled;
}

bool ModelReply::autoDelete() const
{
    Q_D(const ModelReply);
    return d->autoDelete;
}

void ModelReply::abort()
{
    Q_D(ModelReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
