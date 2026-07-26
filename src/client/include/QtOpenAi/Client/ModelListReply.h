// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Model.h>

namespace QtOpenAi {
namespace Client {

// A models-list request (GET /models).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ModelListReply : public TypedReply<Core::ModelList>
{
    Q_OBJECT
public:
    Core::ModelList models() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ModelList &models);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::ModelList &models) override { Q_EMIT finished(models); }
};

} // namespace Client
} // namespace QtOpenAi
