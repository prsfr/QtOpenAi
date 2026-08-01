// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/ToolRegistry.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

// A sample "tool provider" whose invokable methods are dispatched through the
// Qt meta-object system by ToolRegistry::registerMethod.
class WeatherProvider : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("doc:forecast", "Weather for a city")
    Q_CLASSINFO("doc:forecast:location", "City name")
public:
    using QObject::QObject;

    enum class Unit {
        Celsius,
        Fahrenheit,
    };
    Q_ENUM(Unit)

    Q_INVOKABLE QString getWeather(const QJsonObject &args)
    {
        lastLocation = args.value(QStringLiteral("location")).toString();
        return QStringLiteral("sunny in %1").arg(lastLocation);
    }

    // An ordinary C++ signature: the registry fills the parameters in from the
    // model's JSON rather than handing the method an object to unpack.
    Q_INVOKABLE QString forecast(const QString &location, int days)
    {
        lastLocation = location;
        lastDays = days;
        return QStringLiteral("clear in %1 for %2 days").arg(location).arg(days);
    }

    Q_INVOKABLE QString convert(double degrees, Unit unit)
    {
        lastUnit = unit;
        return QString::number(unit == Unit::Celsius ? degrees : degrees * 1.8 + 32);
    }

    Q_INVOKABLE QJsonObject conditions(const QString &location)
    {
        return QJsonObject {{QStringLiteral("location"), location},
                            {QStringLiteral("sky"), QStringLiteral("clear")}};
    }

    QString lastLocation;
    int lastDays = 0;
    Unit lastUnit = Unit::Celsius;
};

class TestToolRegistry : public QObject
{
    Q_OBJECT
private slots:
    void functorDispatchProducesToolResult();
    void metaObjectDispatchByName();
    void registerMethodRejectsMissingSlot();
    void unknownToolEmitsSignalAndErrorPayload();
    void toolsAdvertisedInInsertionOrder();
    void invokeAllReturnsOnePerCall();
    void inferredRegistrationDerivesTheToolDefinition();
    void typedParametersAreFilledFromTheArguments();
    void enumArgumentsArriveAsTheirKey();
    void structuredReturnValuesAreSerialised();
    void missingArgumentsAreReported();
    void validationRejectsArgumentsBeforeDispatch();
    void validationIsOffByDefault();

private:
    static ToolCall makeCall(const QString &id, const QString &name, const QString &args)
    {
        return ToolCall(id, FunctionCall(name, args));
    }
};

void TestToolRegistry::functorDispatchProducesToolResult()
{
    ToolRegistry registry;
    registry.registerFunction(QStringLiteral("add"), QStringLiteral("Add two numbers"),
                              QJsonObject {}, [](const QJsonObject &args) {
                                  const int sum = args.value(QStringLiteral("a")).toInt()
                                                  + args.value(QStringLiteral("b")).toInt();
                                  return QString::number(sum);
                              });

    QSignalSpy invokedSpy(&registry, &ToolRegistry::toolInvoked);

    const Message result = registry.invoke(makeCall(QStringLiteral("c1"), QStringLiteral("add"),
                                                    QStringLiteral("{\"a\":2,\"b\":3}")));

    QCOMPARE(result.role(), Role::Tool);
    QCOMPARE(result.toolCallId(), QStringLiteral("c1"));
    QCOMPARE(result.content(), QStringLiteral("5"));
    QCOMPARE(invokedSpy.count(), 1);
    QCOMPARE(invokedSpy.first().at(2).toString(), QStringLiteral("5"));
}

void TestToolRegistry::metaObjectDispatchByName()
{
    ToolRegistry registry;
    WeatherProvider provider;

    const Tool tool = Tool::function(QStringLiteral("get_weather"),
                                     QStringLiteral("Weather lookup"), QJsonObject {});
    const bool registered = registry.registerMethod(tool, &provider, QStringLiteral("getWeather"));
    QVERIFY(registered);
    QVERIFY(registry.contains(QStringLiteral("get_weather")));

    const Message result
            = registry.invoke(makeCall(QStringLiteral("c2"), QStringLiteral("get_weather"),
                                       QStringLiteral("{\"location\":\"Berlin\"}")));

    // The slot ran through QMetaObject::invokeMethod and mutated the provider.
    QCOMPARE(provider.lastLocation, QStringLiteral("Berlin"));
    QCOMPARE(result.content(), QStringLiteral("sunny in Berlin"));
}

void TestToolRegistry::registerMethodRejectsMissingSlot()
{
    ToolRegistry registry;
    WeatherProvider provider;
    const Tool tool = Tool::function(QStringLiteral("nope"), QString(), QJsonObject {});
    QVERIFY(!registry.registerMethod(tool, &provider, QStringLiteral("doesNotExist")));
    QVERIFY(!registry.contains(QStringLiteral("nope")));
}

void TestToolRegistry::unknownToolEmitsSignalAndErrorPayload()
{
    ToolRegistry registry;
    QSignalSpy unknownSpy(&registry, &ToolRegistry::unknownTool);
    QSignalSpy failedSpy(&registry, &ToolRegistry::toolFailed);

    const Message result = registry.invoke(
            makeCall(QStringLiteral("c3"), QStringLiteral("ghost"), QStringLiteral("{}")));

    QCOMPARE(unknownSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(result.role(), Role::Tool);

    const QJsonObject payload = QJsonDocument::fromJson(result.content().toUtf8()).object();
    QVERIFY(payload.contains(QStringLiteral("error")));
}

void TestToolRegistry::toolsAdvertisedInInsertionOrder()
{
    ToolRegistry registry;
    registry.registerFunction(QStringLiteral("first"), QString(), QJsonObject {},
                              [](const QJsonObject &) { return QString(); });
    registry.registerFunction(QStringLiteral("second"), QString(), QJsonObject {},
                              [](const QJsonObject &) { return QString(); });

    const QStringList names = registry.toolNames();
    QCOMPARE(names, (QStringList {QStringLiteral("first"), QStringLiteral("second")}));
    QCOMPARE(registry.tools().size(), 2);
    QCOMPARE(registry.tools().first().function().name(), QStringLiteral("first"));
}

void TestToolRegistry::invokeAllReturnsOnePerCall()
{
    ToolRegistry registry;
    registry.registerFunction(
            QStringLiteral("echo"), QString(), QJsonObject {},
            [](const QJsonObject &args) { return args.value(QStringLiteral("v")).toString(); });

    const QList<ToolCall> calls {
            makeCall(QStringLiteral("a"), QStringLiteral("echo"), QStringLiteral("{\"v\":\"x\"}")),
            makeCall(QStringLiteral("b"), QStringLiteral("echo"), QStringLiteral("{\"v\":\"y\"}")),
    };
    const QList<Message> results = registry.invokeAll(calls);
    QCOMPARE(results.size(), 2);
    QCOMPARE(results.at(0).content(), QStringLiteral("x"));
    QCOMPARE(results.at(1).toolCallId(), QStringLiteral("b"));
}

void TestToolRegistry::inferredRegistrationDerivesTheToolDefinition()
{
    // Nothing about the tool is written twice: name, schema and description all
    // come from the method, so they cannot drift from it (#39).
    ToolRegistry registry;
    WeatherProvider provider;
    QVERIFY(registry.registerMethod(&provider, QStringLiteral("forecast")));

    const Tool tool = registry.tools().first();
    QCOMPARE(tool.function().name(), QStringLiteral("forecast"));
    QCOMPARE(tool.function().description(), QStringLiteral("Weather for a city"));

    const QJsonObject parameters = tool.function().parameters();
    // The description moved onto the function, so the model is not told twice.
    QVERIFY(!parameters.contains(QStringLiteral("description")));
    const QJsonObject properties = parameters.value(QStringLiteral("properties")).toObject();
    QCOMPARE(properties.value(QStringLiteral("location"))
                     .toObject()
                     .value(QStringLiteral("type"))
                     .toString(),
             QStringLiteral("string"));
    QCOMPARE(properties.value(QStringLiteral("days"))
                     .toObject()
                     .value(QStringLiteral("type"))
                     .toString(),
             QStringLiteral("integer"));
    QCOMPARE(properties.value(QStringLiteral("location"))
                     .toObject()
                     .value(QStringLiteral("description"))
                     .toString(),
             QStringLiteral("City name"));
}

void TestToolRegistry::typedParametersAreFilledFromTheArguments()
{
    ToolRegistry registry;
    WeatherProvider provider;
    QVERIFY(registry.registerMethod(&provider, QStringLiteral("forecast")));

    const Message result
            = registry.invoke(makeCall(QStringLiteral("c4"), QStringLiteral("forecast"),
                                       QStringLiteral("{\"location\":\"Berlin\",\"days\":3}")));

    QCOMPARE(provider.lastLocation, QStringLiteral("Berlin"));
    QCOMPARE(provider.lastDays, 3);
    QCOMPARE(result.content(), QStringLiteral("clear in Berlin for 3 days"));
}

void TestToolRegistry::enumArgumentsArriveAsTheirKey()
{
    // MetaSchema advertises an enum as the set of its keys, so the model sends
    // a key and dispatch has to turn it back into the enumerator.
    ToolRegistry registry;
    WeatherProvider provider;
    QVERIFY(registry.registerMethod(&provider, QStringLiteral("convert")));

    const QJsonObject unit = registry.tools()
                                     .first()
                                     .function()
                                     .parameters()
                                     .value(QStringLiteral("properties"))
                                     .toObject()
                                     .value(QStringLiteral("unit"))
                                     .toObject();
    QCOMPARE(unit.value(QStringLiteral("type")).toString(), QStringLiteral("string"));

    const Message result
            = registry.invoke(makeCall(QStringLiteral("c5"), QStringLiteral("convert"),
                                       QStringLiteral("{\"degrees\":10,\"unit\":\"Fahrenheit\"}")));

    QCOMPARE(provider.lastUnit, WeatherProvider::Unit::Fahrenheit);
    QCOMPARE(result.content(), QStringLiteral("50"));
}

void TestToolRegistry::structuredReturnValuesAreSerialised()
{
    // A method answering with JSON should not have to serialise it by hand.
    ToolRegistry registry;
    WeatherProvider provider;
    QVERIFY(registry.registerMethod(&provider, QStringLiteral("conditions")));

    const Message result
            = registry.invoke(makeCall(QStringLiteral("c6"), QStringLiteral("conditions"),
                                       QStringLiteral("{\"location\":\"Berlin\"}")));

    const QJsonObject payload = QJsonDocument::fromJson(result.content().toUtf8()).object();
    QCOMPARE(payload.value(QStringLiteral("location")).toString(), QStringLiteral("Berlin"));
    QCOMPARE(payload.value(QStringLiteral("sky")).toString(), QStringLiteral("clear"));
}

void TestToolRegistry::missingArgumentsAreReported()
{
    // Without validation the dispatcher is the last line of defence: a missing
    // argument must fail loudly rather than pass a silent zero.
    ToolRegistry registry;
    WeatherProvider provider;
    QVERIFY(registry.registerMethod(&provider, QStringLiteral("forecast")));

    QSignalSpy failedSpy(&registry, &ToolRegistry::toolFailed);
    const Message result
            = registry.invoke(makeCall(QStringLiteral("c7"), QStringLiteral("forecast"),
                                       QStringLiteral("{\"location\":\"Berlin\"}")));

    QCOMPARE(failedSpy.count(), 1);
    const QJsonObject payload = QJsonDocument::fromJson(result.content().toUtf8()).object();
    QVERIFY(payload.value(QStringLiteral("error")).toString().contains(QStringLiteral("days")));
    QCOMPARE(provider.lastDays, 0);
}

void TestToolRegistry::validationRejectsArgumentsBeforeDispatch()
{
    // The point of validation is that the model gets told what to fix (#41).
    ToolRegistry registry;
    WeatherProvider provider;
    QVERIFY(registry.registerMethod(&provider, QStringLiteral("forecast")));
    registry.setValidateArguments(true);
    QVERIFY(registry.validatesArguments());

    QSignalSpy rejectedSpy(&registry, &ToolRegistry::argumentsRejected);
    QSignalSpy invokedSpy(&registry, &ToolRegistry::toolInvoked);

    const Message result
            = registry.invoke(makeCall(QStringLiteral("c8"), QStringLiteral("forecast"),
                                       QStringLiteral("{\"location\":\"Berlin\",\"days\":\"3\"}")));

    QCOMPARE(rejectedSpy.count(), 1);
    QCOMPARE(invokedSpy.count(), 0);
    // The handler never ran, so nothing half-applied.
    QCOMPARE(provider.lastLocation, QString());

    const QJsonObject payload = QJsonDocument::fromJson(result.content().toUtf8()).object();
    const QJsonArray details = payload.value(QStringLiteral("details")).toArray();
    QCOMPARE(details.size(), 1);
    QCOMPARE(details.first().toString(), QStringLiteral("/days: expected integer, got string"));

    // Well-formed arguments still pass straight through.
    const Message good
            = registry.invoke(makeCall(QStringLiteral("c9"), QStringLiteral("forecast"),
                                       QStringLiteral("{\"location\":\"Berlin\",\"days\":3}")));
    QCOMPARE(good.content(), QStringLiteral("clear in Berlin for 3 days"));
    QCOMPARE(rejectedSpy.count(), 1);
}

void TestToolRegistry::validationIsOffByDefault()
{
    // Opt-in, so a registry with a hand-written or absent schema keeps working.
    ToolRegistry registry;
    QVERIFY(!registry.validatesArguments());

    const QJsonObject schema {
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("required"), QJsonArray {QStringLiteral("a")}},
    };
    registry.registerFunction(QStringLiteral("lenient"), QString(), schema,
                              [](const QJsonObject &) { return QStringLiteral("ran"); });

    QCOMPARE(registry.invoke(makeCall(QStringLiteral("c10"), QStringLiteral("lenient"),
                                      QStringLiteral("{}")))
                     .content(),
             QStringLiteral("ran"));
}

QTEST_MAIN(TestToolRegistry)
#include "tst_toolregistry.moc"
