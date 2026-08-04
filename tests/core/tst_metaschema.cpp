// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/MetaSchema.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

namespace {

// A gadget with one property per type family the mapping has to cover.
class Address
{
    Q_GADGET
    Q_PROPERTY(QString street MEMBER street)
    Q_PROPERTY(int number MEMBER number)
public:
    QString street;
    int number = 0;

    // A MEMBER property writes through a comparison, so the gadget needs one.
    bool operator==(const Address &other) const
    {
        return street == other.street && number == other.number;
    }
    bool operator!=(const Address &other) const { return !(*this == other); }
};

class Person
{
    Q_GADGET
    // The class-level description, and one per property. The macros assemble
    // the Q_CLASSINFO key, so the path convention is never spelled out here.
    QTOPENAI_DOC("Someone to greet")
    QTOPENAI_DOC_PROPERTY(age, "Whole years")
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int age MEMBER age)
    Q_PROPERTY(double height MEMBER height)
    Q_PROPERTY(bool active MEMBER active)
    Q_PROPERTY(QStringList tags MEMBER tags)
    Q_PROPERTY(Mood mood MEMBER mood)
    Q_PROPERTY(Address address MEMBER address)
public:
    enum class Mood {
        Happy,
        Sad,
    };
    Q_ENUM(Mood)

    QString name;
    int age = 0;
    double height = 0;
    bool active = false;
    QStringList tags;
    Mood mood = Mood::Happy;
    Address address;
};

// A receiver whose invokable methods are the thing being described.
class Weather : public QObject
{
    Q_OBJECT
    QTOPENAI_DOC_ARGUMENT(forecast, location, "City name")
public:
    Q_INVOKABLE QString forecast(const QString &location, int days)
    {
        return QStringLiteral("%1/%2").arg(location).arg(days);
    }
    Q_INVOKABLE QString noArguments() { return {}; }
};

// The grouped macro: each method named once, its arguments listed after it.
class Grouped : public QObject
{
    Q_OBJECT
    QTOPENAI_DOC("Everything described in one invocation per method")
    QTOPENAI_DOC_METHOD(nothing, "No arguments at all.")
    QTOPENAI_DOC_METHOD(one, "One argument.", only, "The only one.")
    QTOPENAI_DOC_METHOD(two, "Two arguments.", first, "The first.", second, "The second.")
    // The supported ceiling, so a change to it fails here rather than in a
    // caller's header.
    QTOPENAI_DOC_METHOD(eight, "Eight arguments.", a, "A.", b, "B.", c, "C.", d, "D.", e, "E.", f,
                        "F.", g, "G.", h, "H.")
public:
    Q_INVOKABLE void nothing() { }
    Q_INVOKABLE void one(const QString &only) {Q_UNUSED(only)} Q_INVOKABLE
            void two(const QString &first,
                     int second) {Q_UNUSED(first) Q_UNUSED(second)} Q_INVOKABLE
            void eight(int a, int b, int c, int d, int e, int f, int g, int h)
    {
        Q_UNUSED(a)
        Q_UNUSED(b) Q_UNUSED(c) Q_UNUSED(d) Q_UNUSED(e) Q_UNUSED(f) Q_UNUSED(g) Q_UNUSED(h)
    }
};

// The same descriptions as Grouped, written next to the methods they describe
// rather than in a block at the top -- the placement Cutelyst's C_ATTR uses.
// Q_CLASSINFO is legal anywhere in a class body, so this has to produce exactly
// the same meta-object; if it ever stops doing so, the convention has become a
// requirement and callers need to know.
class Adjacent : public QObject
{
    Q_OBJECT
    QTOPENAI_DOC("Everything described in one invocation per method")
public:
    QTOPENAI_DOC_METHOD(nothing, "No arguments at all.")
    Q_INVOKABLE void nothing() { }

    QTOPENAI_DOC_METHOD(one, "One argument.", only, "The only one.")
    Q_INVOKABLE void one(const QString &only) { Q_UNUSED(only) }

private:
    int unrelated = 0; // an access change between annotations must not matter

public:
    QTOPENAI_DOC_METHOD(two, "Two arguments.", first, "The first.", second, "The second.")
    Q_INVOKABLE void two(const QString &first, int second) { Q_UNUSED(first) Q_UNUSED(second) }
};

// Every way a `doc` key can name something that is not there.
class Misannotated : public QObject
{
    Q_OBJECT
    QTOPENAI_DOC("fine -- the class always exists")
    QTOPENAI_DOC_PROPERTY(title, "fine")
    QTOPENAI_DOC_METHOD(rename, "fine")
    QTOPENAI_DOC_ARGUMENT(rename, title, "fine")
    QTOPENAI_DOC_PROPERTY(subtitle, "no such property")
    QTOPENAI_DOC_METHOD(remove, "no such method")
    QTOPENAI_DOC_ARGUMENT(rename, name, "no such argument")
    QTOPENAI_DOC_ARGUMENT(remove, title, "no such method either")
    // objectName is QObject's, so fromMetaObject never emits it.
    QTOPENAI_DOC_PROPERTY(objectName, "inherited, so never described")
    Q_PROPERTY(QString title MEMBER title)
public:
    using QObject::QObject;
    Q_INVOKABLE void rename(const QString &title) { this->title = title; }

    QString title;
};

QJsonObject propertyOf(const QJsonObject &schema, const QString &name)
{
    return schema.value(QStringLiteral("properties")).toObject().value(name).toObject();
}

QStringList requiredOf(const QJsonObject &schema)
{
    QStringList names;
    const QJsonArray required = schema.value(QStringLiteral("required")).toArray();
    for (const QJsonValue &value : required)
        names.append(value.toString());
    return names;
}

} // namespace

// Coverage for the meta-object → JSON-Schema mapping (#39). The point of the
// feature is that a hand-written schema cannot drift from the code it
// describes, so these tests pin the mapping per type family.
class TestMetaSchema : public QObject
{
    Q_OBJECT
private slots:
    void mapsScalarProperties();
    void mapsArrayProperty();
    void mapsEnumToItsKeys();
    void mapsNestedGadget();
    void marksEveryPropertyRequiredAndClosesTheObject();
    void readsDescriptionsFromClassInfo();
    void describesMethodArguments();
    void describesAMethodWithoutArguments();
    void unknownTypesStayUnconstrained();
    void theMacrosBuildTheSameKeysAsHandWrittenOnes();
    void aMethodAndItsArgumentsAreOneInvocation();
    void anAnnotationMaySitNextToWhatItDescribes();
    void theShippedAnnotationsAllDescribeSomething();
    void findsAnnotationsThatDescribeNothing();
};

void TestMetaSchema::mapsScalarProperties()
{
    const QJsonObject schema = MetaSchema::fromType<Person>();

    QCOMPARE(schema.value(QStringLiteral("type")).toString(), QStringLiteral("object"));
    QCOMPARE(propertyOf(schema, QStringLiteral("name")).value(QStringLiteral("type")).toString(),
             QStringLiteral("string"));
    // JSON-Schema distinguishes integers from numbers, and so does C++.
    QCOMPARE(propertyOf(schema, QStringLiteral("age")).value(QStringLiteral("type")).toString(),
             QStringLiteral("integer"));
    QCOMPARE(propertyOf(schema, QStringLiteral("height")).value(QStringLiteral("type")).toString(),
             QStringLiteral("number"));
    QCOMPARE(propertyOf(schema, QStringLiteral("active")).value(QStringLiteral("type")).toString(),
             QStringLiteral("boolean"));
}

void TestMetaSchema::mapsArrayProperty()
{
    const QJsonObject tags = propertyOf(MetaSchema::fromType<Person>(), QStringLiteral("tags"));

    QCOMPARE(tags.value(QStringLiteral("type")).toString(), QStringLiteral("array"));
    QCOMPARE(
            tags.value(QStringLiteral("items")).toObject().value(QStringLiteral("type")).toString(),
            QStringLiteral("string"));
}

void TestMetaSchema::mapsEnumToItsKeys()
{
    // An enum is a closed set of names, which is exactly what a string enum
    // constraint says -- so the model is told the allowed values rather than a
    // bare integer.
    const QJsonObject mood = propertyOf(MetaSchema::fromType<Person>(), QStringLiteral("mood"));

    QCOMPARE(mood.value(QStringLiteral("type")).toString(), QStringLiteral("string"));
    const QJsonArray values = mood.value(QStringLiteral("enum")).toArray();
    QCOMPARE(values.size(), 2);
    QCOMPARE(values.at(0).toString(), QStringLiteral("Happy"));
    QCOMPARE(values.at(1).toString(), QStringLiteral("Sad"));
}

void TestMetaSchema::mapsNestedGadget()
{
    const QJsonObject address
            = propertyOf(MetaSchema::fromType<Person>(), QStringLiteral("address"));

    QCOMPARE(address.value(QStringLiteral("type")).toString(), QStringLiteral("object"));
    QCOMPARE(propertyOf(address, QStringLiteral("street")).value(QStringLiteral("type")).toString(),
             QStringLiteral("string"));
    QCOMPARE(propertyOf(address, QStringLiteral("number")).value(QStringLiteral("type")).toString(),
             QStringLiteral("integer"));
}

void TestMetaSchema::marksEveryPropertyRequiredAndClosesTheObject()
{
    // Structured Outputs in strict mode demands both, and a tool schema loses
    // nothing by them: C++ has no absent members.
    const QJsonObject schema = MetaSchema::fromType<Address>();

    QCOMPARE(requiredOf(schema), QStringList({QStringLiteral("street"), QStringLiteral("number")}));
    QCOMPARE(schema.value(QStringLiteral("additionalProperties")).toBool(true), false);
}

void TestMetaSchema::readsDescriptionsFromClassInfo()
{
    const QJsonObject schema = MetaSchema::fromType<Person>();

    QCOMPARE(schema.value(QStringLiteral("description")).toString(),
             QStringLiteral("Someone to greet"));
    QCOMPARE(propertyOf(schema, QStringLiteral("age"))
                     .value(QStringLiteral("description"))
                     .toString(),
             QStringLiteral("Whole years"));
    // A property without an annotation simply has none.
    QVERIFY(!propertyOf(schema, QStringLiteral("name")).contains(QStringLiteral("description")));
}

void TestMetaSchema::describesMethodArguments()
{
    Weather weather;
    const QJsonObject schema
            = MetaSchema::fromMethod(weather.metaObject(), QStringLiteral("forecast"));

    QCOMPARE(schema.value(QStringLiteral("type")).toString(), QStringLiteral("object"));
    // The parameter names come from the signature moc recorded, so the schema
    // and the handler cannot disagree about them.
    QCOMPARE(
            propertyOf(schema, QStringLiteral("location")).value(QStringLiteral("type")).toString(),
            QStringLiteral("string"));
    QCOMPARE(propertyOf(schema, QStringLiteral("days")).value(QStringLiteral("type")).toString(),
             QStringLiteral("integer"));
    QCOMPARE(requiredOf(schema), QStringList({QStringLiteral("location"), QStringLiteral("days")}));
    // Per-argument descriptions are addressed as doc:<method>:<argument>.
    QCOMPARE(propertyOf(schema, QStringLiteral("location"))
                     .value(QStringLiteral("description"))
                     .toString(),
             QStringLiteral("City name"));
}

void TestMetaSchema::describesAMethodWithoutArguments()
{
    Weather weather;
    const QJsonObject schema
            = MetaSchema::fromMethod(weather.metaObject(), QStringLiteral("noArguments"));

    QCOMPARE(schema.value(QStringLiteral("type")).toString(), QStringLiteral("object"));
    QVERIFY(schema.value(QStringLiteral("properties")).toObject().isEmpty());
    QVERIFY(!schema.contains(QStringLiteral("required")));
}

void TestMetaSchema::unknownTypesStayUnconstrained()
{
    // A type the mapping does not know must not be described wrongly; an empty
    // schema accepts anything, which is the honest answer.
    QVERIFY(MetaSchema::fromMetaType(QMetaType::fromType<QVariant>()).isEmpty());
    // ... and a method naming one still produces a usable object schema.
    QVERIFY(MetaSchema::fromMethod(nullptr, QStringLiteral("nope")).isEmpty());
}

void TestMetaSchema::theMacrosBuildTheSameKeysAsHandWrittenOnes()
{
    // The macros are only a spelling: what reaches the meta-object has to be
    // the same key a hand-written Q_CLASSINFO would have produced, or the
    // convention has silently forked in two.
    const QMetaObject &person = Person::staticMetaObject;
    QVERIFY(person.indexOfClassInfo("doc") >= 0);
    QVERIFY(person.indexOfClassInfo("doc:age") >= 0);
    QCOMPARE(person.classInfo(person.indexOfClassInfo("doc:age")).value(), "Whole years");

    // moc concatenates the three literals of the argument form into one key.
    const QMetaObject &weather = Weather::staticMetaObject;
    const int index = weather.indexOfClassInfo("doc:forecast:location");
    QVERIFY(index >= 0);
    QCOMPARE(weather.classInfo(index).value(), "City name");
}

void TestMetaSchema::aMethodAndItsArgumentsAreOneInvocation()
{
    // Written per argument, a method's name appeared once per parameter plus
    // once for itself -- every repetition a place for the two to drift apart
    // while still compiling. Grouped, it is written once. What lands in the
    // meta-object has to be exactly what the separate form produced.
    const QMetaObject &meta = Grouped::staticMetaObject;

    const auto info = [&meta](const char *key) {
        const int index = meta.indexOfClassInfo(key);
        return index < 0 ? QByteArray() : QByteArray(meta.classInfo(index).value());
    };

    // No arguments: the same macro, nothing after the description. One macro to
    // know rather than two.
    QCOMPARE(info("doc:nothing"), QByteArray("No arguments at all."));
    QVERIFY(meta.indexOfClassInfo("doc:nothing:") < 0);

    QCOMPARE(info("doc:one"), QByteArray("One argument."));
    QCOMPARE(info("doc:one:only"), QByteArray("The only one."));

    QCOMPARE(info("doc:two"), QByteArray("Two arguments."));
    QCOMPARE(info("doc:two:first"), QByteArray("The first."));
    QCOMPARE(info("doc:two:second"), QByteArray("The second."));

    // The ceiling, every argument distinct so an off-by-one in the dispatch
    // chain shows up as a missing or duplicated key rather than passing.
    QCOMPARE(info("doc:eight"), QByteArray("Eight arguments."));
    const QByteArrayList names {"a", "b", "c", "d", "e", "f", "g", "h"};
    for (int i = 0; i < names.size(); ++i) {
        const QByteArray key = "doc:eight:" + names.at(i);
        QCOMPARE(info(key.constData()), QByteArray(names.at(i).toUpper() + "."));
    }

    // And the descriptions reach the schema, which is the only reason any of
    // this exists.
    const QJsonObject schema = MetaSchema::fromMethod(&meta, QStringLiteral("two"));
    const QJsonObject properties = schema.value(QStringLiteral("properties")).toObject();
    QCOMPARE(properties.value(QStringLiteral("first"))
                     .toObject()
                     .value(QStringLiteral("description"))
                     .toString(),
             QStringLiteral("The first."));
    QCOMPARE(properties.value(QStringLiteral("second"))
                     .toObject()
                     .value(QStringLiteral("description"))
                     .toString(),
             QStringLiteral("The second."));

    // Nothing described that is not there: the grouped form must not invent a
    // key, which is the failure mode a dispatch chain off by one would have.
    QCOMPARE(MetaSchema::danglingAnnotations<Grouped>(), QStringList());
}

void TestMetaSchema::anAnnotationMaySitNextToWhatItDescribes()
{
    // Cutelyst's C_ATTR sits on the line above the method it annotates, and it
    // is the better place: the description is where the signature is, so
    // renaming an argument and forgetting its description is a change in one
    // place rather than two screens apart.
    //
    // Q_CLASSINFO does not care where in the class body it appears, and this
    // pins that: same keys, same values, whether grouped at the top or written
    // next to each method -- including across an access specifier in between.
    const QMetaObject &grouped = Grouped::staticMetaObject;
    const QMetaObject &adjacent = Adjacent::staticMetaObject;

    const auto info = [](const QMetaObject &meta, const char *key) {
        const int index = meta.indexOfClassInfo(key);
        return index < 0 ? QByteArray() : QByteArray(meta.classInfo(index).value());
    };

    for (const char *key : {"doc", "doc:nothing", "doc:one", "doc:one:only", "doc:two",
                            "doc:two:first", "doc:two:second"}) {
        QVERIFY2(!info(adjacent, key).isEmpty(), key);
        QCOMPARE(info(adjacent, key), info(grouped, key));
    }

    // And the schema built from either is the same object, which is the only
    // thing that actually reaches the model.
    QCOMPARE(MetaSchema::fromMethod(&adjacent, QStringLiteral("two")),
             MetaSchema::fromMethod(&grouped, QStringLiteral("two")));
    QCOMPARE(MetaSchema::danglingAnnotations<Adjacent>(), QStringList());
}

void TestMetaSchema::theShippedAnnotationsAllDescribeSomething()
{
    // The macros make a typo a lone mistake rather than one hidden among
    // punctuation -- but `QTOPENAI_DOC_METHOD(raed_file, ...)` still compiles,
    // and a key matching nothing is simply never read. Only the meta-object
    // knows the real names, so only a runtime check can answer this, and it
    // costs one line per annotated class.
    //
    // These are the classes this library itself ships annotated. A rename that
    // forgets its description fails here rather than quietly shipping a tool
    // the model is told nothing about.
    QCOMPARE(MetaSchema::danglingAnnotations<Person>(), QStringList());
    QCOMPARE(MetaSchema::danglingAnnotations<Weather>(), QStringList());
    QCOMPARE(MetaSchema::danglingAnnotations<Grouped>(), QStringList());
    QCOMPARE(MetaSchema::danglingAnnotations<Adjacent>(), QStringList());
}

void TestMetaSchema::findsAnnotationsThatDescribeNothing()
{
    // A description that matches nothing never appears in a schema, so nothing
    // about the output says it was wrong. This is what says so.
    QCOMPARE(MetaSchema::danglingAnnotations<Person>(), QStringList());
    QCOMPARE(MetaSchema::danglingAnnotations<Weather>(), QStringList());

    const QStringList dangling = MetaSchema::danglingAnnotations<Misannotated>();
    QCOMPARE(dangling,
             QStringList({QStringLiteral("doc:subtitle"), QStringLiteral("doc:remove"),
                          QStringLiteral("doc:rename:name"), QStringLiteral("doc:remove:title"),
                          QStringLiteral("doc:objectName")}));

    QCOMPARE(MetaSchema::danglingAnnotations(nullptr), QStringList());
}

QTEST_MAIN(TestMetaSchema)
#include "tst_metaschema.moc"
