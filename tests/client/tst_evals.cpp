// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

// Offline stub-server coverage for the Evals endpoints (#22): eval CRUD, the
// run lifecycle nested below it, the per-item output, and the
// poll-until-terminal helper for a run.
class TestEvalsClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsJsonBody();
    void listSendsPaginationQuery();
    void getParsesEval();
    void updatePostsChangedFields();
    void deleteIssuesDeleteVerb();
    void createsRunBelowEval();
    void listsRuns();
    void getParsesRun();
    void cancelPostsToRunPath();
    void deleteRunIssuesDeleteVerb();
    void listsOutputItems();
    void getsOutputItem();
    void pollsRunUntilCompleted();
};

void TestEvalsClient::createPostsJsonBody()
{
    StubServer server(QByteArray(R"({"id":"eval_1","object":"eval","name":"Accuracy"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const QJsonObject config {{QStringLiteral("type"), QStringLiteral("custom")}};
    const QJsonArray criteria {
            QJsonObject {{QStringLiteral("type"), QStringLiteral("string_check")}}};
    CreateEvalRequest request(config, criteria);
    request.setName(QStringLiteral("Accuracy"));

    EvalReply *reply = client.createEval(request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/evals "));
    QVERIFY(server.requestBody().contains("\"name\":\"Accuracy\""));
    QVERIFY(server.requestBody().contains("\"data_source_config\":{\"type\":\"custom\"}"));
    QCOMPARE(reply->eval().id(), QStringLiteral("eval_1"));
    delete reply;
}

void TestEvalsClient::listSendsPaginationQuery()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"eval_1"},{"id":"eval_2"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 5;
    EvalListReply *reply = client.listEvals(params);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/evals?"));
    QVERIFY(server.requestLine().contains("limit=5"));
    QCOMPARE(reply->list().size(), 2);
    delete reply;
}

void TestEvalsClient::getParsesEval()
{
    StubServer server(QByteArray(R"({"id":"eval_1","name":"Accuracy","created_at":1716028800,)"
                                 R"("testing_criteria":[{"type":"string_check"}]})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    EvalReply *reply = client.getEval(QStringLiteral("eval_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/evals/eval_1 "));
    QCOMPARE(reply->eval().createdAt(), Q_INT64_C(1716028800));
    QCOMPARE(reply->eval().testingCriteria().size(), 1);
    delete reply;
}

void TestEvalsClient::updatePostsChangedFields()
{
    StubServer server(QByteArray(R"({"id":"eval_1","name":"Renamed"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    EvalReply *reply
            = client.updateEval(QStringLiteral("eval_1"), QStringLiteral("Renamed"),
                                QJsonObject {{QStringLiteral("team"), QStringLiteral("qa")}});
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/evals/eval_1 "));
    QVERIFY(server.requestBody().contains("\"name\":\"Renamed\""));
    QVERIFY(server.requestBody().contains("\"team\":\"qa\""));
    QCOMPARE(reply->eval().name(), QStringLiteral("Renamed"));
    delete reply;
}

void TestEvalsClient::deleteIssuesDeleteVerb()
{
    StubServer server(QByteArray(R"({"object":"eval.deleted","deleted":true,)"
                                 R"("eval_id":"eval_1"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    EvalReply *reply = client.deleteEval(QStringLiteral("eval_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/evals/eval_1 "));
    QCOMPARE(reply->eval().object(), QStringLiteral("eval.deleted"));
    QCOMPARE(reply->eval().id(), QStringLiteral("eval_1"));
    delete reply;
}

void TestEvalsClient::createsRunBelowEval()
{
    StubServer server(QByteArray(R"({"id":"evalrun_1","object":"eval.run",)"
                                 R"("eval_id":"eval_1","status":"queued"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateEvalRunRequest request(
            QJsonObject {{QStringLiteral("type"), QStringLiteral("completions")}});
    request.setName(QStringLiteral("nightly"));

    EvalRunReply *reply = client.createEvalRun(QStringLiteral("eval_1"), request);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/evals/eval_1/runs "));
    QVERIFY(server.requestBody().contains("\"name\":\"nightly\""));
    QCOMPARE(reply->run().status(), EvalRunStatus::Queued);
    delete reply;
}

void TestEvalsClient::listsRuns()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"evalrun_1"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 3;
    EvalRunListReply *reply = client.listEvalRuns(QStringLiteral("eval_1"), params);
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/evals/eval_1/runs?"));
    QVERIFY(server.requestLine().contains("limit=3"));
    QCOMPARE(reply->list().size(), 1);
    delete reply;
}

void TestEvalsClient::getParsesRun()
{
    StubServer server(QByteArray(R"({"id":"evalrun_1","status":"completed",)"
                                 R"("result_counts":{"total":4,"errored":0,)"
                                 R"("failed":1,"passed":3}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    EvalRunReply *reply = client.getEvalRun(QStringLiteral("eval_1"), QStringLiteral("evalrun_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/evals/eval_1/runs/evalrun_1 "));
    QCOMPARE(reply->run().resultCounts().passed, 3);
    QVERIFY(reply->run().isTerminal());
    delete reply;
}

void TestEvalsClient::cancelPostsToRunPath()
{
    // Cancelling is a POST to the run itself, not to a /cancel sub-path.
    StubServer server(QByteArray(R"({"id":"evalrun_1","status":"canceled"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    EvalRunReply *reply
            = client.cancelEvalRun(QStringLiteral("eval_1"), QStringLiteral("evalrun_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/evals/eval_1/runs/evalrun_1 "));
    QCOMPARE(reply->run().status(), EvalRunStatus::Canceled);
    delete reply;
}

void TestEvalsClient::deleteRunIssuesDeleteVerb()
{
    StubServer server(QByteArray(R"({"object":"eval.run.deleted","deleted":true,)"
                                 R"("run_id":"evalrun_1"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    EvalRunReply *reply
            = client.deleteEvalRun(QStringLiteral("eval_1"), QStringLiteral("evalrun_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("DELETE /v1/evals/eval_1/runs/evalrun_1 "));
    QCOMPARE(reply->run().id(), QStringLiteral("evalrun_1"));
    delete reply;
}

void TestEvalsClient::listsOutputItems()
{
    StubServer server(QByteArray(R"({"object":"list","data":[)"
                                 R"({"id":"outputitem_1","status":"pass"},)"
                                 R"({"id":"outputitem_2","status":"fail"}],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    EvalRunOutputItemListReply *reply
            = client.listEvalRunOutputItems(QStringLiteral("eval_1"), QStringLiteral("evalrun_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/evals/eval_1/runs/evalrun_1/output_items"));
    QCOMPARE(reply->list().size(), 2);
    QCOMPARE(reply->list().data.last().status(), QStringLiteral("fail"));
    delete reply;
}

void TestEvalsClient::getsOutputItem()
{
    StubServer server(QByteArray(R"({"id":"outputitem_1","status":"pass",)"
                                 R"("results":[{"passed":true}]})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    EvalRunOutputItemReply *reply = client.getEvalRunOutputItem(
            QStringLiteral("eval_1"), QStringLiteral("evalrun_1"), QStringLiteral("outputitem_1"));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith(
            "GET /v1/evals/eval_1/runs/evalrun_1/output_items/outputitem_1 "));
    QCOMPARE(reply->item().results().size(), 1);
    delete reply;
}

void TestEvalsClient::pollsRunUntilCompleted()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"evalrun_1","status":"queued"})"},
            {R"({"id":"evalrun_1","status":"in_progress",)"
             R"("result_counts":{"total":2,"passed":1}})"},
            {R"({"id":"evalrun_1","status":"completed",)"
             R"("result_counts":{"total":2,"passed":2}})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    EvalRunPoller *poller
            = client.pollEvalRun(QStringLiteral("eval_1"), QStringLiteral("evalrun_1"), 10);
    poller->setAutoDelete(false);

    QList<EvalRunStatus> observed;
    connect(poller, &EvalRunPoller::progressed, this,
            [&observed](const EvalRun &run) { observed.append(run.status()); });
    QSignalSpy completedSpy(poller, &EvalRunPoller::completed);

    poller->start();
    QVERIFY(completedSpy.wait(5000));

    QCOMPARE(completedSpy.count(), 1);
    QVERIFY(poller->isFinished());
    QCOMPARE(poller->run().status(), EvalRunStatus::Completed);
    QCOMPARE(poller->run().resultCounts().passed, 2);
    QCOMPARE(observed.size(), 3);
    QCOMPARE(observed.first(), EvalRunStatus::Queued);
    // Polling a run needs both ids, so the poller keeps the eval id too.
    QCOMPARE(poller->evalId(), QStringLiteral("eval_1"));
    QVERIFY(server.requestLines().first().startsWith("GET /v1/evals/eval_1/runs/evalrun_1 "));
    delete poller;
}

QTEST_MAIN(TestEvalsClient)
#include "tst_evals.moc"
