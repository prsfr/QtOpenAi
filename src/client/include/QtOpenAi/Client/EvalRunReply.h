// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/EvalRun.h>

namespace QtOpenAi {
namespace Client {

class EvalRunReplyPrivate;

// An asynchronous handle for a single eval run (POST/GET/DELETE below
// /evals/{eval_id}/runs). Cancelling is a POST to the run itself, so it shares
// this reply too. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalRunReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::EvalRun run() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalRun &run);

private:
    friend class Client;
    EvalRunReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                 QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(EvalRunReply)
};

} // namespace Client
} // namespace QtOpenAi
