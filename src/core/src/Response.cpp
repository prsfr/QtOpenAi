// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/Response.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ResponseData : public QSharedData
{
public:
    QString id;
    QString object = QStringLiteral("response");
    qint64 createdAt = 0;
    QString model;
    QString status;
    QList<ResponseOutputItem> output;
    ResponseUsage usage;
    QString previousResponseId;
    QString errorMessage;
    QJsonObject metadata;
};

Response::Response()
    : d(new ResponseData)
{ }

Response::Response(const Response &other) = default;
Response::Response(Response &&other) noexcept = default;
Response &Response::operator=(const Response &other) = default;
Response &Response::operator=(Response &&other) noexcept = default;
Response::~Response() = default;

QString Response::id() const { return d->id; }
void Response::setId(const QString &id) { d->id = id; }

QString Response::object() const { return d->object; }
void Response::setObject(const QString &object) { d->object = object; }

qint64 Response::createdAt() const { return d->createdAt; }
void Response::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QString Response::model() const { return d->model; }
void Response::setModel(const QString &model) { d->model = model; }

QString Response::status() const { return d->status; }
void Response::setStatus(const QString &status) { d->status = status; }

QList<ResponseOutputItem> Response::output() const { return d->output; }
void Response::setOutput(const QList<ResponseOutputItem> &output) { d->output = output; }
void Response::addOutput(const ResponseOutputItem &item) { d->output.append(item); }

ResponseUsage Response::usage() const { return d->usage; }
void Response::setUsage(const ResponseUsage &usage) { d->usage = usage; }

QString Response::previousResponseId() const { return d->previousResponseId; }
void Response::setPreviousResponseId(const QString &id) { d->previousResponseId = id; }

QString Response::errorMessage() const { return d->errorMessage; }
void Response::setErrorMessage(const QString &message) { d->errorMessage = message; }

QJsonObject Response::metadata() const { return d->metadata; }
void Response::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QString Response::outputText() const
{
    QString text;
    for (const ResponseOutputItem &item : d->output) {
        if (item.isMessage())
            text += item.text();
    }
    return text;
}

QList<ResponseOutputItem> Response::functionCalls() const
{
    QList<ResponseOutputItem> calls;
    for (const ResponseOutputItem &item : d->output) {
        if (item.isFunctionCall())
            calls.append(item);
    }
    return calls;
}

QJsonObject Response::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("object"), d->object);
    json.insert(QStringLiteral("created_at"), d->createdAt);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("status"), d->status);

    QJsonArray output;
    for (const ResponseOutputItem &item : d->output)
        output.append(item.toJson());
    json.insert(QStringLiteral("output"), output);

    json.insert(QStringLiteral("usage"), d->usage.toJson());
    detail::insertIfNotEmpty(json, QStringLiteral("previous_response_id"), d->previousResponseId);

    if (!d->errorMessage.isEmpty()) {
        QJsonObject error;
        error.insert(QStringLiteral("message"), d->errorMessage);
        json.insert(QStringLiteral("error"), error);
    } else {
        json.insert(QStringLiteral("error"), QJsonValue::Null);
    }

    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

Response Response::fromJson(const QJsonObject &json)
{
    Response response;
    response.d->id = detail::stringOr(json, QStringLiteral("id"));
    response.d->object
            = detail::stringOr(json, QStringLiteral("object"), QStringLiteral("response"));
    response.d->createdAt
            = static_cast<qint64>(json.value(QStringLiteral("created_at")).toDouble());
    response.d->model = detail::stringOr(json, QStringLiteral("model"));
    response.d->status = detail::stringOr(json, QStringLiteral("status"));

    const QJsonArray output = json.value(QStringLiteral("output")).toArray();
    for (const QJsonValue &value : output)
        response.d->output.append(ResponseOutputItem::fromJson(value.toObject()));

    if (json.contains(QStringLiteral("usage")) && json.value(QStringLiteral("usage")).isObject())
        response.d->usage = ResponseUsage::fromJson(json.value(QStringLiteral("usage")).toObject());

    response.d->previousResponseId = detail::stringOr(json, QStringLiteral("previous_response_id"));

    const QJsonValue error = json.value(QStringLiteral("error"));
    if (error.isObject())
        response.d->errorMessage = error.toObject().value(QStringLiteral("message")).toString();

    response.d->metadata = json.value(QStringLiteral("metadata")).toObject();
    return response;
}

bool Response::operator==(const Response &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->createdAt == other.d->createdAt && d->model == other.d->model
           && d->status == other.d->status && d->output == other.d->output
           && d->usage == other.d->usage && d->previousResponseId == other.d->previousResponseId
           && d->errorMessage == other.d->errorMessage && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
