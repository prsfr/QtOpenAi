// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ResponseReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ResponseReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::Response response;
};

ResponseReply::ResponseReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                             QObject *parent)
    : RestReplyBase(*new ResponseReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::Response ResponseReply::response() const
{
    Q_D(const ResponseReply);
    return d->response;
}

bool ResponseReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ResponseReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->response = Core::Response::fromJson(object);
    Q_EMIT finished(d->response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
