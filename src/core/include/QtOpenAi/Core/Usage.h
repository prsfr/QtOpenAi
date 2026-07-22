// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>

namespace QtOpenAi {
namespace Core {

class UsageData;

// Token accounting returned with a completion (OpenAI CompletionUsage).
class QTOPENAI_CORE_EXPORT Usage
{
public:
    Usage();
    Usage(const Usage &other);
    Usage(Usage &&other) noexcept;
    Usage &operator=(const Usage &other);
    Usage &operator=(Usage &&other) noexcept;
    ~Usage();

    void swap(Usage &other) noexcept { d.swap(other.d); }

    int promptTokens() const;
    void setPromptTokens(int tokens);

    int completionTokens() const;
    void setCompletionTokens(int tokens);

    int totalTokens() const;
    void setTotalTokens(int tokens);

    QJsonObject toJson() const;
    static Usage fromJson(const QJsonObject &json);

    bool operator==(const Usage &other) const;
    bool operator!=(const Usage &other) const { return !(*this == other); }

private:
    QSharedDataPointer<UsageData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Usage)
