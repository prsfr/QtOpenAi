// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/FileObject.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single-file request (POST /files, GET
// /files/{id}, DELETE /files/{id}). All three answer with the file shape, so
// this reply serves them all. See RestReplyBase for the shared lifecycle
// (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT FileReply : public TypedReply<Core::FileObject>
{
    Q_OBJECT
public:
    Core::FileObject file() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FileObject &file);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::FileObject &file) override { Q_EMIT finished(file); }
};

} // namespace Client
} // namespace QtOpenAi
