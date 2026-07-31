// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for the SIP call-control endpoints
// (/realtime/calls/{id}/accept|reject|hangup|refer).
//
// These are the one family in the library with nothing to decode: the API
// acknowledges the action and returns no object, so success *is* the result and
// finished() carries no payload. It therefore derives from RestReplyBase
// directly rather than from TypedReply, which exists to hold a parsed value.
class QTOPENAI_CLIENT_EXPORT RealtimeCallReply : public RestReplyBase
{
    Q_OBJECT
Q_SIGNALS:
    void finished();

private:
    friend class Client;
    using RestReplyBase::RestReplyBase;

    bool dispatchSuccess(const QByteArray &, int) override
    {
        Q_EMIT finished();
        return true;
    }
};

} // namespace Client
} // namespace QtOpenAi
