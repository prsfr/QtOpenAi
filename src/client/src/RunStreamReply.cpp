// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/RunStreamReply.h"

#include "StreamReplyBase_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

namespace {

// The text of a `thread.message.delta` payload: its delta carries the same
// content parts a finished message does, one fragment at a time.
QString deltaText(const QJsonObject &payload)
{
    QString text;
    const QJsonArray parts = payload.value(QStringLiteral("delta"))
                                     .toObject()
                                     .value(QStringLiteral("content"))
                                     .toArray();
    for (const QJsonValue &value : parts) {
        const QJsonObject part = value.toObject();
        if (part.value(QStringLiteral("type")).toString() != QLatin1String("text"))
            continue;
        text += part.value(QStringLiteral("text"))
                        .toObject()
                        .value(QStringLiteral("value"))
                        .toString();
    }
    return text;
}

// Whether a `thread.message*` event carries a message that will not change
// again. The event name says so directly; a server that omits the name leaves
// the message's own status as the only signal.
bool isSettledMessage(const QString &type, const Core::ThreadMessage &message)
{
    if (type == QLatin1String("thread.message.completed")
        || type == QLatin1String("thread.message.incomplete"))
        return true;
    return type == QLatin1String("thread.message")
           && message.status() != QLatin1String("in_progress");
}

} // namespace

class RunStreamReplyPrivate : public StreamReplyBasePrivate
{
public:
    Core::Run run;
    bool sawTerminal = false;
};

RunStreamReply::RunStreamReply(QNetworkReply *reply, QObject *parent)
    : StreamReplyBase(*new RunStreamReplyPrivate, reply, parent)
{ }

void RunStreamReply::handleEvent(const QByteArray &name, const QByteArray &data)
{
    Q_D(RunStreamReply);

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;
    const QJsonObject object = doc.object();
    // Unlike the other streams, the Assistants events carry their type only in
    // the SSE `event:` field: `thread.message.created` and
    // `thread.message.completed` are both a bare message object, and nothing
    // inside them says which of the two arrived. The payload's own `object` is
    // the fallback for a server that omits the name.
    const QString type = !name.isEmpty() ? QString::fromUtf8(name)
                                         : object.value(QStringLiteral("object")).toString();

    Q_EMIT event(type, object);

    if (type == QLatin1String("thread.message.delta")) {
        const QString text = deltaText(object);
        if (!text.isEmpty())
            Q_EMIT messageDelta(text);
    } else if (type.startsWith(QLatin1String("thread.message"))) {
        // Only the settled message is worth a signal -- `created` and
        // `in_progress` carry the same object with no content yet.
        const Core::ThreadMessage message = Core::ThreadMessage::fromJson(object);
        if (isSettledMessage(type, message))
            Q_EMIT messageCompleted(message);
    } else if (type == QLatin1String("thread.run")
               || (type.startsWith(QLatin1String("thread.run."))
                   && !type.startsWith(QLatin1String("thread.run.step")))) {
        d->run = Core::Run::fromJson(object);
        Q_EMIT runChanged(d->run);
        if (d->run.requiresAction()) {
            d->sawTerminal = true;
            Q_EMIT requiresAction(d->run);
        } else if (d->run.isTerminal()) {
            d->sawTerminal = true;
        }
    } else if (type == QLatin1String("error")) {
        // The error event is the one payload that is not an object of the API's
        // own model: a bare {code, message, param, type}.
        ClientError error(ClientError::Kind::Http,
                          object.value(QStringLiteral("message"))
                                  .toString(QStringLiteral("run stream reported an error")));
        error.setType(object.value(QStringLiteral("type")).toString());
        error.setCode(object.value(QStringLiteral("code")).toString());
        setError(error);
    }
}

bool RunStreamReply::dispatchFinished(int httpStatus)
{
    Q_D(RunStreamReply);
    if (d->sawTerminal) {
        Q_EMIT finished(d->run);
        return true;
    }
    // A run that never reached a terminal state, or one waiting for tool
    // output, is not an answer. An error the stream reported in-band says more
    // about why than anything that can be said here, so it stands.
    if (!d->error.isError()) {
        setError(ClientError(ClientError::Kind::Parse,
                             QStringLiteral("stream ended before the run settled"), httpStatus));
    }
    return false;
}

Core::Run RunStreamReply::run() const
{
    Q_D(const RunStreamReply);
    return d->run;
}

} // namespace Client
} // namespace QtOpenAi
