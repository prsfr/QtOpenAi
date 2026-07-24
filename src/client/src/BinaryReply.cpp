// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/BinaryReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class BinaryReplyPrivate : public RestReplyBasePrivate
{
public:
    QByteArray data;
    QByteArray contentType;
};

BinaryReply::BinaryReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                         QObject *parent)
    : RestReplyBase(*new BinaryReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

QByteArray BinaryReply::data() const
{
    Q_D(const BinaryReply);
    return d->data;
}

QByteArray BinaryReply::contentType() const
{
    Q_D(const BinaryReply);
    return d->contentType;
}

bool BinaryReply::dispatchSuccess(const QByteArray &body, int)
{
    Q_D(BinaryReply);
    // Binary payload: surface the bytes verbatim, no JSON parsing.
    d->data = body;
    d->contentType = responseContentType();
    Q_EMIT finished(d->data);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
