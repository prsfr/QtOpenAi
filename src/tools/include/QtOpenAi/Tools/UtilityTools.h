// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/MetaSchema.h>
#include <QtOpenAi/Tools/GlobalTools.h>

#include <QtCore/QObject>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Tools {

// The tools that cannot do any harm: what time it is, what a sum comes to, and
// a fresh identifier.
//
// Worth shipping precisely because they are dull. A model that cannot read a
// clock will confidently tell a user the wrong date, and one that does
// arithmetic by predicting the next token gets it wrong in ways that look
// right. These are also the only tools here that need no policy at all, which
// is why they are the ones DefaultTools installs when asked for the safe set.
//
// `calculate` evaluates the expression with a small parser written for the
// purpose. It is not an interpreter and there is no way to reach one from it:
// handing model-supplied text to a scripting engine would be the same mistake
// as `eval`, made on behalf of a user who never saw the string.
class QTOPENAI_TOOLS_EXPORT UtilityTools : public QObject
{
    Q_OBJECT
    QTOPENAI_DOC("Small utilities: the current time, arithmetic, and unique identifiers.")
public:
    explicit UtilityTools(QObject *parent = nullptr);
    ~UtilityTools() override;

    QTOPENAI_DOC_INVOKABLE(QString, current_time,
                           "The current date and time in ISO 8601 format, in UTC.");

    QTOPENAI_DOC_INVOKABLE(
            QString, calculate, "Evaluate an arithmetic expression and return the result.",
            const QString &, expression,
            "An arithmetic expression, e.g. '(17 + 4) * 3 / 7'. Supports "
            "+ - * / % ^, parentheses, and sqrt, abs, min, max, round, floor, ceil.");

    QTOPENAI_DOC_INVOKABLE(QString, uuid, "Generate a new random UUID.");

    static QStringList toolNames();
};

} // namespace Tools
} // namespace QtOpenAi
