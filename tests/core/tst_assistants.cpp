// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/Assistant.h>
#include <QtOpenAi/Core/CreateAssistantRequest.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Assistants types (#23): the assistant object with its open
// `tools`/`tool_resources` unions, the deletion acknowledgement, and the request
// body that both creates and modifies one.
class TestAssistants : public QObject
{
    Q_OBJECT
private slots:
    void parsesAssistant();
    void assistantRoundTrip();
    void parsesDeletionAcknowledgement();
    void unsetSamplingStaysUnset();
    void requestSerialisesBody();
    void requestBuildsToolEntries();
    void requestOmitsUnsetFields();
};

namespace {

QJsonArray sampleTools()
{
    return QJsonArray {
            QJsonObject {{QStringLiteral("type"), QStringLiteral("code_interpreter")}},
            QJsonObject {{QStringLiteral("type"), QStringLiteral("function")},
                         {QStringLiteral("function"),
                          QJsonObject {{QStringLiteral("name"), QStringLiteral("get_weather")}}}},
    };
}

QJsonObject sampleToolResources()
{
    return QJsonObject {
            {QStringLiteral("file_search"), QJsonObject {{QStringLiteral("vector_store_ids"),
                                                          QJsonArray {QStringLiteral("vs_1")}}}},
    };
}

} // namespace

void TestAssistants::parsesAssistant()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("asst_abc123")},
            {QStringLiteral("object"), QStringLiteral("assistant")},
            {QStringLiteral("created_at"), 1716028800},
            {QStringLiteral("name"), QStringLiteral("Weather bot")},
            {QStringLiteral("description"), QStringLiteral("Answers weather questions")},
            {QStringLiteral("model"), QStringLiteral("gpt-4o-mini")},
            {QStringLiteral("instructions"), QStringLiteral("Be concise.")},
            {QStringLiteral("tools"), sampleTools()},
            {QStringLiteral("tool_resources"), sampleToolResources()},
            {QStringLiteral("metadata"),
             QJsonObject {{QStringLiteral("team"), QStringLiteral("qa")}}},
            {QStringLiteral("temperature"), 0.4},
            {QStringLiteral("top_p"), 0.9},
            {QStringLiteral("response_format"), QStringLiteral("auto")},
    };

    const Assistant assistant = Assistant::fromJson(json);
    QCOMPARE(assistant.id(), QStringLiteral("asst_abc123"));
    QCOMPARE(assistant.object(), QStringLiteral("assistant"));
    QCOMPARE(assistant.createdAt(), Q_INT64_C(1716028800));
    QCOMPARE(assistant.name(), QStringLiteral("Weather bot"));
    QCOMPARE(assistant.description(), QStringLiteral("Answers weather questions"));
    QCOMPARE(assistant.model(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(assistant.instructions(), QStringLiteral("Be concise."));
    // The tool list is an open union (hosted tools carry their own config), so
    // it stays raw JSON.
    QCOMPARE(assistant.tools().size(), 2);
    QCOMPARE(assistant.tools().first().toObject().value(QStringLiteral("type")).toString(),
             QStringLiteral("code_interpreter"));
    QCOMPARE(assistant.toolResources()
                     .value(QStringLiteral("file_search"))
                     .toObject()
                     .value(QStringLiteral("vector_store_ids"))
                     .toArray()
                     .size(),
             1);
    QCOMPARE(assistant.metadata().value(QStringLiteral("team")).toString(), QStringLiteral("qa"));
    QCOMPARE(assistant.temperature().value(), 0.4);
    QCOMPARE(assistant.topP().value(), 0.9);
    QCOMPARE(assistant.responseFormat().toString(), QStringLiteral("auto"));
}

void TestAssistants::assistantRoundTrip()
{
    Assistant assistant;
    assistant.setId(QStringLiteral("asst_1"));
    assistant.setObject(QStringLiteral("assistant"));
    assistant.setCreatedAt(1700000000);
    assistant.setName(QStringLiteral("Helper"));
    assistant.setDescription(QStringLiteral("desc"));
    assistant.setModel(QStringLiteral("gpt-4o-mini"));
    assistant.setInstructions(QStringLiteral("Be brief."));
    assistant.setTools(sampleTools());
    assistant.setToolResources(sampleToolResources());
    assistant.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});
    assistant.setTemperature(0.25);
    assistant.setTopP(0.75);
    assistant.setResponseFormat(QJsonObject {{QStringLiteral("type"), QStringLiteral("text")}});

    QCOMPARE(Assistant::fromJson(assistant.toJson()), assistant);
}

void TestAssistants::parsesDeletionAcknowledgement()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("asst_abc123")},
            {QStringLiteral("object"), QStringLiteral("assistant.deleted")},
            {QStringLiteral("deleted"), true},
    };

    const Assistant assistant = Assistant::fromJson(json);
    QCOMPARE(assistant.id(), QStringLiteral("asst_abc123"));
    QCOMPARE(assistant.object(), QStringLiteral("assistant.deleted"));
}

void TestAssistants::unsetSamplingStaysUnset()
{
    // The API reports an unconfigured sampling parameter as null; that must
    // decode to an unset optional rather than an invented 0.
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("asst_1")},
            {QStringLiteral("temperature"), QJsonValue::Null},
            {QStringLiteral("response_format"), QJsonValue::Null},
    };

    const Assistant assistant = Assistant::fromJson(json);
    QVERIFY(!assistant.temperature().has_value());
    QVERIFY(!assistant.topP().has_value());
    QVERIFY(assistant.responseFormat().isUndefined());
    // ... and an unset value is left out of the body entirely.
    QVERIFY(!assistant.toJson().contains(QStringLiteral("temperature")));
    QVERIFY(!assistant.toJson().contains(QStringLiteral("response_format")));
}

void TestAssistants::requestSerialisesBody()
{
    CreateAssistantRequest request(QStringLiteral("gpt-4o-mini"));
    request.setName(QStringLiteral("Weather bot"));
    request.setDescription(QStringLiteral("Answers weather questions"));
    request.setInstructions(QStringLiteral("Be concise."));
    request.setToolResources(sampleToolResources());
    request.setMetadata(QJsonObject {{QStringLiteral("team"), QStringLiteral("qa")}});
    request.setTemperature(0.4);
    request.setTopP(0.9);
    request.setResponseFormat(ResponseFormat::jsonObject());

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("model")).toString(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(json.value(QStringLiteral("name")).toString(), QStringLiteral("Weather bot"));
    QCOMPARE(json.value(QStringLiteral("instructions")).toString(), QStringLiteral("Be concise."));
    QCOMPARE(json.value(QStringLiteral("temperature")).toDouble(), 0.4);
    QCOMPARE(json.value(QStringLiteral("top_p")).toDouble(), 0.9);
    // The typed ResponseFormat serialises into the same field the raw overload
    // would fill.
    QCOMPARE(json.value(QStringLiteral("response_format"))
                     .toObject()
                     .value(QStringLiteral("type"))
                     .toString(),
             QStringLiteral("json_object"));
    QVERIFY(json.contains(QStringLiteral("tool_resources")));
}

void TestAssistants::requestBuildsToolEntries()
{
    CreateAssistantRequest request(QStringLiteral("gpt-4o-mini"));
    request.addTool(
            Tool::function(QStringLiteral("get_weather"), QStringLiteral("Current weather"),
                           QJsonObject {{QStringLiteral("type"), QStringLiteral("object")}}));
    request.addCodeInterpreterTool();
    request.addFileSearchTool();
    request.addTool(QJsonObject {{QStringLiteral("type"), QStringLiteral("something_new")}});

    const QJsonArray tools = request.toJson().value(QStringLiteral("tools")).toArray();
    QCOMPARE(tools.size(), 4);
    // A typed function tool keeps the shape the rest of the library gives it.
    const QJsonObject function = tools.at(0).toObject();
    QCOMPARE(function.value(QStringLiteral("type")).toString(), QStringLiteral("function"));
    QCOMPARE(function.value(QStringLiteral("function"))
                     .toObject()
                     .value(QStringLiteral("name"))
                     .toString(),
             QStringLiteral("get_weather"));
    QCOMPARE(tools.at(1).toObject().value(QStringLiteral("type")).toString(),
             QStringLiteral("code_interpreter"));
    QCOMPARE(tools.at(2).toObject().value(QStringLiteral("type")).toString(),
             QStringLiteral("file_search"));
    // An unmodelled tool type passes through untouched.
    QCOMPARE(tools.at(3).toObject().value(QStringLiteral("type")).toString(),
             QStringLiteral("something_new"));
}

void TestAssistants::requestOmitsUnsetFields()
{
    // The update endpoint reuses this body, so an untouched request must carry
    // nothing at all -- otherwise a rename would also reset the instructions.
    const CreateAssistantRequest empty;
    QVERIFY(empty.toJson().isEmpty());

    CreateAssistantRequest rename;
    rename.setName(QStringLiteral("New name"));
    const QJsonObject json = rename.toJson();
    QCOMPARE(json.size(), 1);
    QCOMPARE(json.value(QStringLiteral("name")).toString(), QStringLiteral("New name"));
}

QTEST_MAIN(TestAssistants)
#include "tst_assistants.moc"
