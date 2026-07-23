// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/Model.h>

#include <QtCore/QJsonDocument>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// JSON round-trip coverage for the Models value types.
class TestModels : public QObject
{
    Q_OBJECT
private slots:
    void modelRoundTrip();
    void modelListRoundTrip();
    void parsesModelList();
    void valueSemantics();
};

void TestModels::modelRoundTrip()
{
    Model model;
    model.setId(QStringLiteral("gpt-4o"));
    model.setCreated(1700000000);
    model.setOwnedBy(QStringLiteral("openai"));

    const Model parsed = Model::fromJson(model.toJson());
    QCOMPARE(parsed, model);
    QCOMPARE(parsed.object(), QStringLiteral("model"));
}

void TestModels::modelListRoundTrip()
{
    Model a;
    a.setId(QStringLiteral("gpt-4o"));
    Model b;
    b.setId(QStringLiteral("gpt-4o-mini"));

    ModelList list;
    list.data = {a, b};

    const ModelList parsed = ModelList::fromJson(list.toJson());
    QCOMPARE(parsed, list);
    QCOMPARE(parsed.size(), 2);
}

void TestModels::parsesModelList()
{
    const QByteArray body = R"({
        "object": "list",
        "data": [
            {"id": "gpt-4o", "object": "model", "created": 1, "owned_by": "openai"},
            {"id": "gpt-4o-mini", "object": "model", "created": 2, "owned_by": "openai"}
        ]
    })";
    const ModelList list = ModelList::fromJson(QJsonDocument::fromJson(body).object());
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.data.first().id(), QStringLiteral("gpt-4o"));
    QCOMPARE(list.data.at(1).ownedBy(), QStringLiteral("openai"));
}

void TestModels::valueSemantics()
{
    Model a;
    a.setId(QStringLiteral("m1"));
    Model b = a;
    QCOMPARE(a, b);
    b.setId(QStringLiteral("m2"));
    QVERIFY(a != b);
    QCOMPARE(a.id(), QStringLiteral("m1"));
}

QTEST_APPLESS_MAIN(TestModels)
#include "tst_models.moc"
