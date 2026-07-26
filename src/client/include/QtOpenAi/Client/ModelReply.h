// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Model.h>

namespace QtOpenAi {
namespace Client {

// A single model (GET /models/{id}).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ModelReply : public TypedReply<Core::Model>
{
    Q_OBJECT
public:
    Core::Model model() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Model &model);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Model &model) override { Q_EMIT finished(model); }
};

} // namespace Client
} // namespace QtOpenAi
