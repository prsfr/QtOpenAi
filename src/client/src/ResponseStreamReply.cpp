// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ResponseStreamReply.h"

#include "StreamReplyBase_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class ResponseStreamReplyPrivate : public StreamReplyBasePrivate
{
public:
    Core::Response response;
    bool sawCompleted = false;
};

ResponseStreamReply::ResponseStreamReply(QNetworkReply *reply, QObject *parent)
    : StreamReplyBase(*new ResponseStreamReplyPrivate, reply, parent)
{ }

void ResponseStreamReply::handleEvent(const QByteArray &name, const QByteArray &data)
{
    Q_D(ResponseStreamReply);
    // These streams name their event type inside the payload, so the SSE
    // `event:` field is of no interest here.
    Q_UNUSED(name);

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;
    const QJsonObject object = doc.object();
    const QString type = object.value(QStringLiteral("type")).toString();

    Q_EMIT event(type, object);

    if (type == QLatin1String("response.output_text.delta")) {
        Q_EMIT outputTextDelta(object.value(QStringLiteral("delta")).toString());
    } else if (type == QLatin1String("response.function_call_arguments.delta")) {
        Q_EMIT functionCallArgumentsDelta(object.value(QStringLiteral("delta")).toString());
    } else if (type == QLatin1String("response.completed")) {
        d->response = Core::Response::fromJson(object.value(QStringLiteral("response")).toObject());
        d->sawCompleted = true;
    } else if (type == QLatin1String("response.failed") || type == QLatin1String("error")) {
        // The error may sit at the event root or inside a response object.
        QJsonObject errorObject = object.value(QStringLiteral("error")).toObject();
        if (errorObject.isEmpty()) {
            const QJsonObject response = object.value(QStringLiteral("response")).toObject();
            errorObject = response.value(QStringLiteral("error")).toObject();
        }
        const QString message = errorObject.value(QStringLiteral("message")).toString(type);
        ClientError error(ClientError::Kind::Http, message);
        error.setType(errorObject.value(QStringLiteral("type")).toString());
        error.setCode(errorObject.value(QStringLiteral("code")).toString());
        setError(error);
    }
}

bool ResponseStreamReply::dispatchFinished(int httpStatus)
{
    Q_D(ResponseStreamReply);
    if (d->sawCompleted) {
        Q_EMIT finished(d->response);
        return true;
    }
    // A stream that stopped short is not a response, however clean the
    // transport was. An error the stream reported in-band says more about why
    // than anything that can be said here, so it stands.
    if (!d->error.isError()) {
        setError(ClientError(ClientError::Kind::Parse,
                             QStringLiteral("stream ended before response.completed"), httpStatus));
    }
    return false;
}

Core::Response ResponseStreamReply::response() const
{
    Q_D(const ResponseStreamReply);
    return d->response;
}

} // namespace Client
} // namespace QtOpenAi
