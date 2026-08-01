// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ModelInfoData;

// What a model can do and what it costs -- knowledge held locally, as opposed
// to Core::Model, which is the record the API returns for `GET /models`. The
// API says a model exists; this says whether it is worth sending a 200k-token
// prompt to, and what that would cost.
namespace ModelCapability {
enum Flag {
    None = 0x00,
    Tools = 0x01,             // function calling
    Vision = 0x02,            // image parts in the input
    Audio = 0x04,             // audio parts in the input
    StructuredOutputs = 0x08, // response_format: json_schema, strict
    Streaming = 0x10,
    Reasoning = 0x20, // spends reasoning tokens before answering
};
Q_DECLARE_FLAGS(Flags, Flag)
Q_DECLARE_OPERATORS_FOR_FLAGS(Flags)
} // namespace ModelCapability

class QTOPENAI_CORE_EXPORT ModelInfo
{
public:
    ModelInfo();
    explicit ModelInfo(const QString &id);
    ModelInfo(const ModelInfo &other);
    ModelInfo(ModelInfo &&other) noexcept;
    ModelInfo &operator=(const ModelInfo &other);
    ModelInfo &operator=(ModelInfo &&other) noexcept;
    ~ModelInfo();

    void swap(ModelInfo &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // Whether this came from the catalog rather than being the fallback it
    // hands back for a model it has never heard of.
    bool isKnown() const;
    void setKnown(bool known);

    // Total tokens the model accepts, input and output together.
    int contextWindow() const;
    void setContextWindow(int tokens);

    // The largest completion it will produce, which is not the whole window.
    int maxOutputTokens() const;
    void setMaxOutputTokens(int tokens);

    // The tokenizer encoding, e.g. "o200k_base" -- what Core::TokenCounter needs
    // to count for this model.
    QString encoding() const;
    void setEncoding(const QString &encoding);

    ModelCapability::Flags capabilities() const;
    void setCapabilities(ModelCapability::Flags capabilities);
    bool supports(ModelCapability::Flags capabilities) const;

    // US dollars per million tokens, which is the unit the price list uses.
    double inputPrice() const;
    void setInputPrice(double price);

    double outputPrice() const;
    void setOutputPrice(double price);

    // Cached input is billed at its own rate where the model offers it; zero
    // means "no separate rate", not "free".
    double cachedInputPrice() const;
    void setCachedInputPrice(double price);

    QJsonObject toJson() const;
    static ModelInfo fromJson(const QJsonObject &json);

    bool operator==(const ModelInfo &other) const;
    bool operator!=(const ModelInfo &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ModelInfoData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ModelInfo)
