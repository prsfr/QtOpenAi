// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/VectorStore.h>

namespace QtOpenAi {
namespace Client {

class VectorStoreListReplyPrivate;

// A vector-stores list request (GET /vector_stores).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VectorStoreList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreList &list);

private:
    friend class Client;
    VectorStoreListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                         QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VectorStoreListReply)
};

} // namespace Client
} // namespace QtOpenAi
