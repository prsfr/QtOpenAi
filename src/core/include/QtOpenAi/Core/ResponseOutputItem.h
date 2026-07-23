// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

class ResponseOutputItemData;

// A single item in a Response's `output` array.
//
// The Responses API returns a heterogeneous list of output items. This is a
// tagged value type: type() names the variant and the relevant accessors carry
// its payload. The variants modelled here are:
//   - "message":       role() + text() (assistant output_text content)
//   - "function_call": name(), arguments(), callId()
//   - "reasoning":     summary() (reasoning summary lines)
// Unknown types round-trip through raw(), which also preserves any fields not
// mapped to a typed accessor.
class QTOPENAI_CORE_EXPORT ResponseOutputItem
{
public:
    ResponseOutputItem();
    explicit ResponseOutputItem(const QString &type);
    ResponseOutputItem(const ResponseOutputItem &other);
    ResponseOutputItem(ResponseOutputItem &&other) noexcept;
    ResponseOutputItem &operator=(const ResponseOutputItem &other);
    ResponseOutputItem &operator=(ResponseOutputItem &&other) noexcept;
    ~ResponseOutputItem();

    void swap(ResponseOutputItem &other) noexcept { d.swap(other.d); }

    // The item variant, e.g. "message", "function_call", "reasoning".
    QString type() const;
    void setType(const QString &type);

    // Item id assigned by the server (e.g. "msg_..", "fc_..", "rs_..").
    QString id() const;
    void setId(const QString &id);

    // Per-item lifecycle status ("in_progress", "completed", ...); may be empty.
    QString status() const;
    void setStatus(const QString &status);

    // --- message -----------------------------------------------------------
    // Role of a "message" item (usually "assistant").
    QString role() const;
    void setRole(const QString &role);

    // Concatenated output_text of a "message" item. Setting it replaces the
    // message content with a single output_text part.
    QString text() const;
    void setText(const QString &text);

    // --- function_call -----------------------------------------------------
    QString name() const;
    void setName(const QString &name);

    // Raw JSON-encoded arguments string.
    QString arguments() const;
    void setArguments(const QString &arguments);

    // The call id used to correlate a later function_call_output.
    QString callId() const;
    void setCallId(const QString &callId);

    // --- reasoning ---------------------------------------------------------
    // Reasoning summary lines (summary_text entries).
    QStringList summary() const;
    void setSummary(const QStringList &summary);

    bool isMessage() const { return type() == QLatin1String("message"); }
    bool isFunctionCall() const { return type() == QLatin1String("function_call"); }
    bool isReasoning() const { return type() == QLatin1String("reasoning"); }

    // Convenience factories.
    static ResponseOutputItem message(const QString &text,
                                      const QString &role = QStringLiteral("assistant"));
    static ResponseOutputItem functionCall(const QString &name, const QString &arguments,
                                           const QString &callId);
    static ResponseOutputItem reasoning(const QStringList &summary);

    QJsonObject toJson() const;
    static ResponseOutputItem fromJson(const QJsonObject &json);

    bool operator==(const ResponseOutputItem &other) const;
    bool operator!=(const ResponseOutputItem &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ResponseOutputItemData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ResponseOutputItem)
