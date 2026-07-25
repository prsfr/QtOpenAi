// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Container.h>

namespace QtOpenAi {
namespace Client {

class ContainerFileListReplyPrivate;

// A container-files list request (GET /containers/{id}/files).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ContainerFileListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ContainerFileList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ContainerFileList &list);

private:
    friend class Client;
    ContainerFileListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                           QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ContainerFileListReply)
};

} // namespace Client
} // namespace QtOpenAi
