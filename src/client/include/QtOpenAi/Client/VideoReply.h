// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/VideoJob.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single-video request (POST /videos, GET
// /videos/{id}, POST /videos/{id}/remix, DELETE /videos/{id}). All return a
// VideoJob shape, so this reply serves them all. See RestReplyBase for the
// shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VideoReply : public TypedReply<Core::VideoJob>
{
    Q_OBJECT
public:
    Core::VideoJob job() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VideoJob &job);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VideoJob &job) override { Q_EMIT finished(job); }
};

} // namespace Client
} // namespace QtOpenAi
