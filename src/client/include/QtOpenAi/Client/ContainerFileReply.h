// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Container.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single file inside a container (add,
// retrieve, delete). All answer with the container-file shape.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ContainerFileReply : public TypedReply<Core::ContainerFile>
{
    Q_OBJECT
public:
    Core::ContainerFile file() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ContainerFile &file);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ContainerFile &file) override { Q_EMIT finished(file); }
};

} // namespace Client
} // namespace QtOpenAi
