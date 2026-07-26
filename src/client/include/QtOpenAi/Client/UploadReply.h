// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Upload.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single-upload request (POST /uploads and the
// /complete and /cancel actions). All three answer with the upload shape, so
// this reply serves them all. See RestReplyBase for the shared lifecycle
// (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT UploadReply : public TypedReply<Core::Upload>
{
    Q_OBJECT
public:
    Core::Upload upload() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Upload &upload);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Upload &upload) override { Q_EMIT finished(upload); }
};

} // namespace Client
} // namespace QtOpenAi
