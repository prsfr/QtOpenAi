// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class AssistantData;

// A configured assistant (POST/GET /assistants, GET/POST/DELETE
// /assistants/{id}) — a model plus the instructions, tools and resources it
// should always run with. Threads are then run against it (see Run).
//
// `tools` is an open union in the spec: a function tool carries a whole schema,
// while `code_interpreter` and `file_search` carry their own configuration
// objects. It is therefore kept as raw JSON rather than half-modelled — build a
// function entry with Tool::function(...).toJson(), which is what
// CreateAssistantRequest::addTool() does for you. `tool_resources` and
// `response_format` are open in the same way and are carried verbatim.
//
// The deletion acknowledgement of DELETE /assistants/{id} also decodes into
// this type; it keeps the id in `id` and reports the object as
// "assistant.deleted".
class QTOPENAI_CORE_EXPORT Assistant
{
public:
    Assistant();
    Assistant(const Assistant &other);
    Assistant(Assistant &&other) noexcept;
    Assistant &operator=(const Assistant &other);
    Assistant &operator=(Assistant &&other) noexcept;
    ~Assistant();

    void swap(Assistant &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "assistant" (or "assistant.deleted").
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    QString model() const;
    void setModel(const QString &model);

    // The system-level instructions the assistant always runs with.
    QString instructions() const;
    void setInstructions(const QString &instructions);

    // The tools available to it (`tools`), verbatim.
    QJsonArray tools() const;
    void setTools(const QJsonArray &tools);

    // Resources the tools may use (`tool_resources`, e.g. the vector store ids
    // for `file_search`), verbatim.
    QJsonObject toolResources() const;
    void setToolResources(const QJsonObject &toolResources);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // Sampling defaults; unset when the API reported null.
    std::optional<double> temperature() const;
    void setTemperature(double temperature);

    std::optional<double> topP() const;
    void setTopP(double topP);

    // response_format: the string "auto" or a format object. Undefined when the
    // field is absent.
    QJsonValue responseFormat() const;
    void setResponseFormat(const QJsonValue &responseFormat);

    QJsonObject toJson() const;
    static Assistant fromJson(const QJsonObject &json);

    bool operator==(const Assistant &other) const;
    bool operator!=(const Assistant &other) const { return !(*this == other); }

private:
    QSharedDataPointer<AssistantData> d;
};

// A `list` of assistants (GET /assistants). Cursor-paginated; reuses the shared
// list-page type.
using AssistantList = ListPage<Assistant>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Assistant)
Q_DECLARE_METATYPE(QtOpenAi::Core::Assistant)
Q_DECLARE_METATYPE(QtOpenAi::Core::AssistantList)
