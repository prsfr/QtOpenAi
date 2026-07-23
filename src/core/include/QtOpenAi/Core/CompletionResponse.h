// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/CompletionChoice.h>
#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Usage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class CompletionResponseData;

// A parsed legacy `text_completion` response (POST /completions).
class QTOPENAI_CORE_EXPORT CompletionResponse
{
public:
    CompletionResponse();
    CompletionResponse(const CompletionResponse &other);
    CompletionResponse(CompletionResponse &&other) noexcept;
    CompletionResponse &operator=(const CompletionResponse &other);
    CompletionResponse &operator=(CompletionResponse &&other) noexcept;
    ~CompletionResponse();

    void swap(CompletionResponse &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    QString object() const;
    void setObject(const QString &object);

    qint64 created() const;
    void setCreated(qint64 created);

    QString model() const;
    void setModel(const QString &model);

    QList<CompletionChoice> choices() const;
    void setChoices(const QList<CompletionChoice> &choices);

    Usage usage() const;
    void setUsage(const Usage &usage);

    // Convenience: the first choice's text, or an empty string.
    QString firstText() const;

    QJsonObject toJson() const;
    static CompletionResponse fromJson(const QJsonObject &json);

    bool operator==(const CompletionResponse &other) const;
    bool operator!=(const CompletionResponse &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CompletionResponseData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CompletionResponse)
