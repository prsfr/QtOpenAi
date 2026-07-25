// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/VectorStoreFile.h>

namespace QtOpenAi {
namespace Client {

class VectorStoreFileListReplyPrivate;

// A vector-store files list request — both the store's own files and the
// files of a single batch, which share one response shape.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreFileListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VectorStoreFileList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreFileList &list);

private:
    friend class Client;
    VectorStoreFileListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                             QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VectorStoreFileListReply)
};

} // namespace Client
} // namespace QtOpenAi
