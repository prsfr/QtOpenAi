// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Response.h>

namespace QtOpenAi {
namespace Client {

// A Responses API request (POST/GET/DELETE/cancel /responses); on delete the
// response() carries the deletion acknowledgement.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ResponseReply : public TypedReply<Core::Response>
{
    Q_OBJECT
public:
    Core::Response response() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Response &response);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Response &response) override { Q_EMIT finished(response); }
};

} // namespace Client
} // namespace QtOpenAi
