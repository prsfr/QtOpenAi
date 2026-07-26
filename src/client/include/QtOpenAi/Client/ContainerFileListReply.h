// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Container.h>

namespace QtOpenAi {
namespace Client {

// A container-files list request (GET /containers/{id}/files).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ContainerFileListReply : public TypedReply<Core::ContainerFileList>
{
    Q_OBJECT
public:
    Core::ContainerFileList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ContainerFileList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ContainerFileList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
