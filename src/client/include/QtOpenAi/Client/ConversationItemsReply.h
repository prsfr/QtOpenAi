// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Core/ConversationItemList.h>

#include <QtCore/QObject>

#include <functional>

class QNetworkReply;

namespace QtOpenAi {
namespace Client {

class ConversationItemsReplyPrivate;

// An asynchronous handle for a Conversations-API request whose result is a list
// of items (list items, create items) or a single item (get item, surfaced as a
// one-element list). Emits finished() on success and failed() on error; both are
// followed by done(). Auto-deletes after done() unless setAutoDelete(false).
class QTOPENAI_CLIENT_EXPORT ConversationItemsReply : public QObject
{
    Q_OBJECT
public:
    ~ConversationItemsReply() override;

    bool isFinished() const;
    bool isSuccess() const;

    Core::ConversationItemList items() const;
    // Convenience: the first item, or a default-constructed item when empty.
    Core::ResponseOutputItem firstItem() const;

    ClientError error() const;

    RateLimit rateLimit() const;
    int retryCount() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    void abort();

Q_SIGNALS:
    void finished(const QtOpenAi::Core::ConversationItemList &items);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();
    void retrying(int attempt, int delayMs);

private:
    friend class Client;
    ConversationItemsReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                           QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(ConversationItemsReply)
    QScopedPointer<ConversationItemsReplyPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
