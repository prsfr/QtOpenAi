// SPDX-License-Identifier: MIT
#pragma once

// One enum value's wire spelling, and the two conversions derived from it.
//
// The rule this exists to enforce: an enum that travels on the wire is described
// by a *single* table, so the mapping and its inverse cannot drift apart -- which
// they could, and did, when both directions were hand-written as a switch and an
// if-chain one screen apart. Enums.cpp introduced the shape for the thirteen
// enums it owns; it lives here so an enum declared elsewhere -- VectorIndex's
// Metric is the one -- can be held to the same rule instead of hand-rolling the
// pair again.
//
// Not installed and not part of the public API. Nothing here is exported: the
// templates are header-only and the macro defines functions in the translation
// unit that uses it.

#include <QtCore/QLatin1String>
#include <QtCore/QString>

#include <cstddef>

namespace QtOpenAi {
namespace Core {
namespace detail {

// `terminal` answers the third thing a status is asked: whether reaching it ends
// the lifecycle the value belongs to. The enums that describe no lifecycle
// (Role, FinishReason, Metric) leave the column at false and are never asked.
template <typename Enum>
struct WireName
{
    Enum value;
    const char *name;
    bool terminal = false;
};

// An enum value the table does not cover encodes as `fallback`'s spelling, and
// an unrecognised wire string decodes to `fallback`. A table is proven complete
// by tst_core_enums, which round-trips every value the meta-object system
// reports -- a stronger check than a switch's exhaustiveness warning, because it
// also catches a value that has a row but the wrong spelling.
template <typename Enum, size_t N>
QString toWire(Enum value, const WireName<Enum> (&table)[N], Enum fallback)
{
    for (const auto &row : table) {
        if (row.value == value)
            return QString::fromLatin1(row.name);
    }
    return toWire(fallback, table, fallback);
}

template <typename Enum, size_t N>
Enum fromWire(const QString &value, const WireName<Enum> (&table)[N], Enum fallback)
{
    for (const auto &row : table) {
        if (value == QLatin1String(row.name))
            return row.value;
    }
    return fallback;
}

// A value the table does not cover is treated as non-terminal, matching the
// decode fallbacks: an unfamiliar status from a newer server leaves a poller
// waiting rather than stopping it early.
template <typename Enum, size_t N>
bool terminalIn(Enum value, const WireName<Enum> (&table)[N])
{
    for (const auto &row : table) {
        if (row.value == value)
            return row.terminal;
    }
    return false;
}

} // namespace detail
} // namespace Core
} // namespace QtOpenAi

// Define one enum's pair of conversions. Both directions name the same table and
// the same fallback exactly once, so a fallback cannot be changed on the decode
// side and forgotten on the encode side -- the drift the tables themselves were
// introduced to remove, one level up.
#define QTOPENAI_WIRE_CONVERSIONS(ToName, FromName, Enum, table, fallback)                         \
    QString ToName(Enum value) { return QtOpenAi::Core::detail::toWire(value, table, fallback); }  \
    Enum FromName(const QString &value)                                                            \
    {                                                                                              \
        return QtOpenAi::Core::detail::fromWire(value, table, fallback);                           \
    }
