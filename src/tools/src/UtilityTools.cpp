// SPDX-License-Identifier: MIT
#include "QtOpenAi/Tools/UtilityTools.h"

#include <QtCore/QDateTime>
#include <QtCore/QUuid>

#include <cmath>

namespace QtOpenAi {
namespace Tools {

namespace {

// A recursive-descent parser for arithmetic, and nothing else.
//
// The alternative -- handing the model's string to a scripting engine -- would
// be `eval` on text an attacker may have written into the context, on behalf of
// a user who never saw it. This grammar has no names to resolve, no properties
// to reach through and no way to call anything that is not in the switch below,
// so there is nothing to escape into.
class Expression
{
public:
    explicit Expression(const QString &text)
        : m_text(text)
    { }

    bool evaluate(double *result, QString *error)
    {
        skipSpace();
        const double value = parseSum();
        if (m_failed) {
            *error = m_error;
            return false;
        }
        skipSpace();
        if (m_position < m_text.size()) {
            *error = QStringLiteral("unexpected '%1'").arg(m_text.at(m_position));
            return false;
        }
        if (!std::isfinite(value)) {
            *error = QStringLiteral("the result is not a finite number");
            return false;
        }
        *result = value;
        return true;
    }

private:
    void skipSpace()
    {
        while (m_position < m_text.size() && m_text.at(m_position).isSpace())
            ++m_position;
    }

    bool take(QChar expected)
    {
        skipSpace();
        if (m_position < m_text.size() && m_text.at(m_position) == expected) {
            ++m_position;
            return true;
        }
        return false;
    }

    QChar peek() const { return m_position < m_text.size() ? m_text.at(m_position) : QChar(); }

    double fail(const QString &message)
    {
        if (!m_failed) {
            m_failed = true;
            m_error = message;
        }
        return 0.0;
    }

    double parseSum()
    {
        double value = parseProduct();
        while (!m_failed) {
            skipSpace();
            if (take(QLatin1Char('+')))
                value += parseProduct();
            else if (take(QLatin1Char('-')))
                value -= parseProduct();
            else
                break;
        }
        return value;
    }

    double parseProduct()
    {
        double value = parseUnary();
        while (!m_failed) {
            skipSpace();
            if (take(QLatin1Char('*'))) {
                value *= parseUnary();
            } else if (take(QLatin1Char('/'))) {
                const double divisor = parseUnary();
                if (divisor == 0.0)
                    return fail(QStringLiteral("division by zero"));
                value /= divisor;
            } else if (take(QLatin1Char('%'))) {
                const double divisor = parseUnary();
                if (divisor == 0.0)
                    return fail(QStringLiteral("division by zero"));
                value = std::fmod(value, divisor);
            } else {
                break;
            }
        }
        return value;
    }

    // Unary minus binds *looser* than '^', so -2^2 is -(2^2) = -4, as it is in
    // mathematics and in Python. Getting this backwards is the kind of bug
    // nobody notices until an answer is off by a sign.
    double parseUnary()
    {
        skipSpace();
        if (take(QLatin1Char('-')))
            return -parseUnary();
        if (take(QLatin1Char('+')))
            return parseUnary();
        return parsePower();
    }

    // Right-associative, as everyone writes it: 2^3^2 is 512, not 64. The
    // exponent goes back through parseUnary so 2^-3 is a number rather than a
    // syntax error.
    double parsePower()
    {
        const double base = parseAtom();
        skipSpace();
        if (!m_failed && take(QLatin1Char('^')))
            return std::pow(base, parseUnary());
        return base;
    }

    double parseAtom()
    {
        skipSpace();
        if (m_position >= m_text.size())
            return fail(QStringLiteral("the expression ends too early"));

        if (take(QLatin1Char('('))) {
            const double value = parseSum();
            if (!take(QLatin1Char(')')))
                return fail(QStringLiteral("a '(' is not closed"));
            return value;
        }

        if (peek().isLetter())
            return parseCall();

        return parseNumber();
    }

    double parseNumber()
    {
        const int start = m_position;
        while (m_position < m_text.size()
               && (m_text.at(m_position).isDigit() || m_text.at(m_position) == QLatin1Char('.')))
            ++m_position;
        // Exponent notation, so 1e6 is a number rather than a number followed
        // by a name.
        if (m_position < m_text.size()
            && (m_text.at(m_position) == QLatin1Char('e')
                || m_text.at(m_position) == QLatin1Char('E'))) {
            const int mark = m_position++;
            if (m_position < m_text.size()
                && (m_text.at(m_position) == QLatin1Char('+')
                    || m_text.at(m_position) == QLatin1Char('-')))
                ++m_position;
            if (m_position < m_text.size() && m_text.at(m_position).isDigit()) {
                while (m_position < m_text.size() && m_text.at(m_position).isDigit())
                    ++m_position;
            } else {
                m_position = mark;
            }
        }

        if (m_position == start)
            return fail(QStringLiteral("expected a number"));

        bool ok = false;
        const double value = m_text.mid(start, m_position - start).toDouble(&ok);
        return ok ? value : fail(QStringLiteral("that is not a number"));
    }

    double parseCall()
    {
        const int start = m_position;
        while (m_position < m_text.size() && m_text.at(m_position).isLetter())
            ++m_position;
        const QString name = m_text.mid(start, m_position - start).toLower();

        // Constants first: they take no arguments and are the only names that
        // are not calls.
        if (name == QLatin1String("pi"))
            return M_PI;
        if (name == QLatin1String("e"))
            return M_E;

        if (!take(QLatin1Char('(')))
            return fail(QStringLiteral("unknown name '%1'").arg(name));

        QList<double> arguments;
        if (!take(QLatin1Char(')'))) {
            do {
                arguments.append(parseSum());
                if (m_failed)
                    return 0.0;
            } while (take(QLatin1Char(',')));
            if (!take(QLatin1Char(')')))
                return fail(QStringLiteral("a '(' is not closed"));
        }

        const auto arity = [&](int expected) {
            if (arguments.size() == expected)
                return true;
            fail(QStringLiteral("%1 takes %2 argument(s)").arg(name).arg(expected));
            return false;
        };

        if (name == QLatin1String("sqrt")) {
            if (!arity(1))
                return 0.0;
            if (arguments.at(0) < 0.0)
                return fail(QStringLiteral("the square root of a negative number is not real"));
            return std::sqrt(arguments.at(0));
        }
        if (name == QLatin1String("abs"))
            return arity(1) ? std::fabs(arguments.at(0)) : 0.0;
        if (name == QLatin1String("round"))
            return arity(1) ? std::round(arguments.at(0)) : 0.0;
        if (name == QLatin1String("floor"))
            return arity(1) ? std::floor(arguments.at(0)) : 0.0;
        if (name == QLatin1String("ceil"))
            return arity(1) ? std::ceil(arguments.at(0)) : 0.0;
        if (name == QLatin1String("min"))
            return arity(2) ? qMin(arguments.at(0), arguments.at(1)) : 0.0;
        if (name == QLatin1String("max"))
            return arity(2) ? qMax(arguments.at(0), arguments.at(1)) : 0.0;

        return fail(QStringLiteral("unknown function '%1'").arg(name));
    }

    QString m_text;
    QString m_error;
    int m_position = 0;
    bool m_failed = false;
};

} // namespace

UtilityTools::UtilityTools(QObject *parent)
    : QObject(parent)
{ }

UtilityTools::~UtilityTools() = default;

QStringList UtilityTools::toolNames()
{
    return {QStringLiteral("current_time"), QStringLiteral("calculate"), QStringLiteral("uuid")};
}

QString UtilityTools::current_time()
{
    // UTC and ISO 8601: a local time without an offset is a time nobody can
    // convert, and the model has no way to ask which zone it was.
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QString UtilityTools::calculate(const QString &expression)
{
    if (expression.trimmed().isEmpty())
        return QStringLiteral("there is no expression to evaluate");

    double result = 0.0;
    QString error;
    if (!Expression(expression).evaluate(&result, &error))
        return QStringLiteral("cannot evaluate that: %1").arg(error);

    // 15 significant digits: everything a double actually knows, and nothing it
    // does not. %f would turn 1e20 into a wall of zeroes it cannot vouch for.
    QString text = QString::number(result, 'g', 15);
    if (text == QLatin1String("-0"))
        text = QStringLiteral("0");
    return text;
}

QString UtilityTools::uuid() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

} // namespace Tools
} // namespace QtOpenAi
