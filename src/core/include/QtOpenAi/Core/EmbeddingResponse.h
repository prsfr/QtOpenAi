// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Usage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class EmbeddingData;

// A single embedding vector and its position in the input batch.
class QTOPENAI_CORE_EXPORT Embedding
{
public:
    Embedding();
    Embedding(const Embedding &other);
    Embedding(Embedding &&other) noexcept;
    Embedding &operator=(const Embedding &other);
    Embedding &operator=(Embedding &&other) noexcept;
    ~Embedding();

    void swap(Embedding &other) noexcept { d.swap(other.d); }

    int index() const;
    void setIndex(int index);

    // The float vector (decoded from the default "float" encoding_format).
    QList<double> vector() const;
    void setVector(const QList<double> &vector);

    QJsonObject toJson() const;
    static Embedding fromJson(const QJsonObject &json);

    bool operator==(const Embedding &other) const;
    bool operator!=(const Embedding &other) const { return !(*this == other); }

private:
    QSharedDataPointer<EmbeddingData> d;
};

class EmbeddingResponseData;

// A parsed `list` of embeddings (POST /embeddings).
class QTOPENAI_CORE_EXPORT EmbeddingResponse
{
public:
    EmbeddingResponse();
    EmbeddingResponse(const EmbeddingResponse &other);
    EmbeddingResponse(EmbeddingResponse &&other) noexcept;
    EmbeddingResponse &operator=(const EmbeddingResponse &other);
    EmbeddingResponse &operator=(EmbeddingResponse &&other) noexcept;
    ~EmbeddingResponse();

    void swap(EmbeddingResponse &other) noexcept { d.swap(other.d); }

    QString model() const;
    void setModel(const QString &model);

    QList<Embedding> data() const;
    void setData(const QList<Embedding> &data);

    Usage usage() const;
    void setUsage(const Usage &usage);

    // Convenience: the first embedding's vector, or an empty list.
    QList<double> firstVector() const;

    QJsonObject toJson() const;
    static EmbeddingResponse fromJson(const QJsonObject &json);

    bool operator==(const EmbeddingResponse &other) const;
    bool operator!=(const EmbeddingResponse &other) const { return !(*this == other); }

private:
    QSharedDataPointer<EmbeddingResponseData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Embedding)
Q_DECLARE_SHARED(QtOpenAi::Core::EmbeddingResponse)
