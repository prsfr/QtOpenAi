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

class CompletionRequestData;

// The body of a POST /completions (legacy text completion) request. Optional
// parameters are only serialised when explicitly set. The `prompt` may be a
// string or an array of strings/tokens (via the QJsonValue overload).
class QTOPENAI_CORE_EXPORT CompletionRequest
{
public:
    CompletionRequest();
    CompletionRequest(QString model, QString prompt);
    CompletionRequest(const CompletionRequest &other);
    CompletionRequest(CompletionRequest &&other) noexcept;
    CompletionRequest &operator=(const CompletionRequest &other);
    CompletionRequest &operator=(CompletionRequest &&other) noexcept;
    ~CompletionRequest();

    void swap(CompletionRequest &other) noexcept { d.swap(other.d); }

    QString model() const;
    void setModel(const QString &model);

    QJsonValue prompt() const;
    void setPrompt(const QJsonValue &prompt);
    void setPrompt(const QString &prompt);

    std::optional<int> maxTokens() const;
    void setMaxTokens(int maxTokens);

    std::optional<double> temperature() const;
    void setTemperature(double temperature);

    std::optional<double> topP() const;
    void setTopP(double topP);

    std::optional<int> n() const;
    void setN(int n);

    std::optional<bool> echo() const;
    void setEcho(bool echo);

    // stop: string or array of strings; unset omits the field.
    std::optional<QJsonValue> stop() const;
    void setStop(const QJsonValue &stop);

    std::optional<double> presencePenalty() const;
    void setPresencePenalty(double penalty);

    std::optional<double> frequencyPenalty() const;
    void setFrequencyPenalty(double penalty);

    std::optional<int> bestOf() const;
    void setBestOf(int bestOf);

    std::optional<int> seed() const;
    void setSeed(int seed);

    QString suffix() const;
    void setSuffix(const QString &suffix);

    std::optional<bool> stream() const;
    void setStream(bool stream);

    // Extra provider-specific fields merged verbatim into the request body.
    QJsonObject extraBody() const;
    void setExtraBody(const QJsonObject &extra);

    QJsonObject toJson() const;
    static CompletionRequest fromJson(const QJsonObject &json);

    bool operator==(const CompletionRequest &other) const;
    bool operator!=(const CompletionRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CompletionRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CompletionRequest)
