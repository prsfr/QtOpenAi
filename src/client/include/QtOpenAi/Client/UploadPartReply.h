// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Upload.h>

namespace QtOpenAi {
namespace Client {

// An upload-part request (POST /uploads/{id}/parts).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT UploadPartReply : public TypedReply<Core::UploadPart>
{
    Q_OBJECT
public:
    Core::UploadPart part() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::UploadPart &part);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::UploadPart &part) override { Q_EMIT finished(part); }
};

} // namespace Client
} // namespace QtOpenAi
