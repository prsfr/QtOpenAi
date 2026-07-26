// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/CreateEvalRequest.h>
#include <QtOpenAi/Core/Eval.h>
#include <QtOpenAi/Core/EvalRun.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Evals types (#22): the eval definition (whose data-source
// config and grader list are pass-through JSON), the run with its result counts
// and terminal-state classification, the per-item output, the two deletion
// acknowledgements that name their id differently, and the request bodies.
class TestEvals : public QObject
{
    Q_OBJECT
private slots:
    void parsesEval();
    void evalRoundTrip();
    void parsesDeletionAcknowledgement();
    void parsesEvalRun();
    void evalRunRoundTrip();
    void parsesRunDeletionAcknowledgement();
    void reportsTerminalStatus_data();
    void reportsTerminalStatus();
    void parsesOutputItem();
    void createEvalRequestSerialisesBody();
    void createEvalRunRequestSerialisesBody();
    void createRequestsOmitUnsetFields();
};

namespace {

QJsonObject sampleDataSourceConfig()
{
    return QJsonObject {
            {QStringLiteral("type"), QStringLiteral("custom")},
            {QStringLiteral("item_schema"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("object")}}},
    };
}

QJsonArray sampleTestingCriteria()
{
    return QJsonArray {QJsonObject {
            {QStringLiteral("type"), QStringLiteral("string_check")},
            {QStringLiteral("name"), QStringLiteral("exact match")},
            {QStringLiteral("operation"), QStringLiteral("eq")},
    }};
}

} // namespace

void TestEvals::parsesEval()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("eval_abc123")},
            {QStringLiteral("object"), QStringLiteral("eval")},
            {QStringLiteral("name"), QStringLiteral("Sentiment accuracy")},
            {QStringLiteral("created_at"), 1716028800},
            {QStringLiteral("data_source_config"), sampleDataSourceConfig()},
            {QStringLiteral("testing_criteria"), sampleTestingCriteria()},
            {QStringLiteral("metadata"),
             QJsonObject {{QStringLiteral("team"), QStringLiteral("qa")}}},
    };

    const Eval eval = Eval::fromJson(json);
    QCOMPARE(eval.id(), QStringLiteral("eval_abc123"));
    QCOMPARE(eval.object(), QStringLiteral("eval"));
    QCOMPARE(eval.name(), QStringLiteral("Sentiment accuracy"));
    QCOMPARE(eval.createdAt(), Q_INT64_C(1716028800));
    // The config and the grader list are open unions, kept as raw JSON.
    QCOMPARE(eval.dataSourceConfig().value(QStringLiteral("type")).toString(),
             QStringLiteral("custom"));
    QCOMPARE(eval.testingCriteria().size(), 1);
    QCOMPARE(eval.testingCriteria().first().toObject().value(QStringLiteral("name")).toString(),
             QStringLiteral("exact match"));
    QCOMPARE(eval.metadata().value(QStringLiteral("team")).toString(), QStringLiteral("qa"));
}

void TestEvals::evalRoundTrip()
{
    Eval eval;
    eval.setId(QStringLiteral("eval_1"));
    eval.setObject(QStringLiteral("eval"));
    eval.setName(QStringLiteral("My eval"));
    eval.setCreatedAt(1700000000);
    eval.setDataSourceConfig(sampleDataSourceConfig());
    eval.setTestingCriteria(sampleTestingCriteria());
    eval.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});

    QCOMPARE(Eval::fromJson(eval.toJson()), eval);
}

void TestEvals::parsesDeletionAcknowledgement()
{
    // DELETE /evals/{id} answers with `eval_id` rather than `id`, so the id is
    // read from either spelling.
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("eval.deleted")},
            {QStringLiteral("deleted"), true},
            {QStringLiteral("eval_id"), QStringLiteral("eval_abc123")},
    };

    const Eval eval = Eval::fromJson(json);
    QCOMPARE(eval.id(), QStringLiteral("eval_abc123"));
    QCOMPARE(eval.object(), QStringLiteral("eval.deleted"));
}

void TestEvals::parsesEvalRun()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("evalrun_abc")},
            {QStringLiteral("object"), QStringLiteral("eval.run")},
            {QStringLiteral("eval_id"), QStringLiteral("eval_abc123")},
            {QStringLiteral("name"), QStringLiteral("nightly")},
            {QStringLiteral("created_at"), 1716028900},
            {QStringLiteral("status"), QStringLiteral("completed")},
            {QStringLiteral("model"), QStringLiteral("gpt-4o-mini")},
            {QStringLiteral("report_url"), QStringLiteral("https://platform.openai.com/x")},
            {QStringLiteral("result_counts"), QJsonObject {{QStringLiteral("total"), 10},
                                                           {QStringLiteral("errored"), 1},
                                                           {QStringLiteral("failed"), 2},
                                                           {QStringLiteral("passed"), 7}}},
            {QStringLiteral("data_source"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("completions")}}},
            {QStringLiteral("error"),
             QJsonObject {{QStringLiteral("code"), QStringLiteral("sample_error")},
                          {QStringLiteral("message"), QStringLiteral("one item errored")}}},
    };

    const EvalRun run = EvalRun::fromJson(json);
    QCOMPARE(run.id(), QStringLiteral("evalrun_abc"));
    QCOMPARE(run.evalId(), QStringLiteral("eval_abc123"));
    QCOMPARE(run.name(), QStringLiteral("nightly"));
    QCOMPARE(run.createdAt(), Q_INT64_C(1716028900));
    QCOMPARE(run.status(), EvalRunStatus::Completed);
    QCOMPARE(run.model(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(run.reportUrl(), QStringLiteral("https://platform.openai.com/x"));
    QCOMPARE(run.resultCounts().total, 10);
    QCOMPARE(run.resultCounts().errored, 1);
    QCOMPARE(run.resultCounts().failed, 2);
    QCOMPARE(run.resultCounts().passed, 7);
    QCOMPARE(run.dataSource().value(QStringLiteral("type")).toString(),
             QStringLiteral("completions"));
    QCOMPARE(run.errorCode(), QStringLiteral("sample_error"));
    QCOMPARE(run.errorMessage(), QStringLiteral("one item errored"));
    QVERIFY(run.isTerminal());
}

void TestEvals::evalRunRoundTrip()
{
    EvalRun run;
    run.setId(QStringLiteral("evalrun_1"));
    run.setObject(QStringLiteral("eval.run"));
    run.setEvalId(QStringLiteral("eval_1"));
    run.setName(QStringLiteral("run"));
    run.setCreatedAt(1700000000);
    run.setStatus(EvalRunStatus::Failed);
    run.setModel(QStringLiteral("gpt-4o-mini"));
    run.setReportUrl(QStringLiteral("https://example.invalid/report"));
    run.setResultCounts({4, 0, 1, 3});
    run.setDataSource(QJsonObject {{QStringLiteral("type"), QStringLiteral("completions")}});
    run.setErrorCode(QStringLiteral("boom"));
    run.setErrorMessage(QStringLiteral("it broke"));
    run.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});

    QCOMPARE(EvalRun::fromJson(run.toJson()), run);
}

void TestEvals::parsesRunDeletionAcknowledgement()
{
    // DELETE .../runs/{id} names the id `run_id`, mirroring the eval case.
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("eval.run.deleted")},
            {QStringLiteral("deleted"), true},
            {QStringLiteral("run_id"), QStringLiteral("evalrun_abc")},
    };

    const EvalRun run = EvalRun::fromJson(json);
    QCOMPARE(run.id(), QStringLiteral("evalrun_abc"));
    QCOMPARE(run.object(), QStringLiteral("eval.run.deleted"));
}

void TestEvals::reportsTerminalStatus_data()
{
    QTest::addColumn<QString>("wireStatus");
    QTest::addColumn<bool>("terminal");

    QTest::newRow("queued") << QStringLiteral("queued") << false;
    QTest::newRow("in_progress") << QStringLiteral("in_progress") << false;
    QTest::newRow("completed") << QStringLiteral("completed") << true;
    QTest::newRow("failed") << QStringLiteral("failed") << true;
    // The Evals API spells this one with a single "l".
    QTest::newRow("canceled") << QStringLiteral("canceled") << true;
    // An unknown value decodes to the initial state, so polling continues.
    QTest::newRow("unknown") << QStringLiteral("something_new") << false;
}

void TestEvals::reportsTerminalStatus()
{
    QFETCH(QString, wireStatus);
    QFETCH(bool, terminal);

    const EvalRun run = EvalRun::fromJson(QJsonObject {{QStringLiteral("status"), wireStatus}});
    QCOMPARE(run.isTerminal(), terminal);
}

void TestEvals::parsesOutputItem()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("outputitem_1")},
            {QStringLiteral("object"), QStringLiteral("eval.run.output_item")},
            {QStringLiteral("created_at"), 1716029000},
            {QStringLiteral("run_id"), QStringLiteral("evalrun_abc")},
            {QStringLiteral("eval_id"), QStringLiteral("eval_abc123")},
            {QStringLiteral("status"), QStringLiteral("pass")},
            {QStringLiteral("datasource_item_id"), 5},
            {QStringLiteral("datasource_item"),
             QJsonObject {{QStringLiteral("input"), QStringLiteral("hello")}}},
            {QStringLiteral("results"),
             QJsonArray {QJsonObject {{QStringLiteral("name"), QStringLiteral("exact match")},
                                      {QStringLiteral("passed"), true}}}},
            {QStringLiteral("sample"),
             QJsonObject {{QStringLiteral("model"), QStringLiteral("gpt-4o-mini")}}},
    };

    const EvalRunOutputItem item = EvalRunOutputItem::fromJson(json);
    QCOMPARE(item.id(), QStringLiteral("outputitem_1"));
    QCOMPARE(item.runId(), QStringLiteral("evalrun_abc"));
    QCOMPARE(item.evalId(), QStringLiteral("eval_abc123"));
    QCOMPARE(item.createdAt(), Q_INT64_C(1716029000));
    // Per-item pass/fail is a free-form string, not the run's status set.
    QCOMPARE(item.status(), QStringLiteral("pass"));
    QCOMPARE(item.datasourceItemId(), 5);
    QCOMPARE(item.datasourceItem().value(QStringLiteral("input")).toString(),
             QStringLiteral("hello"));
    QCOMPARE(item.results().size(), 1);
    QVERIFY(item.results().first().toObject().value(QStringLiteral("passed")).toBool());
    QCOMPARE(item.sample().value(QStringLiteral("model")).toString(),
             QStringLiteral("gpt-4o-mini"));
    QCOMPARE(EvalRunOutputItem::fromJson(item.toJson()), item);
}

void TestEvals::createEvalRequestSerialisesBody()
{
    CreateEvalRequest request(sampleDataSourceConfig(), sampleTestingCriteria());
    request.setName(QStringLiteral("Sentiment accuracy"));
    request.setMetadata(QJsonObject {{QStringLiteral("team"), QStringLiteral("qa")}});

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("name")).toString(), QStringLiteral("Sentiment accuracy"));
    QCOMPARE(json.value(QStringLiteral("data_source_config"))
                     .toObject()
                     .value(QStringLiteral("type"))
                     .toString(),
             QStringLiteral("custom"));
    QCOMPARE(json.value(QStringLiteral("testing_criteria")).toArray().size(), 1);
    QCOMPARE(json.value(QStringLiteral("metadata"))
                     .toObject()
                     .value(QStringLiteral("team"))
                     .toString(),
             QStringLiteral("qa"));
}

void TestEvals::createEvalRunRequestSerialisesBody()
{
    const QJsonObject dataSource {
            {QStringLiteral("type"), QStringLiteral("completions")},
            {QStringLiteral("model"), QStringLiteral("gpt-4o-mini")},
    };
    CreateEvalRunRequest request(dataSource);
    request.setName(QStringLiteral("nightly"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("name")).toString(), QStringLiteral("nightly"));
    QCOMPARE(json.value(QStringLiteral("data_source"))
                     .toObject()
                     .value(QStringLiteral("model"))
                     .toString(),
             QStringLiteral("gpt-4o-mini"));
}

void TestEvals::createRequestsOmitUnsetFields()
{
    const CreateEvalRequest evalRequest(sampleDataSourceConfig(), sampleTestingCriteria());
    const QJsonObject evalJson = evalRequest.toJson();
    QCOMPARE(evalJson.size(), 2);
    QVERIFY(!evalJson.contains(QStringLiteral("name")));
    QVERIFY(!evalJson.contains(QStringLiteral("metadata")));

    const CreateEvalRunRequest runRequest(
            QJsonObject {{QStringLiteral("type"), QStringLiteral("completions")}});
    const QJsonObject runJson = runRequest.toJson();
    QCOMPARE(runJson.size(), 1);
    QVERIFY(!runJson.contains(QStringLiteral("name")));
}

QTEST_MAIN(TestEvals)
#include "tst_evals.moc"
