// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

// JSON-Schema derived from Qt's meta-object system.
//
// Everywhere this library takes a schema — a tool's `parameters`, a
// Structured Outputs `json_schema` — it has so far been hand-written, which
// means it can drift from the code it describes: rename a handler's argument
// and the schema still advertises the old name, with nothing to catch it. The
// meta-object system already knows the names and types, so the schema can be
// derived from them instead.
//
//     Q_INVOKABLE QString forecast(const QString &location, int days);
//     MetaSchema::fromMethod(receiver->metaObject(), "forecast");
//     // {"type":"object",
//     //  "properties":{"location":{"type":"string"},"days":{"type":"integer"}},
//     //  "required":["location","days"],"additionalProperties":false}
//
// The type mapping covers what a tool argument realistically is: strings,
// integers (kept distinct from floating-point numbers, as JSON-Schema does),
// booleans, string lists and other sequences, `QDateTime` as a date-time
// string, `Q_ENUM` as the closed set of its keys, and any `Q_GADGET` or
// `QObject` as a nested object built from its `Q_PROPERTY`s. A type outside
// that set yields an *empty* schema, which accepts anything — an honest "no
// constraint" rather than a guessed one.
//
// Every property and argument is listed in `required`, and objects are closed
// with `additionalProperties: false`. Both are what Structured Outputs demands
// in strict mode, and neither loses anything for a tool: a C++ signature has no
// absent parameters.
//
// Descriptions — the one thing the meta-object system does not carry — come
// from `Q_CLASSINFO`, keyed by the path to what they describe. Write them with
// the macros below rather than by hand:
//
//     QTOPENAI_DOC("Someone to greet")                        // the object
//     QTOPENAI_DOC_PROPERTY(age, "Whole years")               // a property
//     QTOPENAI_DOC_METHOD(forecast, "Tomorrow's weather")     // a method
//     QTOPENAI_DOC_ARGUMENT(forecast, location, "City name")  // its argument
//
// Those describe a member declared elsewhere. QTOPENAI_DOC_INVOKABLE declares
// it too, so no separate `Q_INVOKABLE` line is needed and no name is written
// twice:
//
//     QTOPENAI_DOC_INVOKABLE(QString, forecast, "Tomorrow's weather",
//                            const QString &, location, "City name");
namespace MetaSchema {

// The schema for a Q_GADGET/QObject, built from its Q_PROPERTYs.
QTOPENAI_CORE_EXPORT QJsonObject fromMetaObject(const QMetaObject *meta);

// The same, for a type known at compile time.
template <typename T>
QJsonObject fromType()
{
    return fromMetaObject(&T::staticMetaObject);
}

// The schema for one invokable method's arguments, addressed by name. Returns
// an empty object when the method does not exist, so a caller that mistypes a
// name gets nothing rather than a wrong description.
QTOPENAI_CORE_EXPORT QJsonObject fromMethod(const QMetaObject *meta, const QString &method);

// The schema fragment for a single type; empty when the type is not one this
// mapping describes.
QTOPENAI_CORE_EXPORT QJsonObject fromMetaType(QMetaType type);

// The `doc` annotations on this class that describe nothing: a key naming a
// property, method or argument the class does not have. Such a key is always a
// mistake -- a typo, or a rename that left its annotation behind -- and it is
// silent, because a description that matches nothing simply never appears in
// the schema. Assert on this in a test and a renamed argument stops being able
// to quietly lose its documentation.
//
// Only annotations MetaSchema itself would read are considered, so a `doc:` key
// for an inherited property (which fromMetaObject does not emit) is reported.
QTOPENAI_CORE_EXPORT QStringList danglingAnnotations(const QMetaObject *meta);

template <typename T>
QStringList danglingAnnotations()
{
    return danglingAnnotations(&T::staticMetaObject);
}

} // namespace MetaSchema

} // namespace Core
} // namespace QtOpenAi

// --- Describing a class to the model ---------------------------------------
//
// Q_CLASSINFO takes a key and a value, and the key is a path this library
// assembles conventions into: a `doc` prefix, then the member, then the
// argument. Written out by hand that is three chances to be wrong -- a missing
// prefix, a stray colon, a misspelt name -- and every one of them fails
// silently, because an annotation that matches nothing is simply never read.
//
// These macros build the key from identifiers instead. They expand to exactly
// the Q_CLASSINFO you would have written, and moc concatenates the literals
// before it ever sees a key, so nothing here costs anything at runtime. The
// same trick Qt uses for QML_NAMED_ELEMENT.
//
// A typo is still a typo -- `QTOPENAI_DOC_PROPERTY(agee, ...)` compiles -- but
// it is now a *lone* mistake rather than one hidden among the punctuation, and
// MetaSchema::danglingAnnotations() finds it.

// The class itself: what this object or tool is.
#define QTOPENAI_DOC(description) Q_CLASSINFO("doc", description)

// One Q_PROPERTY, by name.
#define QTOPENAI_DOC_PROPERTY(property, description) Q_CLASSINFO("doc:" #property, description)

// One Q_INVOKABLE method and, optionally, its arguments -- the method named
// once, each argument a `name, "description"` pair after it:
//
//     QTOPENAI_DOC_METHOD(write_file, "Write UTF-8 text to a file.",
//                         path,    "Path to the file to write.",
//                         content, "The text to write.")
//
// One invocation, three Q_CLASSINFO. Naming the method once is the whole point:
// spelled per argument it was written once per parameter plus once for the
// method itself, and every one of those repetitions was a place for the two to
// drift apart while still compiling.
//
// A method with no arguments is the same macro with nothing after the
// description, so there is one macro to know rather than two.
//
// Up to eight arguments; past that, or to describe an argument away from its
// method, QTOPENAI_DOC_ARGUMENT is still there.
#define QTOPENAI_DOC_METHOD(method, ...)                                                           \
    QTOPENAI_DOC_EXPAND(QTOPENAI_DOC_CAT(QTOPENAI_DOC_M_,                                          \
                                         QTOPENAI_DOC_NARG(__VA_ARGS__))(method, __VA_ARGS__))

// One argument of one method, on its own. `argument` is the parameter name as
// the signature spells it -- the name moc recorded, which is the same name the
// generated schema advertises.
#define QTOPENAI_DOC_ARGUMENT(method, argument, description)                                       \
    Q_CLASSINFO("doc:" #method ":" #argument, description)

// --- The plumbing behind QTOPENAI_DOC_METHOD -------------------------------
//
// Preprocessor, and it has to be. Q_CLASSINFO's key must be a string literal in
// the source text, because moc reads the tokens rather than compiling them --
// so no constexpr function, no consteval, nothing from C++17 or later can take
// part in building one. Assembling the key is the preprocessor's job or it is
// the caller's, and the whole point of these macros is that it is not the
// caller's.
//
// The argument count is the *variadic* count, description included, so it is
// never zero -- which is what makes the dispatch portable. Counting a possibly
// empty __VA_ARGS__ needs __VA_OPT__ (C++20) or a compiler extension, and this
// library is C++17 and has to pass through moc's own preprocessor besides.
//
// Verified against moc rather than assumed: its preprocessor accepts token
// pasting and a macro expanding to several Q_CLASSINFO, but *not* the usual
// trick of unparenthesising a parameter, which is why the arguments are flat
// pairs rather than `(name, "description")` tuples.
#define QTOPENAI_DOC_EXPAND(x) x
#define QTOPENAI_DOC_CAT_(a, b) a##b
#define QTOPENAI_DOC_CAT(a, b) QTOPENAI_DOC_CAT_(a, b)
// Counts to 19, which is QTOPENAI_DOC_INVOKABLE with six parameters (1 + 3*6); the
// eight-argument ceiling of QTOPENAI_DOC_METHOD sits at 17. Shared, so there is
// one counter to reason about rather than two that could disagree.
#define QTOPENAI_DOC_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16,  \
                           _17, _18, _19, N, ...)                                                  \
    N
#define QTOPENAI_DOC_NARG(...)                                                                     \
    QTOPENAI_DOC_EXPAND(QTOPENAI_DOC_COUNT(__VA_ARGS__, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, \
                                           8, 7, 6, 5, 4, 3, 2, 1))

#define QTOPENAI_DOC_M_1(m, d) Q_CLASSINFO("doc:" #m, d)
#define QTOPENAI_DOC_M_3(m, d, n1, d1) QTOPENAI_DOC_M_1(m, d) Q_CLASSINFO("doc:" #m ":" #n1, d1)
#define QTOPENAI_DOC_M_5(m, d, n1, d1, n2, d2)                                                     \
    QTOPENAI_DOC_EXPAND(QTOPENAI_DOC_M_3(m, d, n1, d1)) Q_CLASSINFO("doc:" #m ":" #n2, d2)
#define QTOPENAI_DOC_M_7(m, d, n1, d1, n2, d2, n3, d3)                                             \
    QTOPENAI_DOC_EXPAND(QTOPENAI_DOC_M_5(m, d, n1, d1, n2, d2)) Q_CLASSINFO("doc:" #m ":" #n3, d3)
#define QTOPENAI_DOC_M_9(m, d, n1, d1, n2, d2, n3, d3, n4, d4)                                     \
    QTOPENAI_DOC_EXPAND(QTOPENAI_DOC_M_7(m, d, n1, d1, n2, d2, n3, d3))                            \
    Q_CLASSINFO("doc:" #m ":" #n4, d4)
#define QTOPENAI_DOC_M_11(m, d, n1, d1, n2, d2, n3, d3, n4, d4, n5, d5)                            \
    QTOPENAI_DOC_EXPAND(QTOPENAI_DOC_M_9(m, d, n1, d1, n2, d2, n3, d3, n4, d4))                    \
    Q_CLASSINFO("doc:" #m ":" #n5, d5)
#define QTOPENAI_DOC_M_13(m, d, n1, d1, n2, d2, n3, d3, n4, d4, n5, d5, n6, d6)                    \
    QTOPENAI_DOC_EXPAND(QTOPENAI_DOC_M_11(m, d, n1, d1, n2, d2, n3, d3, n4, d4, n5, d5))           \
    Q_CLASSINFO("doc:" #m ":" #n6, d6)
#define QTOPENAI_DOC_M_15(m, d, n1, d1, n2, d2, n3, d3, n4, d4, n5, d5, n6, d6, n7, d7)            \
    QTOPENAI_DOC_EXPAND(QTOPENAI_DOC_M_13(m, d, n1, d1, n2, d2, n3, d3, n4, d4, n5, d5, n6, d6))   \
    Q_CLASSINFO("doc:" #m ":" #n7, d7)
#define QTOPENAI_DOC_M_17(m, d, n1, d1, n2, d2, n3, d3, n4, d4, n5, d5, n6, d6, n7, d7, n8, d8)    \
    QTOPENAI_DOC_EXPAND(                                                                           \
            QTOPENAI_DOC_M_15(m, d, n1, d1, n2, d2, n3, d3, n4, d4, n5, d5, n6, d6, n7, d7))       \
    Q_CLASSINFO("doc:" #m ":" #n8, d8)

// --- Declaring an invokable and describing it in one place -----------------
//
// QTOPENAI_DOC_METHOD describes a method declared on the next line, which
// leaves the method name and every argument name written twice -- once in the
// description, once in the signature -- with nothing checking that the two
// agree until danglingAnnotations() is run. This macro is that one and the
// `Q_INVOKABLE` declaration together, which is what the two halves of its name
// mean. Each name is then written exactly once:
//
//     QTOPENAI_DOC_INVOKABLE(QString, write_file, "Write UTF-8 text to a file.",
//                            const QString &, path,    "Path to the file to write.",
//                            const QString &, content, "The text to write.");
//
// expands to the three Q_CLASSINFO *and* `Q_INVOKABLE QString write_file(const
// QString &path, const QString &content);`. There is no separate Q_INVOKABLE
// line to keep in step, and a renamed argument renames its description with it
// because they are the same token. The type, the name and the meaning of each
// parameter sit together, which is the order one thinks about them in.
//
// The trade is that the signature is now inside a macro: go-to-definition, a
// grep for the return type, and anything else that reads C++ by pattern have
// one more layer to see through. That cost is real, but it is paid once per
// method, whereas the duplicate names it removes were a standing invitation to
// drift.
//
// Two limits, both reported as a sentence rather than as a puzzle:
//   * A parameter type containing a comma (QMap<QString, int>) needs a typedef,
//     because the preprocessor splits on it.
//   * Up to 6 parameters. A tool with more has a different problem.
//
// The expansion ends at the closing parenthesis of the signature, so what
// follows decides which it is: a `;` makes it a declaration and the body goes
// out of line as usual, or a `{ ... }` follows directly and defines it inline.
// moc reads both -- checked by running it and looking at what it recorded.
#define QTOPENAI_DOC_INVOKABLE(returnType, method, ...)                                            \
    QTOPENAI_DOC_EXPAND(QTOPENAI_DOC_CAT(QTOPENAI_DOC_I_, QTOPENAI_DOC_NARG(__VA_ARGS__))(         \
            returnType, method, __VA_ARGS__))

// Reached only when the item count is not 1 + 3n, which means a triple is
// short -- or a type had a comma in it and split into two items.
#define QTOPENAI_DOC_I_BAD                                                                         \
    static_assert(false,                                                                           \
                  "QTOPENAI_DOC_INVOKABLE: every parameter needs three items -- type, name, "      \
                  "description. A parameter type containing a comma, such as "                     \
                  "QMap<QString, int>, needs a typedef first.")
#define QTOPENAI_DOC_I_2(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_3(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_5(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_6(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_8(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_9(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_11(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_12(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_14(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_15(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_17(...) QTOPENAI_DOC_I_BAD;
#define QTOPENAI_DOC_I_18(...) QTOPENAI_DOC_I_BAD;

#define QTOPENAI_DOC_I_1(r, m, d)                                                                  \
    Q_CLASSINFO("doc:" #m, d)                                                                      \
    Q_INVOKABLE r m()
#define QTOPENAI_DOC_I_4(r, m, d, t1, n1, d1)                                                      \
    Q_CLASSINFO("doc:" #m, d)                                                                      \
    Q_CLASSINFO("doc:" #m ":" #n1, d1)                                                             \
    Q_INVOKABLE r m(t1 n1)
#define QTOPENAI_DOC_I_7(r, m, d, t1, n1, d1, t2, n2, d2)                                          \
    Q_CLASSINFO("doc:" #m, d)                                                                      \
    Q_CLASSINFO("doc:" #m ":" #n1, d1)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n2, d2)                                                             \
    Q_INVOKABLE r m(t1 n1, t2 n2)
#define QTOPENAI_DOC_I_10(r, m, d, t1, n1, d1, t2, n2, d2, t3, n3, d3)                             \
    Q_CLASSINFO("doc:" #m, d)                                                                      \
    Q_CLASSINFO("doc:" #m ":" #n1, d1)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n2, d2)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n3, d3)                                                             \
    Q_INVOKABLE r m(t1 n1, t2 n2, t3 n3)
#define QTOPENAI_DOC_I_13(r, m, d, t1, n1, d1, t2, n2, d2, t3, n3, d3, t4, n4, d4)                 \
    Q_CLASSINFO("doc:" #m, d)                                                                      \
    Q_CLASSINFO("doc:" #m ":" #n1, d1)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n2, d2)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n3, d3)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n4, d4)                                                             \
    Q_INVOKABLE r m(t1 n1, t2 n2, t3 n3, t4 n4)
#define QTOPENAI_DOC_I_16(r, m, d, t1, n1, d1, t2, n2, d2, t3, n3, d3, t4, n4, d4, t5, n5, d5)     \
    Q_CLASSINFO("doc:" #m, d)                                                                      \
    Q_CLASSINFO("doc:" #m ":" #n1, d1)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n2, d2)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n3, d3)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n4, d4)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n5, d5)                                                             \
    Q_INVOKABLE r m(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5)
#define QTOPENAI_DOC_I_19(r, m, d, t1, n1, d1, t2, n2, d2, t3, n3, d3, t4, n4, d4, t5, n5, d5, t6, \
                          n6, d6)                                                                  \
    Q_CLASSINFO("doc:" #m, d)                                                                      \
    Q_CLASSINFO("doc:" #m ":" #n1, d1)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n2, d2)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n3, d3)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n4, d4)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n5, d5)                                                             \
    Q_CLASSINFO("doc:" #m ":" #n6, d6)                                                             \
    Q_INVOKABLE r m(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6)
