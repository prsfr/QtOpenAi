// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/EvalRun.h>

namespace QtOpenAi {
namespace Client {

class EvalRunListReplyPrivate;

// An asynchronous handle for GET /evals/{eval_id}/runs, returning a page of an
// eval's runs. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalRunListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::EvalRunList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalRunList &list);

private:
    friend class Client;
    EvalRunListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                     QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(EvalRunListReply)
};

} // namespace Client
} // namespace QtOpenAi
