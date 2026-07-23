// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/EmbeddingRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class EmbeddingRequestData : public QSharedData
{
public:
    QString model;
    QJsonValue input;
    std::optional<int> dimensions;
    QString encodingFormat;
    QString user;
};

EmbeddingRequest::EmbeddingRequest()
    : d(new EmbeddingRequestData)
{ }

EmbeddingRequest::EmbeddingRequest(QString model, QString input)
    : d(new EmbeddingRequestData)
{
    d->model = std::move(model);
    d->input = std::move(input);
}

EmbeddingRequest::EmbeddingRequest(const EmbeddingRequest &other) = default;
EmbeddingRequest::EmbeddingRequest(EmbeddingRequest &&other) noexcept = default;
EmbeddingRequest &EmbeddingRequest::operator=(const EmbeddingRequest &other) = default;
EmbeddingRequest &EmbeddingRequest::operator=(EmbeddingRequest &&other) noexcept = default;
EmbeddingRequest::~EmbeddingRequest() = default;

QString EmbeddingRequest::model() const { return d->model; }
void EmbeddingRequest::setModel(const QString &model) { d->model = model; }

QJsonValue EmbeddingRequest::input() const { return d->input; }
void EmbeddingRequest::setInput(const QJsonValue &input) { d->input = input; }
void EmbeddingRequest::setInput(const QString &input) { d->input = input; }

std::optional<int> EmbeddingRequest::dimensions() const { return d->dimensions; }
void EmbeddingRequest::setDimensions(int dimensions) { d->dimensions = dimensions; }

QString EmbeddingRequest::encodingFormat() const { return d->encodingFormat; }
void EmbeddingRequest::setEncodingFormat(const QString &format) { d->encodingFormat = format; }

QString EmbeddingRequest::user() const { return d->user; }
void EmbeddingRequest::setUser(const QString &user) { d->user = user; }

QJsonObject EmbeddingRequest::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("model"), d->model);
    if (!d->input.isNull() && !d->input.isUndefined())
        json.insert(QStringLiteral("input"), d->input);
    if (d->dimensions)
        json.insert(QStringLiteral("dimensions"), *d->dimensions);
    detail::insertIfNotEmpty(json, QStringLiteral("encoding_format"), d->encodingFormat);
    detail::insertIfNotEmpty(json, QStringLiteral("user"), d->user);
    return json;
}

EmbeddingRequest EmbeddingRequest::fromJson(const QJsonObject &json)
{
    EmbeddingRequest request;
    request.d->model = detail::stringOr(json, QStringLiteral("model"));
    request.d->input = json.value(QStringLiteral("input"));
    if (json.contains(QStringLiteral("dimensions")))
        request.d->dimensions = json.value(QStringLiteral("dimensions")).toInt();
    request.d->encodingFormat = detail::stringOr(json, QStringLiteral("encoding_format"));
    request.d->user = detail::stringOr(json, QStringLiteral("user"));
    return request;
}

bool EmbeddingRequest::operator==(const EmbeddingRequest &other) const
{
    return d->model == other.d->model && d->input == other.d->input
           && d->dimensions == other.d->dimensions && d->encodingFormat == other.d->encodingFormat
           && d->user == other.d->user;
}

} // namespace Core
} // namespace QtOpenAi
