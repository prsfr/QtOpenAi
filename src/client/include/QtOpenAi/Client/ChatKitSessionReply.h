// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ChatKitSession.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a ChatKit session (POST /chatkit/sessions and
// POST /chatkit/sessions/{id}/cancel, which answers with the same object).
// See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT ChatKitSessionReply : public TypedReply<Core::ChatKitSession>
{
    Q_OBJECT
public:
    Core::ChatKitSession session() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ChatKitSession &session);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ChatKitSession &session) override { Q_EMIT finished(session); }
};

} // namespace Client
} // namespace QtOpenAi
