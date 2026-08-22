// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChatCompletionStreamReply.h"

#include "QtOpenAi/Client/ChatCompletionAccumulator.h"
#include "StreamReplyBase_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ChatCompletionStreamReplyPrivate : public StreamReplyBasePrivate
{
public:
    ChatCompletionAccumulator accumulator;
};

ChatCompletionStreamReply::ChatCompletionStreamReply(QNetworkReply *reply, QObject *parent)
    : StreamReplyBase(*new ChatCompletionStreamReplyPrivate, reply, parent)
{ }

void ChatCompletionStreamReply::handleEvent(const QByteArray &name, const QByteArray &data)
{
    Q_D(ChatCompletionStreamReply);
    // These streams name their event type inside the payload, so the SSE
    // `event:` field is of no interest here.
    Q_UNUSED(name);

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;

    const Core::ChatCompletionChunk chunk = Core::ChatCompletionChunk::fromJson(doc.object());
    d->accumulator.add(chunk);
    Q_EMIT delta(chunk);

    if (!chunk.choices().isEmpty()) {
        const Core::ChoiceDelta first = chunk.choices().first().delta();
        if (first.hasContent() && !first.content().isEmpty())
            Q_EMIT contentDelta(first.content());
    }
}

bool ChatCompletionStreamReply::dispatchFinished(int httpStatus)
{
    Q_D(ChatCompletionStreamReply);
    // A chat completion stream carries no end-of-answer event of its own beyond
    // the [DONE] sentinel, so a transport that finished cleanly is the answer.
    Q_UNUSED(httpStatus);
    Q_EMIT finished(d->accumulator.response());
    return true;
}

Core::ChatCompletionResponse ChatCompletionStreamReply::response() const
{
    Q_D(const ChatCompletionStreamReply);
    return d->accumulator.response();
}

} // namespace Client
} // namespace QtOpenAi
