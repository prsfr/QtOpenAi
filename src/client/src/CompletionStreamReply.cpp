// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/CompletionStreamReply.h"

#include "StreamReplyBase_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class CompletionStreamReplyPrivate : public StreamReplyBasePrivate
{
public:
    // Accumulated fields reassembled from the streamed chunks.
    QString id;
    QString model;
    QString text;
    QString finishReason;

    Core::CompletionResponse response() const
    {
        Core::CompletionResponse response;
        response.setId(id);
        response.setModel(model);
        Core::CompletionChoice choice;
        choice.setText(text);
        choice.setIndex(0);
        choice.setFinishReason(finishReason);
        response.setChoices({choice});
        return response;
    }
};

CompletionStreamReply::CompletionStreamReply(QNetworkReply *reply, QObject *parent)
    : StreamReplyBase(*new CompletionStreamReplyPrivate, reply, parent)
{ }

void CompletionStreamReply::handleEvent(const QByteArray &name, const QByteArray &data)
{
    Q_D(CompletionStreamReply);
    // These streams name their event type inside the payload, so the SSE
    // `event:` field is of no interest here.
    Q_UNUSED(name);

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;
    const QJsonObject object = doc.object();

    if (const QString id = object.value(QStringLiteral("id")).toString(); !id.isEmpty())
        d->id = id;
    if (const QString model = object.value(QStringLiteral("model")).toString(); !model.isEmpty())
        d->model = model;

    const QJsonArray choices = object.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty())
        return;
    const QJsonObject choice = choices.first().toObject();
    const QString fragment = choice.value(QStringLiteral("text")).toString();
    if (const QString reason = choice.value(QStringLiteral("finish_reason")).toString();
        !reason.isEmpty())
        d->finishReason = reason;
    if (!fragment.isEmpty()) {
        d->text += fragment;
        Q_EMIT textDelta(fragment);
    }
}

bool CompletionStreamReply::dispatchFinished(int httpStatus)
{
    Q_D(CompletionStreamReply);
    // Like the chat stream, this one ends with the [DONE] sentinel and nothing
    // else, so a transport that finished cleanly is the answer.
    Q_UNUSED(httpStatus);
    Q_EMIT finished(d->response());
    return true;
}

Core::CompletionResponse CompletionStreamReply::response() const
{
    Q_D(const CompletionStreamReply);
    return d->response();
}

} // namespace Client
} // namespace QtOpenAi
