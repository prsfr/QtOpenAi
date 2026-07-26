// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Container.h>

namespace QtOpenAi {
namespace Client {

// A containers list request (GET /containers).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ContainerListReply : public TypedReply<Core::ContainerList>
{
    Q_OBJECT
public:
    Core::ContainerList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ContainerList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ContainerList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
