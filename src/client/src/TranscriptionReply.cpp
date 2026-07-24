// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/TranscriptionReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class TranscriptionReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::TranscriptionResponse response;
};

TranscriptionReply::TranscriptionReply(std::function<QNetworkReply *()> requestFactory,
                                       RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new TranscriptionReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::TranscriptionResponse TranscriptionReply::response() const
{
    Q_D(const TranscriptionReply);
    return d->response;
}

bool TranscriptionReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(TranscriptionReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->response = Core::TranscriptionResponse::fromJson(object);
    Q_EMIT finished(d->response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
