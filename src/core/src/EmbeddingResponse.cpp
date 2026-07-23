// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/EmbeddingResponse.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class EmbeddingData : public QSharedData
{
public:
    int index = 0;
    QList<double> vector;
};

Embedding::Embedding()
    : d(new EmbeddingData)
{ }

Embedding::Embedding(const Embedding &other) = default;
Embedding::Embedding(Embedding &&other) noexcept = default;
Embedding &Embedding::operator=(const Embedding &other) = default;
Embedding &Embedding::operator=(Embedding &&other) noexcept = default;
Embedding::~Embedding() = default;

int Embedding::index() const { return d->index; }
void Embedding::setIndex(int index) { d->index = index; }

QList<double> Embedding::vector() const { return d->vector; }
void Embedding::setVector(const QList<double> &vector) { d->vector = vector; }

QJsonObject Embedding::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("object"), QStringLiteral("embedding"));
    json.insert(QStringLiteral("index"), d->index);
    QJsonArray vector;
    for (double value : d->vector)
        vector.append(value);
    json.insert(QStringLiteral("embedding"), vector);
    return json;
}

Embedding Embedding::fromJson(const QJsonObject &json)
{
    Embedding embedding;
    embedding.d->index = json.value(QStringLiteral("index")).toInt();
    const QJsonArray vector = json.value(QStringLiteral("embedding")).toArray();
    for (const QJsonValue &value : vector)
        embedding.d->vector.append(value.toDouble());
    return embedding;
}

bool Embedding::operator==(const Embedding &other) const
{
    return d->index == other.d->index && d->vector == other.d->vector;
}

class EmbeddingResponseData : public QSharedData
{
public:
    QString model;
    QList<Embedding> data;
    Usage usage;
};

EmbeddingResponse::EmbeddingResponse()
    : d(new EmbeddingResponseData)
{ }

EmbeddingResponse::EmbeddingResponse(const EmbeddingResponse &other) = default;
EmbeddingResponse::EmbeddingResponse(EmbeddingResponse &&other) noexcept = default;
EmbeddingResponse &EmbeddingResponse::operator=(const EmbeddingResponse &other) = default;
EmbeddingResponse &EmbeddingResponse::operator=(EmbeddingResponse &&other) noexcept = default;
EmbeddingResponse::~EmbeddingResponse() = default;

QString EmbeddingResponse::model() const { return d->model; }
void EmbeddingResponse::setModel(const QString &model) { d->model = model; }

QList<Embedding> EmbeddingResponse::data() const { return d->data; }
void EmbeddingResponse::setData(const QList<Embedding> &data) { d->data = data; }

Usage EmbeddingResponse::usage() const { return d->usage; }
void EmbeddingResponse::setUsage(const Usage &usage) { d->usage = usage; }

QList<double> EmbeddingResponse::firstVector() const
{
    return d->data.isEmpty() ? QList<double>() : d->data.first().vector();
}

QJsonObject EmbeddingResponse::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("object"), QStringLiteral("list"));
    QJsonArray data;
    for (const Embedding &embedding : d->data)
        data.append(embedding.toJson());
    json.insert(QStringLiteral("data"), data);
    json.insert(QStringLiteral("model"), d->model);
    json.insert(QStringLiteral("usage"), d->usage.toJson());
    return json;
}

EmbeddingResponse EmbeddingResponse::fromJson(const QJsonObject &json)
{
    EmbeddingResponse response;
    response.d->model = detail::stringOr(json, QStringLiteral("model"));
    const QJsonArray data = json.value(QStringLiteral("data")).toArray();
    for (const QJsonValue &value : data)
        response.d->data.append(Embedding::fromJson(value.toObject()));
    if (json.contains(QStringLiteral("usage")) && json.value(QStringLiteral("usage")).isObject())
        response.d->usage = Usage::fromJson(json.value(QStringLiteral("usage")).toObject());
    return response;
}

bool EmbeddingResponse::operator==(const EmbeddingResponse &other) const
{
    return d->model == other.d->model && d->data == other.d->data && d->usage == other.d->usage;
}

} // namespace Core
} // namespace QtOpenAi
