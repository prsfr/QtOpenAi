// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Voice.h>

namespace QtOpenAi {
namespace Client {

class VoiceConsentReplyPrivate;

// An asynchronous handle for a single voice consent (POST/GET/DELETE below
// /audio/voice_consents). All of them return a consent shape — the deletion
// acknowledgement included — so this reply serves them all. See RestReplyBase
// for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT VoiceConsentReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VoiceConsent consent() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VoiceConsent &consent);

private:
    friend class Client;
    VoiceConsentReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                      QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VoiceConsentReply)
};

} // namespace Client
} // namespace QtOpenAi
