// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ModerationReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class ModerationReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::ModerationResponse response;
};

ModerationReply::ModerationReply(std::function<QNetworkReply *()> requestFactory,
                                 RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new ModerationReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::ModerationResponse ModerationReply::response() const
{
    Q_D(const ModerationReply);
    return d->response;
}

bool ModerationReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(ModerationReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->response = Core::ModerationResponse::fromJson(object);
    Q_EMIT finished(d->response);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
