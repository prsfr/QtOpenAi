// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/VectorStoreSearch.h>

namespace QtOpenAi {
namespace Client {

// A file-contents request (GET /vector_stores/{id}/files/{file_id}/content),
// carrying a page of the file's parsed text chunks.
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT VectorStoreFileContentReply
    : public TypedReply<Core::VectorStoreFileContentPage>
{
    Q_OBJECT
public:
    Core::VectorStoreFileContentPage page() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::VectorStoreFileContentPage &page);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::VectorStoreFileContentPage &page) override
    {
        Q_EMIT finished(page);
    }
};

} // namespace Client
} // namespace QtOpenAi
