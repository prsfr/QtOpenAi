// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Eval.h>

namespace QtOpenAi {
namespace Client {

class EvalReplyPrivate;

// An asynchronous handle for a single eval (POST /evals, GET/POST/DELETE
// /evals/{id}). All four return an eval shape — the delete acknowledgement
// included — so this reply serves them all. See RestReplyBase for the shared
// lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT EvalReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::Eval eval() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Eval &eval);

private:
    friend class Client;
    EvalReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
              QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(EvalReply)
};

} // namespace Client
} // namespace QtOpenAi
