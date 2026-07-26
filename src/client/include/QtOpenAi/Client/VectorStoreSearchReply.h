// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/VectorStoreSearch.h>

namespace QtOpenAi {
namespace Client {

// A vector-store search request (POST /vector_stores/{id}/search), carrying a
// page of ranked hits.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreSearchReply : public TypedReply<Core::VectorStoreSearchPage>
{
    Q_OBJECT
public:
    Core::VectorStoreSearchPage page() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreSearchPage &page);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VectorStoreSearchPage &page) override { Q_EMIT finished(page); }
};

} // namespace Client
} // namespace QtOpenAi
