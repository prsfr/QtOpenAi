// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Container.h>

namespace QtOpenAi {
namespace Client {

class ContainerListReplyPrivate;

// A containers list request (GET /containers).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ContainerListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ContainerList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ContainerList &list);

private:
    friend class Client;
    ContainerListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ContainerListReply)
};

} // namespace Client
} // namespace QtOpenAi
