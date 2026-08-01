// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/MetaJson.h>
#include <QtOpenAi/Core/MetaSchema.h>
#include <QtOpenAi/Core/ResponseFormat.h>
#include <QtOpenAi/Core/SchemaValidator.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

namespace {

class Ingredient
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(double grams MEMBER grams)
public:
    QString name;
    double grams = 0;

    bool operator==(const Ingredient &other) const
    {
        return name == other.name && qFuzzyCompare(grams, other.grams);
    }
    bool operator!=(const Ingredient &other) const { return !(*this == other); }
};

// The type a model is asked to answer in: scalars, an array, an enum, and a
// nested object, which is every shape MetaSchema describes.
class Recipe
{
    Q_GADGET
    Q_CLASSINFO("doc", "A cooking recipe")
    Q_CLASSINFO("doc:minutes", "Total time in minutes")
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(int minutes MEMBER minutes)
    Q_PROPERTY(QStringList steps MEMBER steps)
    Q_PROPERTY(Difficulty difficulty MEMBER difficulty)
    Q_PROPERTY(Ingredient main MEMBER main)
public:
    enum class Difficulty {
        Easy,
        Hard,
    };
    Q_ENUM(Difficulty)

    QString title;
    int minutes = 0;
    QStringList steps;
    Difficulty difficulty = Difficulty::Easy;
    Ingredient main;
};

// A QObject answers through the same code path, minus objectName.
class Report : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString summary MEMBER summary)
    Q_PROPERTY(int score MEMBER score)
public:
    using QObject::QObject;

    QString summary;
    int score = 0;
};

const char kRecipeJson[] = R"({
    "title": "Pancakes",
    "minutes": 20,
    "steps": ["Mix", "Fry"],
    "difficulty": "Hard",
    "main": {"name": "Flour", "grams": 250}
})";

QJsonObject objectOf(const char *json)
{
    return QJsonDocument::fromJson(QByteArray(json)).object();
}

} // namespace

// Coverage for typed structured outputs (#40): a type describes itself to the
// model through MetaSchema, and the answer comes back into that same type.
class TestMetaJson : public QObject
{
    Q_OBJECT
private slots:
    void readsScalarsArraysEnumsAndNestedObjects();
    void leavesUnmentionedPropertiesAlone();
    void reportsValuesThatDoNotFitWithoutLosingTheRest();
    void writesTheInverseOfWhatItReads();
    void populatesAQObject();
    void buildsAResponseFormatFromAType();
    void roundTripsThroughTheSchemaItAdvertised();
};

void TestMetaJson::readsScalarsArraysEnumsAndNestedObjects()
{
    const Recipe recipe = MetaJson::fromJson<Recipe>(objectOf(kRecipeJson));

    QCOMPARE(recipe.title, QStringLiteral("Pancakes"));
    QCOMPARE(recipe.minutes, 20);
    QCOMPARE(recipe.steps, QStringList({QStringLiteral("Mix"), QStringLiteral("Fry")}));
    // The enum was advertised as its keys, so it arrives as one.
    QCOMPARE(recipe.difficulty, Recipe::Difficulty::Hard);
    QCOMPARE(recipe.main.name, QStringLiteral("Flour"));
    QCOMPARE(recipe.main.grams, 250.0);
}

void TestMetaJson::leavesUnmentionedPropertiesAlone()
{
    // A property the JSON does not mention keeps whatever the constructor gave
    // it, rather than being reset to something the model never said.
    Recipe recipe;
    recipe.title = QStringLiteral("Kept");
    QVERIFY(MetaJson::readInto(&Recipe::staticMetaObject, &recipe, objectOf(R"({"minutes":5})")));

    QCOMPARE(recipe.title, QStringLiteral("Kept"));
    QCOMPARE(recipe.minutes, 5);
}

void TestMetaJson::reportsValuesThatDoNotFitWithoutLosingTheRest()
{
    Recipe recipe;
    const bool complete = MetaJson::readInto(
            &Recipe::staticMetaObject, &recipe,
            objectOf(R"({"title":"Soup","minutes":{"nested":true},"difficulty":"Impossible"})"));

    QVERIFY(!complete);
    // One bad field does not cost the object: what fitted was still written.
    QCOMPARE(recipe.title, QStringLiteral("Soup"));
    QCOMPARE(recipe.minutes, 0);
    QCOMPARE(recipe.difficulty, Recipe::Difficulty::Easy);
}

void TestMetaJson::writesTheInverseOfWhatItReads()
{
    const Recipe recipe = MetaJson::fromJson<Recipe>(objectOf(kRecipeJson));
    const QJsonObject json = MetaJson::toJson(recipe);

    QCOMPARE(json.value(QStringLiteral("title")).toString(), QStringLiteral("Pancakes"));
    QCOMPARE(json.value(QStringLiteral("steps")).toArray().size(), 2);
    QCOMPARE(json.value(QStringLiteral("difficulty")).toString(), QStringLiteral("Hard"));
    QCOMPARE(json.value(QStringLiteral("main")).toObject().value(QStringLiteral("name")).toString(),
             QStringLiteral("Flour"));

    // Reading it back gives the same object, which is what makes the pair a
    // round trip rather than two mappings that happen to agree.
    const Recipe again = MetaJson::fromJson<Recipe>(json);
    QCOMPARE(again.title, recipe.title);
    QCOMPARE(again.steps, recipe.steps);
    QCOMPARE(again.difficulty, recipe.difficulty);
    QCOMPARE(again.main, recipe.main);
}

void TestMetaJson::populatesAQObject()
{
    Report report;
    QVERIFY(MetaJson::readInto(&report, objectOf(R"({"summary":"Fine","score":7})")));

    QCOMPARE(report.summary, QStringLiteral("Fine"));
    QCOMPARE(report.score, 7);
    // objectName is QObject's, not the model's business.
    QVERIFY(!MetaJson::write(&report).contains(QStringLiteral("objectName")));
}

void TestMetaJson::buildsAResponseFormatFromAType()
{
    const ResponseFormat format = ResponseFormat::forType<Recipe>();

    QCOMPARE(format.type(), QStringLiteral("json_schema"));
    // The API's name field has no room for a qualified C++ name.
    QCOMPARE(format.name(), QStringLiteral("Recipe"));
    QCOMPARE(format.description(), QStringLiteral("A cooking recipe"));
    QVERIFY(format.strict());

    // The class description moved into the format's own field, and the rest of
    // the schema is MetaSchema's, unaltered.
    QJsonObject expected = MetaSchema::fromType<Recipe>();
    QCOMPARE(expected.take(QStringLiteral("description")).toString(),
             QStringLiteral("A cooking recipe"));
    QVERIFY(!format.schema().contains(QStringLiteral("description")));
    QCOMPARE(format.schema(), expected);

    // The name is overridable, since it is what the API refers to the schema by.
    QCOMPARE(ResponseFormat::forType<Recipe>(QStringLiteral("dinner")).name(),
             QStringLiteral("dinner"));
}

void TestMetaJson::roundTripsThroughTheSchemaItAdvertised()
{
    // The whole point of the feature: one type, describing itself to the model
    // and reading the answer back, with the same schema on both sides.
    const ResponseFormat format = ResponseFormat::forType<Recipe>();
    const QJsonObject answer = objectOf(kRecipeJson);

    QCOMPARE(SchemaValidator::validate(format.schema(), answer), QStringList());

    const QString content = QString::fromUtf8(QJsonDocument(answer).toJson());
    const Recipe recipe = MetaJson::parse<Recipe>(content);
    QCOMPARE(recipe.title, QStringLiteral("Pancakes"));
    QCOMPARE(recipe.main.grams, 250.0);
}

QTEST_MAIN(TestMetaJson)
#include "tst_metajson.moc"
