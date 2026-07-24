// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/SpeechReply.h"

namespace QtOpenAi {
namespace Client {

SpeechReply::SpeechReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                         QObject *parent)
    : BinaryReply(std::move(requestFactory), std::move(policy), parent)
{ }

QByteArray SpeechReply::audioData() const { return data(); }

} // namespace Client
} // namespace QtOpenAi
