// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/FunctionCall.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class FunctionCallData : public QSharedData
{
public:
    QString name;
    QString arguments;
};

FunctionCall::FunctionCall()
    : d(new FunctionCallData)
{
}

FunctionCall::FunctionCall(QString name, QString arguments)
    : d(new FunctionCallData)
{
    d->name = std::move(name);
    d->arguments = std::move(arguments);
}

FunctionCall::FunctionCall(const FunctionCall &other) = default;
FunctionCall::FunctionCall(FunctionCall &&other) noexcept = default;
FunctionCall &FunctionCall::operator=(const FunctionCall &other) = default;
FunctionCall &FunctionCall::operator=(FunctionCall &&other) noexcept = default;
FunctionCall::~FunctionCall() = default;

QString FunctionCall::name() const { return d->name; }
void FunctionCall::setName(const QString &name) { d->name = name; }

QString FunctionCall::arguments() const { return d->arguments; }
void FunctionCall::setArguments(const QString &arguments) { d->arguments = arguments; }

QJsonObject FunctionCall::argumentsObject() const
{
    if (d->arguments.isEmpty())
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(d->arguments.toUtf8());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

bool FunctionCall::isEmpty() const
{
    return d->name.isEmpty() && d->arguments.isEmpty();
}

QJsonObject FunctionCall::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("name"), d->name);
    // arguments is required by the schema; always emit (even if empty string).
    json.insert(QStringLiteral("arguments"), d->arguments);
    return json;
}

FunctionCall FunctionCall::fromJson(const QJsonObject &json)
{
    FunctionCall call;
    call.d->name = detail::stringOr(json, QStringLiteral("name"));
    call.d->arguments = detail::stringOr(json, QStringLiteral("arguments"));
    return call;
}

bool FunctionCall::operator==(const FunctionCall &other) const
{
    return d->name == other.d->name && d->arguments == other.d->arguments;
}

} // namespace Core
} // namespace QtOpenAi
