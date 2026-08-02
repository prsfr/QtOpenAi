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
