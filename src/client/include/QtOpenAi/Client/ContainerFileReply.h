// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Container.h>

namespace QtOpenAi {
namespace Client {

class ContainerFileReplyPrivate;

// An asynchronous handle for a single file inside a container (add,
// retrieve, delete). All answer with the container-file shape.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ContainerFileReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ContainerFile file() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ContainerFile &file);

private:
    friend class Client;
    ContainerFileReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                       QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ContainerFileReply)
};

} // namespace Client
} // namespace QtOpenAi
