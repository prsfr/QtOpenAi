// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/EvalRun.h>

namespace QtOpenAi {
namespace Client {

class EvalRunOutputItemListReplyPrivate;

// An asynchronous handle for GET /evals/{eval_id}/runs/{run_id}/output_items,
// returning a page of graded results. See RestReplyBase for the shared
// lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalRunOutputItemListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::EvalRunOutputItemList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalRunOutputItemList &list);

private:
    friend class Client;
    EvalRunOutputItemListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                               QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(EvalRunOutputItemListReply)
};

} // namespace Client
} // namespace QtOpenAi
