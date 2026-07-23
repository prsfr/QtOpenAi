// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ResponseOutputItem.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ResponseOutputItemData : public QSharedData
{
public:
    QString type;
    QString id;
    QString status;
    QString role;
    QString text;
    QString name;
    QString arguments;
    QString callId;
    QStringList summary;
};

ResponseOutputItem::ResponseOutputItem()
    : d(new ResponseOutputItemData)
{ }

ResponseOutputItem::ResponseOutputItem(const QString &type)
    : d(new ResponseOutputItemData)
{
    d->type = type;
}

ResponseOutputItem::ResponseOutputItem(const ResponseOutputItem &other) = default;
ResponseOutputItem::ResponseOutputItem(ResponseOutputItem &&other) noexcept = default;
ResponseOutputItem &ResponseOutputItem::operator=(const ResponseOutputItem &other) = default;
ResponseOutputItem &ResponseOutputItem::operator=(ResponseOutputItem &&other) noexcept = default;
ResponseOutputItem::~ResponseOutputItem() = default;

QString ResponseOutputItem::type() const { return d->type; }
void ResponseOutputItem::setType(const QString &type) { d->type = type; }

QString ResponseOutputItem::id() const { return d->id; }
void ResponseOutputItem::setId(const QString &id) { d->id = id; }

QString ResponseOutputItem::status() const { return d->status; }
void ResponseOutputItem::setStatus(const QString &status) { d->status = status; }

QString ResponseOutputItem::role() const { return d->role; }
void ResponseOutputItem::setRole(const QString &role) { d->role = role; }

QString ResponseOutputItem::text() const { return d->text; }
void ResponseOutputItem::setText(const QString &text) { d->text = text; }

QString ResponseOutputItem::name() const { return d->name; }
void ResponseOutputItem::setName(const QString &name) { d->name = name; }

QString ResponseOutputItem::arguments() const { return d->arguments; }
void ResponseOutputItem::setArguments(const QString &arguments) { d->arguments = arguments; }

QString ResponseOutputItem::callId() const { return d->callId; }
void ResponseOutputItem::setCallId(const QString &callId) { d->callId = callId; }

QStringList ResponseOutputItem::summary() const { return d->summary; }
void ResponseOutputItem::setSummary(const QStringList &summary) { d->summary = summary; }

ResponseOutputItem ResponseOutputItem::message(const QString &text, const QString &role)
{
    ResponseOutputItem item(QStringLiteral("message"));
    item.d->role = role;
    item.d->text = text;
    return item;
}

ResponseOutputItem ResponseOutputItem::functionCall(const QString &name, const QString &arguments,
                                                    const QString &callId)
{
    ResponseOutputItem item(QStringLiteral("function_call"));
    item.d->name = name;
    item.d->arguments = arguments;
    item.d->callId = callId;
    return item;
}

ResponseOutputItem ResponseOutputItem::reasoning(const QStringList &summary)
{
    ResponseOutputItem item(QStringLiteral("reasoning"));
    item.d->summary = summary;
    return item;
}

QJsonObject ResponseOutputItem::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("type"), d->type);
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("status"), d->status);

    if (d->type == QLatin1String("message")) {
        detail::insertIfNotEmpty(json, QStringLiteral("role"), d->role);
        QJsonObject part;
        part.insert(QStringLiteral("type"), QStringLiteral("output_text"));
        part.insert(QStringLiteral("text"), d->text);
        part.insert(QStringLiteral("annotations"), QJsonArray());
        json.insert(QStringLiteral("content"), QJsonArray {part});
    } else if (d->type == QLatin1String("function_call")) {
        json.insert(QStringLiteral("name"), d->name);
        json.insert(QStringLiteral("arguments"), d->arguments);
        detail::insertIfNotEmpty(json, QStringLiteral("call_id"), d->callId);
    } else if (d->type == QLatin1String("reasoning")) {
        QJsonArray summary;
        for (const QString &line : d->summary) {
            QJsonObject entry;
            entry.insert(QStringLiteral("type"), QStringLiteral("summary_text"));
            entry.insert(QStringLiteral("text"), line);
            summary.append(entry);
        }
        json.insert(QStringLiteral("summary"), summary);
    }
    return json;
}

ResponseOutputItem ResponseOutputItem::fromJson(const QJsonObject &json)
{
    ResponseOutputItem item;
    item.d->type = detail::stringOr(json, QStringLiteral("type"));
    item.d->id = detail::stringOr(json, QStringLiteral("id"));
    item.d->status = detail::stringOr(json, QStringLiteral("status"));
    item.d->role = detail::stringOr(json, QStringLiteral("role"));

    // message: gather text from text content parts. Assistant items use
    // "output_text"; conversation input items use "input_text" — accept both.
    const QJsonArray content = json.value(QStringLiteral("content")).toArray();
    QString text;
    for (const QJsonValue &value : content) {
        const QJsonObject part = value.toObject();
        const QString partType = part.value(QStringLiteral("type")).toString();
        if (partType == QLatin1String("output_text") || partType == QLatin1String("input_text"))
            text += part.value(QStringLiteral("text")).toString();
    }
    item.d->text = text;

    // function_call
    item.d->name = detail::stringOr(json, QStringLiteral("name"));
    item.d->arguments = detail::stringOr(json, QStringLiteral("arguments"));
    item.d->callId = detail::stringOr(json, QStringLiteral("call_id"));

    // reasoning: gather summary_text lines.
    const QJsonArray summary = json.value(QStringLiteral("summary")).toArray();
    for (const QJsonValue &value : summary) {
        const QJsonObject entry = value.toObject();
        if (entry.value(QStringLiteral("type")).toString() == QLatin1String("summary_text"))
            item.d->summary.append(entry.value(QStringLiteral("text")).toString());
    }

    return item;
}

bool ResponseOutputItem::operator==(const ResponseOutputItem &other) const
{
    return d->type == other.d->type && d->id == other.d->id && d->status == other.d->status
           && d->role == other.d->role && d->text == other.d->text && d->name == other.d->name
           && d->arguments == other.d->arguments && d->callId == other.d->callId
           && d->summary == other.d->summary;
}

} // namespace Core
} // namespace QtOpenAi
