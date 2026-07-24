// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/EmbeddingReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class EmbeddingReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::EmbeddingResponse response;
};

EmbeddingReply::EmbeddingReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent)
    : RestReplyBase(*new EmbeddingReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::EmbeddingResponse EmbeddingReply::response() const
{
    Q_D(const EmbeddingReply);
    return d->response;
}

bool EmbeddingReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(EmbeddingReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->response = Core::EmbeddingResponse::fromJson(object);
    Q_EMIT finished(d->response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
