// SPDX-License-Identifier: MIT
#pragma once

// The fields an assistant *is*, as opposed to what the server stamped on it --
// shared by the `assistant` resource and the body of POST /assistants.
//
// Why this exists: CreateAssistantRequestData was a strict subset of
// AssistantData, ten members with identical names and types, and the two
// toJson() tails were byte-identical. Nothing in the build connected them, so
// the failure mode was silent and one-directional: the API adds a field, it gets
// added to Assistant so a user can read it back, and forgotten on
// CreateAssistantRequest so the user cannot set it. The symptom is a setting
// that reverts to the server default.
//
// Private, and deliberately so. These are Q_DECLARE_SHARED value types with
// public non-virtual destructors, so a shared *public* base would make slicing
// silent and `AssistantFields &ref = someAssistant;` legal, and would let
// operator== compare ten fields across two unrelated types. The d-pointer is
// opaque behind QSharedDataPointer, so sharing it costs nothing visible: no
// installed header, no exported symbol, no change to the class hierarchy users
// see.
//
// Not installed. See src/core/src/RunFields_p.h for the same arrangement over
// the thirteen fields Run and CreateRunRequest share.

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QSharedData>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {
namespace detail {

class AssistantFieldsData : public QSharedData
{
public:
    QString name;
    QString description;
    QString model;
    QString instructions;
    QJsonArray tools;
    QJsonObject toolResources;
    QJsonObject metadata;
    std::optional<double> temperature;
    std::optional<double> topP;
    QJsonValue responseFormat = QJsonValue::Undefined;
};

// The one place the ten fields' wire form is written down. The two copies this
// replaces inserted in different orders (Assistant: name, description, model...;
// the request: model, name, description...), which never mattered and still does
// not: QJsonObject keeps its keys sorted, so what is emitted is identical either
// way.
inline void insertAssistantFields(QJsonObject &json, const AssistantFieldsData &d)
{
    insertIfNotEmpty(json, QStringLiteral("name"), d.name);
    insertIfNotEmpty(json, QStringLiteral("description"), d.description);
    insertIfNotEmpty(json, QStringLiteral("model"), d.model);
    insertIfNotEmpty(json, QStringLiteral("instructions"), d.instructions);
    if (!d.tools.isEmpty())
        json.insert(QStringLiteral("tools"), d.tools);
    if (!d.toolResources.isEmpty())
        json.insert(QStringLiteral("tool_resources"), d.toolResources);
    if (!d.metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d.metadata);
    insertIfSet(json, QStringLiteral("temperature"), d.temperature);
    insertIfSet(json, QStringLiteral("top_p"), d.topP);
    if (!d.responseFormat.isUndefined())
        json.insert(QStringLiteral("response_format"), d.responseFormat);
}

inline void readAssistantFields(AssistantFieldsData &d, const QJsonObject &json)
{
    d.name = stringOr(json, QStringLiteral("name"));
    d.description = stringOr(json, QStringLiteral("description"));
    d.model = stringOr(json, QStringLiteral("model"));
    d.instructions = stringOr(json, QStringLiteral("instructions"));
    d.tools = json.value(QStringLiteral("tools")).toArray();
    d.toolResources = json.value(QStringLiteral("tool_resources")).toObject();
    d.metadata = json.value(QStringLiteral("metadata")).toObject();
    d.temperature = optionalDouble(json, QStringLiteral("temperature"));
    d.topP = optionalDouble(json, QStringLiteral("top_p"));
    // A null response_format means "not set", the same as an absent one.
    const QJsonValue format = json.value(QStringLiteral("response_format"));
    d.responseFormat = format.isNull() ? QJsonValue(QJsonValue::Undefined) : format;
}

inline bool assistantFieldsEqual(const AssistantFieldsData &a, const AssistantFieldsData &b)
{
    return a.name == b.name && a.description == b.description && a.model == b.model
           && a.instructions == b.instructions && a.tools == b.tools
           && a.toolResources == b.toolResources && a.metadata == b.metadata
           && a.temperature == b.temperature && a.topP == b.topP
           && a.responseFormat == b.responseFormat;
}

} // namespace detail
} // namespace Core
} // namespace QtOpenAi
