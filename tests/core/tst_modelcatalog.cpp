// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/ModelCatalog.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the model capability and pricing catalog (#45). The table itself
// is a snapshot and will age; what these pin is the behaviour around it, which
// is what callers depend on -- lookup that never fails, and a table that can be
// corrected without a release.
class TestModelCatalog : public QObject
{
    Q_OBJECT
private slots:
    void looksUpAKnownModel();
    void resolvesADatedVariantToItsFamily();
    void unknownModelsYieldAUsableEntryMarkedAsAGuess();
    void reportsCapabilities();
    void mergeAddsAndOverridesEntries();
    void roundTripsThroughJson();
    void theSharedCatalogIsReplaceable();
    void theFallbackIsReplaceable();
};

void TestModelCatalog::looksUpAKnownModel()
{
    const ModelInfo info = ModelCatalog::defaults().model(QStringLiteral("gpt-4o-mini"));

    QVERIFY(info.isKnown());
    QCOMPARE(info.id(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(info.contextWindow(), 128000);
    QCOMPARE(info.encoding(), QStringLiteral("o200k_base"));
    QVERIFY(info.inputPrice() > 0);
    QVERIFY(info.outputPrice() > info.inputPrice());
}

void TestModelCatalog::resolvesADatedVariantToItsFamily()
{
    const ModelCatalog catalog = ModelCatalog::defaults();

    // Model ids are versioned by suffix, and a caller pinning a date should not
    // lose every fact about the model as a result.
    const ModelInfo dated = catalog.model(QStringLiteral("gpt-4o-mini-2024-07-18"));
    QVERIFY(dated.isKnown());
    QCOMPARE(dated.contextWindow(), 128000);
    // The answer is about the id that was asked for.
    QCOMPARE(dated.id(), QStringLiteral("gpt-4o-mini-2024-07-18"));

    // The longest matching prefix wins, so a -mini variant does not resolve to
    // its larger sibling.
    QCOMPARE(catalog.model(QStringLiteral("gpt-4.1-mini-2025-04-14")).inputPrice(),
             catalog.model(QStringLiteral("gpt-4.1-mini")).inputPrice());
}

void TestModelCatalog::unknownModelsYieldAUsableEntryMarkedAsAGuess()
{
    const ModelInfo info = ModelCatalog::defaults().model(QStringLiteral("llama-3.1-70b"));

    // Never fails, so no caller has to guard the lookup ...
    QCOMPARE(info.id(), QStringLiteral("llama-3.1-70b"));
    QVERIFY(info.contextWindow() > 0);
    QVERIFY(!info.encoding().isEmpty());
    // ... but says it is guessing, so one that cares can tell.
    QVERIFY(!info.isKnown());
    // No price is claimed for a model nothing is known about.
    QCOMPARE(info.inputPrice(), 0.0);
}

void TestModelCatalog::reportsCapabilities()
{
    const ModelCatalog catalog = ModelCatalog::defaults();

    QVERIFY(catalog.model(QStringLiteral("gpt-4o")).supports(ModelCapability::Vision));
    QVERIFY(catalog.model(QStringLiteral("gpt-4o"))
                    .supports(ModelCapability::Tools | ModelCapability::StructuredOutputs));
    // An embedding model does none of it.
    QVERIFY(!catalog.model(QStringLiteral("text-embedding-3-small"))
                     .supports(ModelCapability::Tools));
}

void TestModelCatalog::mergeAddsAndOverridesEntries()
{
    ModelCatalog catalog = ModelCatalog::defaults();
    const int before = catalog.count();

    // A file only has to carry what changed: the price that moved, and the
    // model that did not exist when this library was released.
    const QJsonObject update = QJsonDocument::fromJson(R"({
        "gpt-4o-mini": {"input_price": 0.10, "output_price": 0.50,
                        "context_window": 128000, "encoding": "o200k_base",
                        "capabilities": ["tools", "vision"]},
        "house-model": {"context_window": 32000, "encoding": "cl100k_base",
                        "capabilities": ["tools"]}
    })")
                                       .object();
    catalog.merge(update);

    QCOMPARE(catalog.count(), before + 1);
    QCOMPARE(catalog.model(QStringLiteral("gpt-4o-mini")).inputPrice(), 0.10);
    QVERIFY(catalog.model(QStringLiteral("house-model")).isKnown());
    QCOMPARE(catalog.model(QStringLiteral("house-model")).contextWindow(), 32000);
    // The key is the id, so the file need not repeat it inside the entry.
    QCOMPARE(catalog.model(QStringLiteral("house-model")).id(), QStringLiteral("house-model"));
    // Entries the update did not mention are untouched.
    QCOMPARE(catalog.model(QStringLiteral("gpt-4o")).contextWindow(), 128000);
}

void TestModelCatalog::roundTripsThroughJson()
{
    const ModelCatalog original = ModelCatalog::defaults();

    ModelCatalog restored;
    restored.merge(original.toJson());

    QCOMPARE(restored.count(), original.count());
    QCOMPARE(restored.model(QStringLiteral("gpt-4o")), original.model(QStringLiteral("gpt-4o")));
    QCOMPARE(restored, original);
}

void TestModelCatalog::theSharedCatalogIsReplaceable()
{
    const ModelCatalog saved = ModelCatalog::shared();

    ModelInfo custom(QStringLiteral("gpt-4o"));
    custom.setContextWindow(42);
    ModelCatalog::shared().insert(custom);

    QCOMPARE(ModelCatalog::shared().model(QStringLiteral("gpt-4o")).contextWindow(), 42);

    ModelCatalog::shared() = saved;
    QCOMPARE(ModelCatalog::shared().model(QStringLiteral("gpt-4o")).contextWindow(), 128000);
}

void TestModelCatalog::theFallbackIsReplaceable()
{
    // What "unknown model" should mean is the application's decision, not this
    // library's.
    ModelCatalog catalog;
    ModelInfo fallback;
    fallback.setKnown(false);
    fallback.setContextWindow(4096);
    fallback.setEncoding(QStringLiteral("cl100k_base"));
    catalog.setFallback(fallback);

    const ModelInfo info = catalog.model(QStringLiteral("anything"));
    QCOMPARE(info.contextWindow(), 4096);
    QCOMPARE(info.encoding(), QStringLiteral("cl100k_base"));
    QCOMPARE(info.id(), QStringLiteral("anything"));
}

QTEST_MAIN(TestModelCatalog)
#include "tst_modelcatalog.moc"
