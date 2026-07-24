// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/CompletionReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class CompletionReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::CompletionResponse response;
};

CompletionReply::CompletionReply(std::function<QNetworkReply *()> requestFactory,
                                 RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new CompletionReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::CompletionResponse CompletionReply::response() const
{
    Q_D(const CompletionReply);
    return d->response;
}

bool CompletionReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(CompletionReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->response = Core::CompletionResponse::fromJson(object);
    Q_EMIT finished(d->response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
