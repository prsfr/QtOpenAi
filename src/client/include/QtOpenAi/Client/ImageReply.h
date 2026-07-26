// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/ImageResponse.h>

namespace QtOpenAi {
namespace Client {

// An images request (POST /images/generations, /edits or /variations).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ImageReply : public TypedReply<Core::ImageResponse>
{
    Q_OBJECT
public:
    Core::ImageResponse response() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ImageResponse &response);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ImageResponse &response) override { Q_EMIT finished(response); }
};

} // namespace Client
} // namespace QtOpenAi
