// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/VectorStoreSearch.h>

namespace QtOpenAi {
namespace Client {

class VectorStoreFileContentReplyPrivate;

// A file-contents request (GET /vector_stores/{id}/files/{file_id}/content),
// carrying a page of the file's parsed text chunks.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreFileContentReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VectorStoreFileContentPage page() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreFileContentPage &page);

private:
    friend class Client;
    VectorStoreFileContentReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                                QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VectorStoreFileContentReply)
};

} // namespace Client
} // namespace QtOpenAi
