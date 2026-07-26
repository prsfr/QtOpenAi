// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/VectorStoreFile.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single vector-store file (attach, retrieve,
// update attributes, detach). All answer with the file shape.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreFileReply : public TypedReply<Core::VectorStoreFile>
{
    Q_OBJECT
public:
    Core::VectorStoreFile file() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreFile &file);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VectorStoreFile &file) override { Q_EMIT finished(file); }
};

} // namespace Client
} // namespace QtOpenAi
