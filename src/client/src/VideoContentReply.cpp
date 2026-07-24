// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VideoContentReply.h"

namespace QtOpenAi {
namespace Client {

VideoContentReply::VideoContentReply(std::function<QNetworkReply *()> requestFactory,
                                     RetryPolicy policy, QObject *parent)
    : BinaryReply(std::move(requestFactory), std::move(policy), parent)
{ }

QByteArray VideoContentReply::videoData() const { return data(); }

} // namespace Client
} // namespace QtOpenAi
