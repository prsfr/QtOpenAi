// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/ConversationItemList.h>

namespace QtOpenAi {
namespace Client {

class ConversationItemsReplyPrivate;

// The items of a conversation (/conversations/{id}/items).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT ConversationItemsReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::ConversationItemList items() const;
    // Convenience: the first item, or a default-constructed item when empty.
    Core::ResponseOutputItem firstItem() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ConversationItemList &items);

private:
    friend class Client;
    ConversationItemsReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                           QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(ConversationItemsReply)
};

} // namespace Client
} // namespace QtOpenAi
