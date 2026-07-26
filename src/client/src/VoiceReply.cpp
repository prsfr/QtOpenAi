// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VoiceReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VoiceReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::Voice voice;
};

VoiceReply::VoiceReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent)
    : RestReplyBase(*new VoiceReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::Voice VoiceReply::voice() const
{
    Q_D(const VoiceReply);
    return d->voice;
}

bool VoiceReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VoiceReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->voice = Core::Voice::fromJson(object);
    Q_EMIT finished(d->voice);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
