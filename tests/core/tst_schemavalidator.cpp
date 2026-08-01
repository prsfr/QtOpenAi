// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/MetaSchema.h>
#include <QtOpenAi/Core/SchemaValidator.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

namespace {

QJsonObject schemaOf(const char *json)
{
    return QJsonDocument::fromJson(QByteArray(json)).object();
}

QJsonValue valueOf(const char *json)
{
    // Wrapped so scalars parse too -- QJsonDocument only accepts a document.
    return QJsonDocument::fromJson(QByteArray("[") + json + "]").array().at(0);
}

// A gadget whose schema comes from the meta-object, to prove the two halves of
// the feature speak the same vocabulary.
class Booking
{
    Q_GADGET
    Q_PROPERTY(QString city MEMBER city)
    Q_PROPERTY(int nights MEMBER nights)
public:
    QString city;
    int nights = 0;
};

} // namespace

// Coverage for the JSON-Schema subset used to check model-generated arguments
// (#41). Each slot pins one keyword family, plus the shape of the message: the
// error text goes back to the model, so it has to name what is wrong and where.
class TestSchemaValidator : public QObject
{
    Q_OBJECT
private slots:
    void acceptsAValueMatchingItsType();
    void reportsTypeMismatch();
    void treatsIntegerAsAWholeNumber();
    void reportsMissingRequiredProperties();
    void rejectsUnexpectedPropertiesWhenClosed();
    void checksNestedPropertiesByPath();
    void checksArrayItems();
    void checksEnumAndConst();
    void checksNumericBounds();
    void checksStringConstraints();
    void unknownKeywordsConstrainNothing();
    void validatesAgainstAGeneratedSchema();
};

void TestSchemaValidator::acceptsAValueMatchingItsType()
{
    const QJsonObject schema = schemaOf(R"({"type":"object",
        "properties":{"city":{"type":"string"},"nights":{"type":"integer"}},
        "required":["city","nights"],"additionalProperties":false})");

    QVERIFY(SchemaValidator::isValid(schema, valueOf(R"({"city":"Berlin","nights":3})")));
    QVERIFY(SchemaValidator::validate(schema, valueOf(R"({"city":"Berlin","nights":3})"))
                    .isEmpty());
}

void TestSchemaValidator::reportsTypeMismatch()
{
    const QJsonObject schema = schemaOf(R"({"type":"object",
        "properties":{"nights":{"type":"integer"}}})");

    const QStringList errors = SchemaValidator::validate(schema, valueOf(R"({"nights":"three"})"));
    QCOMPARE(errors.size(), 1);
    QCOMPARE(errors.first(), QStringLiteral("/nights: expected integer, got string"));

    // A `type` listing alternatives passes when any of them fits.
    const QJsonObject nullable = schemaOf(R"({"type":["string","null"]})");
    QVERIFY(SchemaValidator::isValid(nullable, valueOf("null")));
    QCOMPARE(SchemaValidator::validate(nullable, valueOf("7")).first(),
             QStringLiteral("expected string or null, got number"));
}

void TestSchemaValidator::treatsIntegerAsAWholeNumber()
{
    // JSON has a single numeric type; `integer` is the whole-valued part of it.
    const QJsonObject schema = schemaOf(R"({"type":"integer"})");
    QVERIFY(SchemaValidator::isValid(schema, valueOf("3")));
    QVERIFY(SchemaValidator::isValid(schema, valueOf("3.0")));
    QVERIFY(!SchemaValidator::isValid(schema, valueOf("3.5")));
    // ... while `number` accepts both.
    QVERIFY(SchemaValidator::isValid(schemaOf(R"({"type":"number"})"), valueOf("3.5")));
}

void TestSchemaValidator::reportsMissingRequiredProperties()
{
    const QJsonObject schema = schemaOf(R"({"type":"object",
        "properties":{"city":{"type":"string"},"nights":{"type":"integer"}},
        "required":["city","nights"]})");

    const QStringList errors = SchemaValidator::validate(schema, valueOf(R"({"city":"Berlin"})"));
    QCOMPARE(errors, QStringList {QStringLiteral("missing required property 'nights'")});
}

void TestSchemaValidator::rejectsUnexpectedPropertiesWhenClosed()
{
    const QJsonObject closed = schemaOf(R"({"type":"object",
        "properties":{"city":{"type":"string"}},"additionalProperties":false})");
    QCOMPARE(SchemaValidator::validate(closed, valueOf(R"({"city":"Berlin","hotel":"x"})")),
             QStringList {QStringLiteral("unexpected property 'hotel'")});

    // Without the keyword, extra properties are simply unconstrained.
    const QJsonObject open = schemaOf(R"({"type":"object",
        "properties":{"city":{"type":"string"}}})");
    QVERIFY(SchemaValidator::isValid(open, valueOf(R"({"city":"Berlin","hotel":"x"})")));
}

void TestSchemaValidator::checksNestedPropertiesByPath()
{
    // The path is what makes a nested failure actionable for the model.
    const QJsonObject schema = schemaOf(R"({"type":"object",
        "properties":{"address":{"type":"object",
            "properties":{"number":{"type":"integer"}},"required":["number"]}}})");

    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"({"address":{"number":"12"}})")),
             QStringList {QStringLiteral("/address/number: expected integer, got string")});
    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"({"address":{}})")),
             QStringList {QStringLiteral("/address: missing required property 'number'")});
}

void TestSchemaValidator::checksArrayItems()
{
    const QJsonObject schema = schemaOf(R"({"type":"array",
        "items":{"type":"string"},"minItems":1,"maxItems":2})");

    QVERIFY(SchemaValidator::isValid(schema, valueOf(R"(["a","b"])")));
    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"(["a",2])")),
             QStringList {QStringLiteral("/1: expected string, got number")});
    QCOMPARE(SchemaValidator::validate(schema, valueOf("[]")),
             QStringList {QStringLiteral("has fewer than 1 items")});
    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"(["a","b","c"])")),
             QStringList {QStringLiteral("has more than 2 items")});
}

void TestSchemaValidator::checksEnumAndConst()
{
    const QJsonObject schema = schemaOf(R"({"type":"string","enum":["Happy","Sad"]})");
    QVERIFY(SchemaValidator::isValid(schema, valueOf(R"("Sad")")));
    // The message repeats the allowed set, so the model can pick from it.
    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"("Angry")")),
             QStringList {QStringLiteral("expected one of \"Happy\", \"Sad\", got \"Angry\"")});

    const QJsonObject constant = schemaOf(R"({"const":"weather"})");
    QVERIFY(SchemaValidator::isValid(constant, valueOf(R"("weather")")));
    QCOMPARE(SchemaValidator::validate(constant, valueOf(R"("news")")),
             QStringList {QStringLiteral("expected \"weather\", got \"news\"")});
}

void TestSchemaValidator::checksNumericBounds()
{
    const QJsonObject schema = schemaOf(R"({"type":"integer","minimum":1,"maximum":10})");
    QVERIFY(SchemaValidator::isValid(schema, valueOf("1")));
    QVERIFY(SchemaValidator::isValid(schema, valueOf("10")));
    QCOMPARE(SchemaValidator::validate(schema, valueOf("0")),
             QStringList {QStringLiteral("0 is not at least 1")});
    QCOMPARE(SchemaValidator::validate(schema, valueOf("11")),
             QStringList {QStringLiteral("11 is not at most 10")});

    const QJsonObject exclusive
            = schemaOf(R"({"type":"number","exclusiveMinimum":0,"exclusiveMaximum":1})");
    QVERIFY(SchemaValidator::isValid(exclusive, valueOf("0.5")));
    QCOMPARE(SchemaValidator::validate(exclusive, valueOf("0")),
             QStringList {QStringLiteral("0 is not greater than 0")});
    QCOMPARE(SchemaValidator::validate(exclusive, valueOf("1")),
             QStringList {QStringLiteral("1 is not less than 1")});
}

void TestSchemaValidator::checksStringConstraints()
{
    const QJsonObject schema
            = schemaOf(R"({"type":"string","minLength":2,"maxLength":4,"pattern":"^[a-z]+$"})");
    QVERIFY(SchemaValidator::isValid(schema, valueOf(R"("abc")")));
    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"("a")")),
             QStringList {QStringLiteral("shorter than 2 characters")});
    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"("abcde")")),
             QStringList {QStringLiteral("longer than 4 characters")});
    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"("AB")")),
             QStringList {QStringLiteral("does not match ^[a-z]+$")});
}

void TestSchemaValidator::unknownKeywordsConstrainNothing()
{
    // An empty schema accepts anything, and so does one built only from
    // keywords this validator does not implement -- a partial understanding
    // must not reject valid data.
    QVERIFY(SchemaValidator::isValid(QJsonObject {}, valueOf(R"({"anything":true})")));
    QVERIFY(SchemaValidator::isValid(schemaOf(R"({"allOf":[{"type":"string"}]})"), valueOf("7")));
}

void TestSchemaValidator::validatesAgainstAGeneratedSchema()
{
    // The schema MetaSchema derives is exactly the schema this validates, which
    // is the whole point: the advertised contract and the check are one source.
    const QJsonObject schema = MetaSchema::fromType<Booking>();

    QVERIFY(SchemaValidator::isValid(schema, valueOf(R"({"city":"Berlin","nights":3})")));
    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"({"city":"Berlin","nights":"three"})")),
             QStringList {QStringLiteral("/nights: expected integer, got string")});
    QCOMPARE(SchemaValidator::validate(schema, valueOf(R"({"city":"Berlin"})")),
             QStringList {QStringLiteral("missing required property 'nights'")});
}

QTEST_MAIN(TestSchemaValidator)
#include "tst_schemavalidator.moc"
