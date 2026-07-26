// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VoiceConsentListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VoiceConsentListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VoiceConsentList list;
};

VoiceConsentListReply::VoiceConsentListReply(std::function<QNetworkReply *()> requestFactory,
                                             RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new VoiceConsentListReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::VoiceConsentList VoiceConsentListReply::list() const
{
    Q_D(const VoiceConsentListReply);
    return d->list;
}

bool VoiceConsentListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VoiceConsentListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::VoiceConsentList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
