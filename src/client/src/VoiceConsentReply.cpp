// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VoiceConsentReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VoiceConsentReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VoiceConsent consent;
};

VoiceConsentReply::VoiceConsentReply(std::function<QNetworkReply *()> requestFactory,
                                     RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new VoiceConsentReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::VoiceConsent VoiceConsentReply::consent() const
{
    Q_D(const VoiceConsentReply);
    return d->consent;
}

bool VoiceConsentReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VoiceConsentReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->consent = Core::VoiceConsent::fromJson(object);
    Q_EMIT finished(d->consent);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
