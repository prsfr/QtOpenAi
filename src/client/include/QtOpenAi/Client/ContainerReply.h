// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Container.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single container (POST /containers,
// GET/DELETE /containers/{id}). All answer with the container shape,
// including the deletion acknowledgement.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ContainerReply : public TypedReply<Core::Container>
{
    Q_OBJECT
public:
    Core::Container container() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Container &container);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Container &container) override { Q_EMIT finished(container); }
};

} // namespace Client
} // namespace QtOpenAi
