// SPDX-License-Identifier: MIT
#pragma once

// Private data for StreamReplyBase and its subclasses. A concrete stream
// derives its own XxxStreamReplyPrivate from this and stores what it
// accumulates there, so those members stay out of the public header (Qt
// d-pointer convention). Not installed / not part of the public API.

#include "QtOpenAi/Client/ClientError.h"
#include "QtOpenAi/Client/RetryPolicy.h"

#include "SseParser_p.h"

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class StreamReplyBasePrivate
{
public:
    virtual ~StreamReplyBasePrivate() = default;

    QNetworkReply *networkReply = nullptr;
    detail::SseParser parser;
    ClientError error;
    RateLimit rateLimit;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

} // namespace Client
} // namespace QtOpenAi
