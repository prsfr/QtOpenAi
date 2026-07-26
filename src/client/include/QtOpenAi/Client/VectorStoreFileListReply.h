// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/VectorStoreFile.h>

namespace QtOpenAi {
namespace Client {

// A vector-store files list request — both the store's own files and the
// files of a single batch, which share one response shape.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreFileListReply : public TypedReply<Core::VectorStoreFileList>
{
    Q_OBJECT
public:
    Core::VectorStoreFileList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreFileList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VectorStoreFileList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
