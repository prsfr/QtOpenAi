// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Voice.h>

namespace QtOpenAi {
namespace Client {

class VoiceConsentListReplyPrivate;

// An asynchronous handle for GET /audio/voice_consents, returning a
// cursor-paginated page of consents. See RestReplyBase for the shared
// lifecycle.
class QTOPENAI_CLIENT_EXPORT VoiceConsentListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VoiceConsentList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VoiceConsentList &list);

private:
    friend class Client;
    VoiceConsentListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                          QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VoiceConsentListReply)
};

} // namespace Client
} // namespace QtOpenAi
