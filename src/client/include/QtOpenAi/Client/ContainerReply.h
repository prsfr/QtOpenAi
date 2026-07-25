// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Container.h>

namespace QtOpenAi {
namespace Client {

class ContainerReplyPrivate;

// An asynchronous handle for a single container (POST /containers,
// GET/DELETE /containers/{id}). All answer with the container shape,
// including the deletion acknowledgement.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ContainerReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::Container container() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Container &container);

private:
    friend class Client;
    ContainerReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                   QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ContainerReply)
};

} // namespace Client
} // namespace QtOpenAi
