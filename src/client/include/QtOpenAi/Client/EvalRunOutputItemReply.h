// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/EvalRun.h>

namespace QtOpenAi {
namespace Client {

class EvalRunOutputItemReplyPrivate;

// An asynchronous handle for GET
// /evals/{eval_id}/runs/{run_id}/output_items/{id} — one item's graded result.
// See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalRunOutputItemReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::EvalRunOutputItem item() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalRunOutputItem &item);

private:
    friend class Client;
    EvalRunOutputItemReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                           QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(EvalRunOutputItemReply)
};

} // namespace Client
} // namespace QtOpenAi
