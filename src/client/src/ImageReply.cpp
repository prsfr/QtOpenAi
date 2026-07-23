// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ImageReply.h"

#include "RestReply_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ImageReplyPrivate
{
public:
    RestReply *engine = nullptr;
    Core::ImageResponse response;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

ImageReply::ImageReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : QObject(parent)
    , d_ptr(new ImageReplyPrivate)
{
    Q_D(ImageReply);
    d->engine = new RestReply(std::move(requestFactory), std::move(policy), this);

    connect(d->engine, &RestReply::retrying, this, &ImageReply::retrying);

    connect(d->engine, &RestReply::succeeded, this, [this](const QByteArray &body, int status) {
        Q_D(ImageReply);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            d->success = false;
            d->error = ClientError(
                    ClientError::Kind::Parse,
                    QStringLiteral("invalid JSON response: %1").arg(parseError.errorString()),
                    status);
        } else {
            d->response = Core::ImageResponse::fromJson(doc.object());
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
        Q_D(ImageReply);
        d->finished = true;
        d->success = false;
        d->error = error;
        Q_EMIT failed(error);
        Q_EMIT done();
        if (d->autoDelete)
            deleteLater();
    });
}

ImageReply::~ImageReply() = default;

bool ImageReply::isFinished() const
{
    Q_D(const ImageReply);
    return d->finished;
}

bool ImageReply::isSuccess() const
{
    Q_D(const ImageReply);
    return d->success;
}

Core::ImageResponse ImageReply::response() const
{
    Q_D(const ImageReply);
    return d->response;
}

ClientError ImageReply::error() const
{
    Q_D(const ImageReply);
    return d->error;
}

RateLimit ImageReply::rateLimit() const
{
    Q_D(const ImageReply);
    return d->engine->rateLimit();
}

int ImageReply::retryCount() const
{
    Q_D(const ImageReply);
    return d->engine->retryCount();
}

void ImageReply::setAutoDelete(bool enabled)
{
    Q_D(ImageReply);
    d->autoDelete = enabled;
}

bool ImageReply::autoDelete() const
{
    Q_D(const ImageReply);
    return d->autoDelete;
}

void ImageReply::abort()
{
    Q_D(ImageReply);
    d->engine->abort();
}

} // namespace Client
} // namespace QtOpenAi
