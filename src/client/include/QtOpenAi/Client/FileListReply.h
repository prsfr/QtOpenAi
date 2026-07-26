// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/FileObject.h>

namespace QtOpenAi {
namespace Client {

// A files-list request (GET /files).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT FileListReply : public TypedReply<Core::FileList>
{
    Q_OBJECT
public:
    Core::FileList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FileList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::FileList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
