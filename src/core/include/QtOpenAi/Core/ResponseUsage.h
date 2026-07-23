// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>

namespace QtOpenAi {
namespace Core {

class ResponseUsageData;

// Token accounting returned with a Responses-API result (OpenAI ResponseUsage).
// Unlike the Chat Completions Usage, tokens are named input/output and the
// output count carries a reasoning-token breakdown.
class QTOPENAI_CORE_EXPORT ResponseUsage
{
public:
    ResponseUsage();
    ResponseUsage(const ResponseUsage &other);
    ResponseUsage(ResponseUsage &&other) noexcept;
    ResponseUsage &operator=(const ResponseUsage &other);
    ResponseUsage &operator=(ResponseUsage &&other) noexcept;
    ~ResponseUsage();

    void swap(ResponseUsage &other) noexcept { d.swap(other.d); }

    int inputTokens() const;
    void setInputTokens(int tokens);

    int outputTokens() const;
    void setOutputTokens(int tokens);

    int totalTokens() const;
    void setTotalTokens(int tokens);

    // Reasoning tokens included in outputTokens (output_tokens_details).
    int reasoningTokens() const;
    void setReasoningTokens(int tokens);

    QJsonObject toJson() const;
    static ResponseUsage fromJson(const QJsonObject &json);

    bool operator==(const ResponseUsage &other) const;
    bool operator!=(const ResponseUsage &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ResponseUsageData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ResponseUsage)
