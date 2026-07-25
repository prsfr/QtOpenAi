// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/VectorStoreSearch.h>

namespace QtOpenAi {
namespace Client {

class VectorStoreSearchReplyPrivate;

// A vector-store search request (POST /vector_stores/{id}/search), carrying a
// page of ranked hits.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreSearchReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::VectorStoreSearchPage page() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreSearchPage &page);

private:
    friend class Client;
    VectorStoreSearchReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                           QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(VectorStoreSearchReply)
};

} // namespace Client
} // namespace QtOpenAi
