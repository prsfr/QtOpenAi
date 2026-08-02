// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatMap.h"

#include "QtOpenAi/Client/Client.h"

#include <QtCore/QPointer>

namespace QtOpenAi {
namespace Client {

class ChatMapReplyPrivate
{
public:
    QList<ChatMapItem> items;
    QList<Core::ChatCompletionRequest> requests;
    QPointer<Client> client;
    // The replies still running, so abort() has something to abort.
    QList<QPointer<ChatCompletionReply>> running;
    int concurrency = 4;
    int maxFailures = 0;
    int nextToIssue = 0;
    int finished = 0;
    int successes = 0;
    int failures = 0;
    bool aborted = false;
    bool complete = false;
};

ChatMapReply::ChatMapReply(QObject *parent)
    : QObject(parent)
    , d_ptr(new ChatMapReplyPrivate)
{ }

ChatMapReply::~ChatMapReply() = default;

int ChatMapReply::count() const
{
    Q_D(const ChatMapReply);
    return d->items.size();
}

int ChatMapReply::finishedCount() const
{
    Q_D(const ChatMapReply);
    return d->finished;
}

int ChatMapReply::successCount() const
{
    Q_D(const ChatMapReply);
    return d->successes;
}

int ChatMapReply::failureCount() const
{
    Q_D(const ChatMapReply);
    return d->failures;
}

bool ChatMapReply::isFinished() const
{
    Q_D(const ChatMapReply);
    return d->complete;
}

QList<ChatMapItem> ChatMapReply::results() const
{
    Q_D(const ChatMapReply);
    return d->items;
}

ChatMapItem ChatMapReply::result(int index) const
{
    Q_D(const ChatMapReply);
    return d->items.value(index);
}

QStringList ChatMapReply::contents() const
{
    Q_D(const ChatMapReply);
    QStringList texts;
    texts.reserve(d->items.size());
    for (const ChatMapItem &item : d->items)
        texts.append(item.isSuccess() ? item.content() : QString());
    return texts;
}

bool ChatMapReply::isAborted() const
{
    Q_D(const ChatMapReply);
    return d->aborted;
}

void ChatMapReply::abort()
{
    Q_D(ChatMapReply);
    if (d->complete)
        return;
    d->aborted = true;
    // Issue nothing further ...
    d->nextToIssue = d->items.size();
    // ... and let go of what is running.
    for (const QPointer<ChatCompletionReply> &reply : std::as_const(d->running)) {
        if (reply)
            reply->abort();
    }
    // The aborted replies will report failures and drive the run to its end
    // through the ordinary path; if none were running there is nothing left to
    // do it, so finish here. Either way allFinished() fires exactly once --
    // a caller waiting on it must not be left waiting because it gave up.
    if (d->running.isEmpty()) {
        d->complete = true;
        Q_EMIT allFinished();
    }
}

class ChatMapPrivate
{
public:
    QPointer<Client> client;
    int concurrency = 4;
    int maxFailures = 0;

    // Keep the pipe full: issue until `concurrency` are in flight or the input
    // runs out. Called once at the start and again after every item, which is
    // what makes the cap a cap rather than a batch size -- a slow item does not
    // hold back the ones behind it.
    static void pump(ChatMapReply *run)
    {
        ChatMapReplyPrivate *d = run->d_func();

        while (!d->aborted && d->running.size() < d->concurrency
               && d->nextToIssue < d->items.size()) {
            const int index = d->nextToIssue++;
            if (!d->client) {
                report(run, index,
                       ClientError(ClientError::Kind::InvalidRequest,
                                   QStringLiteral("no client to send with")));
                continue;
            }

            ChatCompletionReply *reply = d->client->createChatCompletion(d->requests.at(index));
            d->running.append(reply);

            QObject::connect(reply, &ChatCompletionReply::finished, run,
                             [run, index](const Core::ChatCompletionResponse &response) {
                                 run->d_func()->items[index].response = response;
                             });
            QObject::connect(reply, &ChatCompletionReply::failed, run,
                             [run, index](const ClientError &error) {
                                 run->d_func()->items[index].error = error;
                             });
            // done() rather than the two above, so an item is counted exactly
            // once whichever way it went -- and after both have been recorded.
            QObject::connect(reply, &ChatCompletionReply::done, run,
                             [run, index, reply]() { settle(run, index, reply); });
        }

        finishIfDone(run);
    }

    // An item that never got as far as a request.
    static void report(ChatMapReply *run, int index, const ClientError &error)
    {
        run->d_func()->items[index].error = error;
        settle(run, index, nullptr);
    }

    static void settle(ChatMapReply *run, int index, ChatCompletionReply *reply)
    {
        ChatMapReplyPrivate *d = run->d_func();
        if (reply)
            d->running.removeAll(QPointer<ChatCompletionReply>(reply));

        ChatMapItem &item = d->items[index];
        if (item.finished)
            return; // already counted
        item.finished = true;
        ++d->finished;
        if (item.isSuccess())
            ++d->successes;
        else
            ++d->failures;

        Q_EMIT run->itemFinished(index, item);
        Q_EMIT run->progress(d->finished, d->items.size());

        // A wrong API key fails every item; burning a thousand requests to
        // discover that is a waste worth stopping.
        if (d->maxFailures > 0 && d->failures >= d->maxFailures && !d->aborted) {
            run->abort();
            return;
        }
        pump(run);
    }

    static void finishIfDone(ChatMapReply *run)
    {
        ChatMapReplyPrivate *d = run->d_func();
        if (d->complete || !d->running.isEmpty())
            return;
        // Aborted runs stop with items still unanswered, which is why this asks
        // whether anything is left to do rather than whether everything is done.
        if (!d->aborted && d->nextToIssue < d->items.size())
            return;
        d->complete = true;
        Q_EMIT run->allFinished();
    }
};

ChatMap::ChatMap(Client *client, QObject *parent)
    : QObject(parent)
    , d_ptr(new ChatMapPrivate)
{
    Q_D(ChatMap);
    d->client = client;
}

ChatMap::~ChatMap() = default;

Client *ChatMap::client() const
{
    Q_D(const ChatMap);
    return d->client;
}

void ChatMap::setConcurrency(int count)
{
    Q_D(ChatMap);
    d->concurrency = qMax(1, count);
}

int ChatMap::concurrency() const
{
    Q_D(const ChatMap);
    return d->concurrency;
}

void ChatMap::setMaxFailures(int count)
{
    Q_D(ChatMap);
    d->maxFailures = qMax(0, count);
}

int ChatMap::maxFailures() const
{
    Q_D(const ChatMap);
    return d->maxFailures;
}

ChatMapReply *ChatMap::map(const QList<Core::ChatCompletionRequest> &requests)
{
    Q_D(ChatMap);

    auto *run = new ChatMapReply(this);
    ChatMapReplyPrivate *rd = run->d_func();
    rd->client = d->client;
    rd->concurrency = d->concurrency;
    rd->maxFailures = d->maxFailures;
    rd->requests = requests;

    // Sized up front so results() is index-aligned with the input from the
    // first moment, rather than only once everything has answered.
    rd->items.reserve(requests.size());
    for (int i = 0; i < requests.size(); ++i) {
        ChatMapItem item;
        item.index = i;
        rd->items.append(item);
    }

    // Deferred, so a caller can connect to itemFinished() before anything can
    // fire -- an empty run would otherwise announce itself before it was
    // returned.
    QMetaObject::invokeMethod(run, [run]() { ChatMapPrivate::pump(run); }, Qt::QueuedConnection);
    return run;
}

ChatMapReply *ChatMap::map(const QString &model, const QStringList &prompts)
{
    QList<Core::ChatCompletionRequest> requests;
    requests.reserve(prompts.size());
    for (const QString &prompt : prompts)
        requests.append(Core::ChatCompletionRequest(model, {Core::Message::user(prompt)}));
    return map(requests);
}

} // namespace Client
} // namespace QtOpenAi
