// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/BinaryReply.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for one text-to-speech request (POST /audio/speech).
// The endpoint returns a binary audio blob; this is a BinaryReply whose
// audioData() is a domain alias for data(). See BinaryReply / RestReplyBase for
// the raw-bytes handling and the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT SpeechReply : public BinaryReply
{
    Q_OBJECT
public:
    // The raw audio bytes returned by the server (alias for data()).
    QByteArray audioData() const;

private:
    friend class Client;
    SpeechReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                QObject *parent = nullptr);
};

} // namespace Client
} // namespace QtOpenAi
