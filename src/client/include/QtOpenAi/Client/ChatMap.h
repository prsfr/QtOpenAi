// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/ChatCompletionRequest.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Client {

class Client;
class ChatMapPrivate;
class ChatMapReplyPrivate;

// One item of a mapped run: which input it answers, and what came back.
//
// Success and failure sit side by side rather than in separate lists, because
// the caller almost always wants them in input order -- classifying a thousand
// rows is only useful if row 837's answer can still be found at 837 after two
// of its neighbours failed.
struct QTOPENAI_CLIENT_EXPORT ChatMapItem
{
    int index = -1;
    Core::ChatCompletionResponse response;
    ClientError error;
    bool finished = false;

    bool isSuccess() const { return finished && !error.isError(); }
    // The answer's text, the thing a mapped run is usually after.
    QString content() const { return response.choices().value(0).message().content(); }
};

// A mapped run in progress.
class QTOPENAI_CLIENT_EXPORT ChatMapReply : public QObject
{
    Q_OBJECT
public:
    ~ChatMapReply() override;

    int count() const;
    int finishedCount() const;
    int successCount() const;
    int failureCount() const;
    bool isFinished() const;

    // Ordered and index-aligned with the input from the first moment, so
    // results().at(i) always refers to input i even before it has an answer.
    QList<ChatMapItem> results() const;
    ChatMapItem result(int index) const;
    // Just the text, in order. Failed items contribute an empty string -- for
    // the common case where a caller wants a column, not a report.
    QStringList contents() const;

    // Stop. Nothing further is issued and what is in flight is abandoned;
    // whatever finished stays. allFinished() still fires, because a caller
    // waiting on it would otherwise wait forever.
    void abort();
    bool isAborted() const;

Q_SIGNALS:
    void itemFinished(int index, const QtOpenAi::Client::ChatMapItem &item);
    // How far along the run is. Emitted with every item, after itemFinished().
    void progress(int finished, int total);
    void allFinished();

private:
    friend class ChatMap;
    // The dispatch loop lives there, so it needs the state it is driving.
    friend class ChatMapPrivate;
    explicit ChatMapReply(QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(ChatMapReply)
    QScopedPointer<ChatMapReplyPrivate> d_ptr;
};

// Runs many prompts and collects the answers in order.
//
//     ChatMap map(&client);
//     map.setConcurrency(4);
//
//     auto *run = map.map(QStringLiteral("gpt-4o-mini"), prompts);
//     connect(run, &ChatMapReply::allFinished, this, [run]() { use(run->contents()); });
//
// The shape of classification over a dataset, fan-out summarisation, and
// offline evals: N requests that have nothing to do with each other, which
// should go out together but not all at once.
//
// **A failed item does not fail the run.** One row of a thousand hitting a
// content filter is not a reason to throw away the other nine hundred and
// ninety-nine; the error is recorded against its index and the run carries on.
// setMaxFailures() is there for the case where it *is* a reason -- a wrong API
// key fails every item, and burning a thousand requests to discover that is a
// waste worth stopping.
//
// This is the client-side counterpart to the server-side Batch API, and the
// trade is latency against cost: a batch job is cheaper and answers in hours,
// this answers in seconds at full price.
//
// Concurrency here is the number of requests this run keeps in flight, and it
// composes with rather than replaces Client::setRateLimiter(): a limiter
// governs everything the client does, a ChatMap governs one run. With both,
// whichever is stricter decides.
class QTOPENAI_CLIENT_EXPORT ChatMap : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int concurrency READ concurrency WRITE setConcurrency)
    Q_PROPERTY(int maxFailures READ maxFailures WRITE setMaxFailures)
public:
    explicit ChatMap(Client *client, QObject *parent = nullptr);
    ~ChatMap() override;

    Client *client() const;

    // Requests in flight at once. Default 4 -- enough to be worth doing, low
    // enough not to be the reason a provider starts refusing.
    void setConcurrency(int count);
    int concurrency() const;

    // Abandon the run after this many item failures. 0 (the default) means
    // never.
    void setMaxFailures(int count);
    int maxFailures() const;

    // One request per item, answered in input order.
    ChatMapReply *map(const QList<Core::ChatCompletionRequest> &requests);
    // The common case: the same model, one user message per prompt.
    ChatMapReply *map(const QString &model, const QStringList &prompts);

private:
    Q_DECLARE_PRIVATE(ChatMap)
    QScopedPointer<ChatMapPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi

Q_DECLARE_METATYPE(QtOpenAi::Client::ChatMapItem)
