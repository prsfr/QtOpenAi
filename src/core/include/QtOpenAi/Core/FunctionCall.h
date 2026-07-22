// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class FunctionCallData;

// The function a model asked to invoke: a name plus a raw JSON argument string.
//
// Implicitly shared value type (copy-on-write). The private implementation is
// hidden behind a QSharedDataPointer d-pointer, following Qt conventions.
class QTOPENAI_CORE_EXPORT FunctionCall
{
public:
    FunctionCall();
    FunctionCall(QString name, QString arguments);
    FunctionCall(const FunctionCall &other);
    FunctionCall(FunctionCall &&other) noexcept;
    FunctionCall &operator=(const FunctionCall &other);
    FunctionCall &operator=(FunctionCall &&other) noexcept;
    ~FunctionCall();

    void swap(FunctionCall &other) noexcept { d.swap(other.d); }

    QString name() const;
    void setName(const QString &name);

    // Raw, model-generated JSON argument string (not guaranteed to be valid).
    QString arguments() const;
    void setArguments(const QString &arguments);

    // Convenience: parse arguments() into a QJsonObject. Returns an empty
    // object when the arguments are absent or not a valid JSON object.
    QJsonObject argumentsObject() const;

    bool isEmpty() const;

    QJsonObject toJson() const;
    static FunctionCall fromJson(const QJsonObject &json);

    bool operator==(const FunctionCall &other) const;
    bool operator!=(const FunctionCall &other) const { return !(*this == other); }

private:
    QSharedDataPointer<FunctionCallData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::FunctionCall)
