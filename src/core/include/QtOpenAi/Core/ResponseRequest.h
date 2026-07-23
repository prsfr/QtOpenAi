// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ResponseFormat.h>
#include <QtOpenAi/Core/Tool.h>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class ResponseRequestData;

// The body of a POST /responses request. Optional parameters are only
// serialised when explicitly set, matching the OpenAI schema semantics.
//
// The `input` may be a plain string or a structured item array; both are
// supported through the overloaded setInput() and the QJsonValue accessor.
class QTOPENAI_CORE_EXPORT ResponseRequest
{
public:
    ResponseRequest();
    ResponseRequest(QString model, QString input);
    ResponseRequest(const ResponseRequest &other);
    ResponseRequest(ResponseRequest &&other) noexcept;
    ResponseRequest &operator=(const ResponseRequest &other);
    ResponseRequest &operator=(ResponseRequest &&other) noexcept;
    ~ResponseRequest();

    void swap(ResponseRequest &other) noexcept { d.swap(other.d); }

    QString model() const;
    void setModel(const QString &model);

    // The `input` field, as an opaque JSON value (string or item array).
    QJsonValue input() const;
    void setInput(const QJsonValue &input);
    void setInput(const QString &input);

    // System / developer instructions prepended to the model context.
    QString instructions() const;
    void setInstructions(const QString &instructions);

    QList<Tool> tools() const;
    void setTools(const QList<Tool> &tools);
    void addTool(const Tool &tool);

    // tool_choice: "auto", "none", "required", or a specific tool object.
    std::optional<QJsonValue> toolChoice() const;
    void setToolChoice(const QJsonValue &toolChoice);

    // text.format: constrained decoding (text / json_object / json_schema).
    // The schema fields are inlined under text.format per the Responses schema.
    std::optional<ResponseFormat> textFormat() const;
    void setTextFormat(const ResponseFormat &format);

    std::optional<int> maxOutputTokens() const;
    void setMaxOutputTokens(int tokens);

    std::optional<double> temperature() const;
    void setTemperature(double temperature);

    std::optional<double> topP() const;
    void setTopP(double topP);

    // Whether the server should persist the response for later retrieval.
    std::optional<bool> store() const;
    void setStore(bool store);

    // Chain onto a previous stored response.
    QString previousResponseId() const;
    void setPreviousResponseId(const QString &id);

    // Reasoning effort ("minimal", "low", "medium", "high"); empty omits it.
    QString reasoningEffort() const;
    void setReasoningEffort(const QString &effort);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    std::optional<bool> stream() const;
    void setStream(bool stream);

    // Extra provider-specific fields merged verbatim into the request body.
    QJsonObject extraBody() const;
    void setExtraBody(const QJsonObject &extra);

    QJsonObject toJson() const;
    static ResponseRequest fromJson(const QJsonObject &json);

    bool operator==(const ResponseRequest &other) const;
    bool operator!=(const ResponseRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ResponseRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ResponseRequest)
