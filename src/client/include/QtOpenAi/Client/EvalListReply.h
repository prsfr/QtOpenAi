// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Eval.h>

namespace QtOpenAi {
namespace Client {

class EvalListReplyPrivate;

// An asynchronous handle for GET /evals, returning a cursor-paginated page of
// eval definitions. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT EvalListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::EvalList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::EvalList &list);

private:
    friend class Client;
    EvalListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                  QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(EvalListReply)
};

} // namespace Client
} // namespace QtOpenAi
