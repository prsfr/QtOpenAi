// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ResponseOutputItem.h>
#include <QtOpenAi/Core/ResponseUsage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ResponseData;

// A parsed `response` object returned by the Responses API (POST /responses,
// GET /responses/{id}, ...).
class QTOPENAI_CORE_EXPORT Response
{
public:
    Response();
    Response(const Response &other);
    Response(Response &&other) noexcept;
    Response &operator=(const Response &other);
    Response &operator=(Response &&other) noexcept;
    ~Response();

    void swap(Response &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    QString object() const;
    void setObject(const QString &object);

    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString model() const;
    void setModel(const QString &model);

    // Lifecycle: "completed", "in_progress", "failed", "incomplete", "cancelled".
    QString status() const;
    void setStatus(const QString &status);

    QList<ResponseOutputItem> output() const;
    void setOutput(const QList<ResponseOutputItem> &output);
    void addOutput(const ResponseOutputItem &item);

    ResponseUsage usage() const;
    void setUsage(const ResponseUsage &usage);

    // Id of the previous response in a stateful chain, if any.
    QString previousResponseId() const;
    void setPreviousResponseId(const QString &id);

    // Error message when status() == "failed" (empty otherwise).
    QString errorMessage() const;
    void setErrorMessage(const QString &message);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // Convenience: concatenated output_text across all message items.
    QString outputText() const;
    // Convenience: all function_call output items.
    QList<ResponseOutputItem> functionCalls() const;

    QJsonObject toJson() const;
    static Response fromJson(const QJsonObject &json);

    bool operator==(const Response &other) const;
    bool operator!=(const Response &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ResponseData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Response)
