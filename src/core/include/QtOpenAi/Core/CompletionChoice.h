// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class CompletionChoiceData;

// One choice of a legacy `text_completion` response: the generated text, its
// index, an optional logprobs payload, and the finish reason.
class QTOPENAI_CORE_EXPORT CompletionChoice
{
public:
    CompletionChoice();
    CompletionChoice(const CompletionChoice &other);
    CompletionChoice(CompletionChoice &&other) noexcept;
    CompletionChoice &operator=(const CompletionChoice &other);
    CompletionChoice &operator=(CompletionChoice &&other) noexcept;
    ~CompletionChoice();

    void swap(CompletionChoice &other) noexcept { d.swap(other.d); }

    QString text() const;
    void setText(const QString &text);

    int index() const;
    void setIndex(int index);

    // "stop", "length", "content_filter", ... (empty when absent).
    QString finishReason() const;
    void setFinishReason(const QString &finishReason);

    // Opaque logprobs payload (null when absent).
    QJsonValue logprobs() const;
    void setLogprobs(const QJsonValue &logprobs);

    QJsonObject toJson() const;
    static CompletionChoice fromJson(const QJsonObject &json);

    bool operator==(const CompletionChoice &other) const;
    bool operator!=(const CompletionChoice &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CompletionChoiceData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CompletionChoice)
