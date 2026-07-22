// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/ToolRegistry.h>

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
public:
    using QObject::QObject;

    Q_INVOKABLE QString getWeather(const QJsonObject &args)
    {
        lastLocation = args.value(QStringLiteral("location")).toString();
        return QStringLiteral("sunny in %1").arg(lastLocation);
    }

    QString lastLocation;
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

QTEST_MAIN(TestToolRegistry)
#include "tst_toolregistry.moc"
