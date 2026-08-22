// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for POST /realtime/calls -- the signalling half of the
// WebRTC handshake.
//
// This is the only reply in the library whose result is **not** JSON and is not
// wholly in the body. The API answers `201 Created` with the SDP answer as
// `application/sdp`, and puts the new call's identifier in the `Location`
// header and nowhere else. Both halves are needed: the answer completes the
// peer connection, and the id is what acceptRealtimeCall(),
// hangupRealtimeCall() and a monitoring WebSocket are addressed with. Dropping
// the header would leave a caller with a working call they cannot control.
//
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT RealtimeCallCreateReply : public RestReplyBase
{
    Q_OBJECT
public:
    // The SDP answer, to be handed to the peer connection as its remote
    // description.
    QByteArray sdpAnswer() const { return m_sdpAnswer; }

    // The new call's id, parsed out of the Location header. Empty if the API
    // sent no Location -- which is not an error here, because the SDP answer is
    // still valid and the media path still works.
    QString callId() const { return m_callId; }

Q_SIGNALS:
    void finished(const QString &callId, const QByteArray &sdpAnswer);

private:
    friend class Client;
    using RestReplyBase::RestReplyBase;

    bool dispatchSuccess(const QByteArray &body, int) override
    {
        m_sdpAnswer = body;
        // Location is documented as a relative URL "containing the call ID",
        // e.g. /v1/realtime/calls/rtc_123 -- so the id is its last segment
        // rather than the whole value. Query strings and trailing slashes are
        // not part of the documented shape but cost nothing to survive.
        QByteArray location = responseHeader("Location");
        const int query = location.indexOf('?');
        if (query >= 0)
            location.truncate(query);
        while (location.endsWith('/'))
            location.chop(1);
        const int slash = location.lastIndexOf('/');
        m_callId = QString::fromUtf8(slash >= 0 ? location.mid(slash + 1) : location);

        Q_EMIT finished(m_callId, m_sdpAnswer);
        return true;
    }

    QByteArray m_sdpAnswer;
    QString m_callId;
};

} // namespace Client
} // namespace QtOpenAi
