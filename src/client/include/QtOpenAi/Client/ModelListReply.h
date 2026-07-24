// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Model.h>

namespace QtOpenAi {
namespace Client {

class ModelListReplyPrivate;

// A models-list request (GET /models).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ModelListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ModelList models() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ModelList &models);

private:
    friend class Client;
    ModelListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                   QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ModelListReply)
};

} // namespace Client
} // namespace QtOpenAi
