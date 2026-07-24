// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Model.h>

namespace QtOpenAi {
namespace Client {

class ModelReplyPrivate;

// A single model (GET /models/{id}).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ModelReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::Model model() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Model &model);

private:
    friend class Client;
    ModelReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
               QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ModelReply)
};

} // namespace Client
} // namespace QtOpenAi
