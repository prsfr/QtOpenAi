// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/VideoJob.h>

namespace QtOpenAi {
namespace Client {

// A videos-list request (GET /videos).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VideoListReply : public TypedReply<Core::VideoList>
{
    Q_OBJECT
public:
    Core::VideoList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VideoList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VideoList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
