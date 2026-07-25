// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/VectorStoreFile.h>

namespace QtOpenAi {
namespace Client {

class VectorStoreFileReplyPrivate;

// An asynchronous handle for a single vector-store file (attach, retrieve,
// update attributes, detach). All answer with the file shape.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreFileReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VectorStoreFile file() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreFile &file);

private:
    friend class Client;
    VectorStoreFileReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                         QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VectorStoreFileReply)
};

} // namespace Client
} // namespace QtOpenAi
