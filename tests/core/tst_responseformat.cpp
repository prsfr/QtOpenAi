// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ChatCompletionRequest.h>
#include <QtOpenAi/Core/ResponseFormat.h>
#include <QtOpenAi/Core/ResponseRequest.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for Structured Outputs (#9): the ResponseFormat union serialises to
// both the Chat Completions `response_format` (nested) and Responses API
// `text.format` (inlined) shapes, and wires into both request bodies.
class TestResponseFormat : public QObject
{
    Q_OBJECT
private:
    static QJsonObject personSchema()
    {
        return QJsonObject {
                {QStringLiteral("type"), QStringLiteral("object")},
                {QStringLiteral("properties"),
                 QJsonObject {{QStringLiteral("name"),
                               QJsonObject {{QStringLiteral("type"), QStringLiteral("string")}}},
                              {QStringLiteral("age"),
                               QJsonObject {{QStringLiteral("type"), QStringLiteral("integer")}}}}},
                {QStringLiteral("required"),
                 QJsonArray {QStringLiteral("name"), QStringLiteral("age")}},
                {QStringLiteral("additionalProperties"), false}};
    }

private slots:
    void textVariant();
    void jsonObjectVariant();
    void jsonSchemaChatShape();
    void jsonSchemaTextFormatShape();
    void schemaPreservedVerbatim();
    void roundTripBothShapes();
    void chatRequestWiring();
    void responseRequestWiring();
    void nullByDefault();
};

void TestResponseFormat::textVariant()
{
    const ResponseFormat format = ResponseFormat::text();
    QCOMPARE(format.type(), QStringLiteral("text"));
    QVERIFY(!format.isNull());
    const QJsonObject json = format.toJson();
    QCOMPARE(json.value(QStringLiteral("type")).toString(), QStringLiteral("text"));
    QVERIFY(!json.contains(QStringLiteral("json_schema")));
}

void TestResponseFormat::jsonObjectVariant()
{
    const ResponseFormat format = ResponseFormat::jsonObject();
    QCOMPARE(format.type(), QStringLiteral("json_object"));
    const QJsonObject json = format.toJson();
    QCOMPARE(json.value(QStringLiteral("type")).toString(), QStringLiteral("json_object"));
    QVERIFY(!json.contains(QStringLiteral("json_schema")));
}

void TestResponseFormat::jsonSchemaChatShape()
{
    const ResponseFormat format = ResponseFormat::jsonSchema(
            QStringLiteral("person"), personSchema(), true, QStringLiteral("A person record"));
    const QJsonObject json = format.toJson();
    QCOMPARE(json.value(QStringLiteral("type")).toString(), QStringLiteral("json_schema"));
    const QJsonObject nested = json.value(QStringLiteral("json_schema")).toObject();
    QCOMPARE(nested.value(QStringLiteral("name")).toString(), QStringLiteral("person"));
    QCOMPARE(nested.value(QStringLiteral("description")).toString(),
             QStringLiteral("A person record"));
    QVERIFY(nested.value(QStringLiteral("strict")).toBool());
    QCOMPARE(nested.value(QStringLiteral("schema")).toObject(), personSchema());
}

void TestResponseFormat::jsonSchemaTextFormatShape()
{
    const ResponseFormat format
            = ResponseFormat::jsonSchema(QStringLiteral("person"), personSchema());
    const QJsonObject json = format.toTextFormatJson();
    // Fields are inlined (no json_schema wrapper) for the Responses API.
    QCOMPARE(json.value(QStringLiteral("type")).toString(), QStringLiteral("json_schema"));
    QCOMPARE(json.value(QStringLiteral("name")).toString(), QStringLiteral("person"));
    QVERIFY(!json.contains(QStringLiteral("json_schema")));
    QCOMPARE(json.value(QStringLiteral("schema")).toObject(), personSchema());
    QVERIFY(json.value(QStringLiteral("strict")).toBool());
    // Empty description is omitted.
    QVERIFY(!json.contains(QStringLiteral("description")));
}

void TestResponseFormat::schemaPreservedVerbatim()
{
    const QJsonObject schema = personSchema();
    const ResponseFormat format = ResponseFormat::jsonSchema(QStringLiteral("person"), schema);
    // The schema object must be carried through unchanged, not re-derived.
    QCOMPARE(format.schema(), schema);
    QCOMPARE(ResponseFormat::fromJson(format.toJson()).schema(), schema);
    QCOMPARE(ResponseFormat::fromJson(format.toTextFormatJson()).schema(), schema);
}

void TestResponseFormat::roundTripBothShapes()
{
    const ResponseFormat format = ResponseFormat::jsonSchema(
            QStringLiteral("person"), personSchema(), false, QStringLiteral("desc"));
    QCOMPARE(ResponseFormat::fromJson(format.toJson()), format);
    QCOMPARE(ResponseFormat::fromJson(format.toTextFormatJson()), format);

    const ResponseFormat text = ResponseFormat::text();
    QCOMPARE(ResponseFormat::fromJson(text.toJson()), text);
}

void TestResponseFormat::chatRequestWiring()
{
    ChatCompletionRequest request(QStringLiteral("gpt-4o"), {Message::user(QStringLiteral("hi"))});
    QVERIFY(!request.toJson().contains(QStringLiteral("response_format")));

    request.setResponseFormat(ResponseFormat::jsonSchema(QStringLiteral("person"), personSchema()));
    const QJsonObject json = request.toJson();
    const QJsonObject rf = json.value(QStringLiteral("response_format")).toObject();
    QCOMPARE(rf.value(QStringLiteral("type")).toString(), QStringLiteral("json_schema"));
    QVERIFY(rf.contains(QStringLiteral("json_schema")));

    const ChatCompletionRequest parsed = ChatCompletionRequest::fromJson(json);
    QCOMPARE(parsed, request);
    QVERIFY(parsed.responseFormat().has_value());
    QCOMPARE(parsed.responseFormat()->schema(), personSchema());
}

void TestResponseFormat::responseRequestWiring()
{
    ResponseRequest request(QStringLiteral("gpt-4o"), QStringLiteral("hi"));
    QVERIFY(!request.toJson().contains(QStringLiteral("text")));

    request.setTextFormat(ResponseFormat::jsonSchema(QStringLiteral("person"), personSchema()));
    const QJsonObject json = request.toJson();
    const QJsonObject text = json.value(QStringLiteral("text")).toObject();
    const QJsonObject fmt = text.value(QStringLiteral("format")).toObject();
    QCOMPARE(fmt.value(QStringLiteral("type")).toString(), QStringLiteral("json_schema"));
    // Responses shape inlines the schema fields.
    QCOMPARE(fmt.value(QStringLiteral("name")).toString(), QStringLiteral("person"));
    QVERIFY(!fmt.contains(QStringLiteral("json_schema")));

    const ResponseRequest parsed = ResponseRequest::fromJson(json);
    QCOMPARE(parsed, request);
    QVERIFY(parsed.textFormat().has_value());
    QCOMPARE(parsed.textFormat()->schema(), personSchema());
}

void TestResponseFormat::nullByDefault()
{
    const ResponseFormat format;
    QVERIFY(format.isNull());
    QVERIFY(format.strict()); // default is strict decoding
}

QTEST_APPLESS_MAIN(TestResponseFormat)
#include "tst_responseformat.moc"
