// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ModerationRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class ModerationRequestData : public QSharedData
{
public:
    QJsonValue input;
    QString model;
};

ModerationRequest::ModerationRequest()
    : d(new ModerationRequestData)
{ }

ModerationRequest::ModerationRequest(QString input)
    : d(new ModerationRequestData)
{
    d->input = std::move(input);
}

ModerationRequest::ModerationRequest(const ModerationRequest &other) = default;
ModerationRequest::ModerationRequest(ModerationRequest &&other) noexcept = default;
ModerationRequest &ModerationRequest::operator=(const ModerationRequest &other) = default;
ModerationRequest &ModerationRequest::operator=(ModerationRequest &&other) noexcept = default;
ModerationRequest::~ModerationRequest() = default;

QJsonValue ModerationRequest::input() const { return d->input; }
void ModerationRequest::setInput(const QJsonValue &input) { d->input = input; }
void ModerationRequest::setInput(const QString &input) { d->input = input; }

QString ModerationRequest::model() const { return d->model; }
void ModerationRequest::setModel(const QString &model) { d->model = model; }

QJsonObject ModerationRequest::toJson() const
{
    QJsonObject json;
    if (!d->input.isNull() && !d->input.isUndefined())
        json.insert(QStringLiteral("input"), d->input);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    return json;
}

ModerationRequest ModerationRequest::fromJson(const QJsonObject &json)
{
    ModerationRequest request;
    request.d->input = json.value(QStringLiteral("input"));
    request.d->model = detail::stringOr(json, QStringLiteral("model"));
    return request;
}

bool ModerationRequest::operator==(const ModerationRequest &other) const
{
    return d->input == other.d->input && d->model == other.d->model;
}

} // namespace Core
} // namespace QtOpenAi
