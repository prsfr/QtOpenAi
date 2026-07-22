// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class FunctionDefinitionData;

// The definition of a callable function exposed to the model: name, an optional
// natural-language description, and a JSON-Schema parameter object.
class QTOPENAI_CORE_EXPORT FunctionDefinition
{
public:
    FunctionDefinition();
    FunctionDefinition(QString name, QString description, QJsonObject parameters);
    FunctionDefinition(const FunctionDefinition &other);
    FunctionDefinition(FunctionDefinition &&other) noexcept;
    FunctionDefinition &operator=(const FunctionDefinition &other);
    FunctionDefinition &operator=(FunctionDefinition &&other) noexcept;
    ~FunctionDefinition();

    void swap(FunctionDefinition &other) noexcept { d.swap(other.d); }

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    // JSON-Schema object describing the accepted arguments.
    QJsonObject parameters() const;
    void setParameters(const QJsonObject &parameters);

    bool isEmpty() const;

    QJsonObject toJson() const;
    static FunctionDefinition fromJson(const QJsonObject &json);

    bool operator==(const FunctionDefinition &other) const;
    bool operator!=(const FunctionDefinition &other) const { return !(*this == other); }

private:
    QSharedDataPointer<FunctionDefinitionData> d;
};

class ToolData;

// A tool the model may call. Currently only the "function" tool type exists.
class QTOPENAI_CORE_EXPORT Tool
{
public:
    Tool();
    explicit Tool(FunctionDefinition function);
    Tool(const Tool &other);
    Tool(Tool &&other) noexcept;
    Tool &operator=(const Tool &other);
    Tool &operator=(Tool &&other) noexcept;
    ~Tool();

    void swap(Tool &other) noexcept { d.swap(other.d); }

    QString type() const;
    void setType(const QString &type);

    FunctionDefinition function() const;
    void setFunction(const FunctionDefinition &function);

    bool isEmpty() const;

    QJsonObject toJson() const;
    static Tool fromJson(const QJsonObject &json);

    // Convenience factory for a function tool.
    static Tool function(const QString &name,
                         const QString &description,
                         const QJsonObject &parameters);

    bool operator==(const Tool &other) const;
    bool operator!=(const Tool &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ToolData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::FunctionDefinition)
Q_DECLARE_SHARED(QtOpenAi::Core::Tool)
