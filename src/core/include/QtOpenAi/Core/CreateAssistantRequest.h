// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ResponseFormat.h>
#include <QtOpenAi/Core/Tool.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class CreateAssistantRequestData;

// The body of a POST /assistants request — and of POST /assistants/{id}, which
// modifies an assistant with the very same fields, all of them optional. Only
// what was set is serialised, so an update body carries exactly the changes.
//
// The `tools` array is an open union (function / code_interpreter /
// file_search), so it is built as raw JSON. addTool() takes either a typed
// function Tool or a ready-made object, and the two hosted-tool helpers spell
// the built-in entries for you.
class QTOPENAI_CORE_EXPORT CreateAssistantRequest
{
public:
    CreateAssistantRequest();
    explicit CreateAssistantRequest(QString model);
    CreateAssistantRequest(const CreateAssistantRequest &other);
    CreateAssistantRequest(CreateAssistantRequest &&other) noexcept;
    CreateAssistantRequest &operator=(const CreateAssistantRequest &other);
    CreateAssistantRequest &operator=(CreateAssistantRequest &&other) noexcept;
    ~CreateAssistantRequest();

    void swap(CreateAssistantRequest &other) noexcept { d.swap(other.d); }

    QString model() const;
    void setModel(const QString &model);

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    QString instructions() const;
    void setInstructions(const QString &instructions);

    QJsonArray tools() const;
    void setTools(const QJsonArray &tools);

    // Append a function tool, described the way the rest of the library
    // describes one.
    void addTool(const Tool &tool);
    // Append a tool entry verbatim, for tool types this library does not model.
    void addTool(const QJsonObject &tool);
    // The two hosted tools, which carry no schema of their own.
    void addCodeInterpreterTool();
    void addFileSearchTool();

    QJsonObject toolResources() const;
    void setToolResources(const QJsonObject &toolResources);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    std::optional<double> temperature() const;
    void setTemperature(double temperature);

    std::optional<double> topP() const;
    void setTopP(double topP);

    // response_format: "auto" or a format object. The ResponseFormat overload
    // serialises the typed value used elsewhere in the library.
    QJsonValue responseFormat() const;
    void setResponseFormat(const QJsonValue &responseFormat);
    void setResponseFormat(const ResponseFormat &responseFormat);

    QJsonObject toJson() const;

    bool operator==(const CreateAssistantRequest &other) const;
    bool operator!=(const CreateAssistantRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateAssistantRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateAssistantRequest)
