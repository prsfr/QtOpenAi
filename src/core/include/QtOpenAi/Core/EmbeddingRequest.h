// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class EmbeddingRequestData;

// The body of a POST /embeddings request. `input` may be a string or an array of
// strings/token arrays (via the QJsonValue overload). Optional parameters are
// only serialised when explicitly set.
class QTOPENAI_CORE_EXPORT EmbeddingRequest
{
public:
    EmbeddingRequest();
    EmbeddingRequest(QString model, QString input);
    EmbeddingRequest(const EmbeddingRequest &other);
    EmbeddingRequest(EmbeddingRequest &&other) noexcept;
    EmbeddingRequest &operator=(const EmbeddingRequest &other);
    EmbeddingRequest &operator=(EmbeddingRequest &&other) noexcept;
    ~EmbeddingRequest();

    void swap(EmbeddingRequest &other) noexcept { d.swap(other.d); }

    QString model() const;
    void setModel(const QString &model);

    QJsonValue input() const;
    void setInput(const QJsonValue &input);
    void setInput(const QString &input);

    // Number of output dimensions (supported models only); unset omits it.
    std::optional<int> dimensions() const;
    void setDimensions(int dimensions);

    // "float" (default) or "base64"; empty omits it.
    QString encodingFormat() const;
    void setEncodingFormat(const QString &format);

    QString user() const;
    void setUser(const QString &user);

    QJsonObject toJson() const;
    static EmbeddingRequest fromJson(const QJsonObject &json);

    bool operator==(const EmbeddingRequest &other) const;
    bool operator!=(const EmbeddingRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<EmbeddingRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::EmbeddingRequest)
