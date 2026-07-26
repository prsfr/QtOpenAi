// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

// Offline stub-server coverage for the Fine-tuning endpoints (#21): the job
// lifecycle (create/list/get/cancel/pause/resume), the events and checkpoints
// sub-resources, checkpoint permissions, and the poll-until-terminal helper.
class TestFineTuningClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsJsonBody();
    void listSendsPaginationQuery();
    void getParsesJob();
    void cancelPauseResumeUseTheirPaths_data();
    void cancelPauseResumeUseTheirPaths();
    void listsEvents();
    void listsCheckpoints();
    void listsCheckpointPermissions();
    void createsCheckpointPermissions();
    void deletesCheckpointPermission();
    void pollsUntilSucceeded();
};

void TestFineTuningClient::createPostsJsonBody()
{
    StubServer server(QByteArray(R"({"id":"ftjob-1","object":"fine_tuning.job",)"
                                 R"("status":"queued","model":"gpt-4o-mini",)"
                                 R"("training_file":"file-train"})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    CreateFineTuningJobRequest request(QStringLiteral("gpt-4o-mini"), QStringLiteral("file-train"));
    request.setSuffix(QStringLiteral("my-run"));

    const auto reply = awaited(client.createFineTuningJob(request));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/fine_tuning/jobs "));
    QVERIFY(server.requestBody().contains("\"training_file\":\"file-train\""));
    QVERIFY(server.requestBody().contains("\"suffix\":\"my-run\""));
    QCOMPARE(reply->job().id(), QStringLiteral("ftjob-1"));
    QCOMPARE(reply->job().status(), FineTuningJobStatus::Queued);
}

void TestFineTuningClient::listSendsPaginationQuery()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"ftjob-1"},{"id":"ftjob-2"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 5;
    const auto reply = awaited(client.listFineTuningJobs(params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/fine_tuning/jobs?"));
    QVERIFY(server.requestLine().contains("limit=5"));
    QCOMPARE(reply->list().size(), 2);
}

void TestFineTuningClient::getParsesJob()
{
    StubServer server(QByteArray(R"({"id":"ftjob-1","status":"succeeded",)"
                                 R"("fine_tuned_model":"ft:gpt-4o-mini:org::x",)"
                                 R"("trained_tokens":5768,"result_files":["file-r"]})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.getFineTuningJob(QStringLiteral("ftjob-1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/fine_tuning/jobs/ftjob-1 "));
    QCOMPARE(reply->job().fineTunedModel(), QStringLiteral("ft:gpt-4o-mini:org::x"));
    QCOMPARE(reply->job().trainedTokens(), Q_INT64_C(5768));
    QCOMPARE(reply->job().resultFiles(), QStringList {QStringLiteral("file-r")});
    QVERIFY(reply->job().isTerminal());
}

void TestFineTuningClient::cancelPauseResumeUseTheirPaths_data()
{
    QTest::addColumn<QString>("action");
    QTest::addColumn<QByteArray>("expectedLine");
    QTest::addColumn<QString>("wireStatus");

    QTest::newRow("cancel") << QStringLiteral("cancel")
                            << QByteArray("POST /v1/fine_tuning/jobs/ftjob-1/cancel ")
                            << QStringLiteral("cancelled");
    QTest::newRow("pause") << QStringLiteral("pause")
                           << QByteArray("POST /v1/fine_tuning/jobs/ftjob-1/pause ")
                           << QStringLiteral("paused");
    QTest::newRow("resume") << QStringLiteral("resume")
                            << QByteArray("POST /v1/fine_tuning/jobs/ftjob-1/resume ")
                            << QStringLiteral("running");
}

void TestFineTuningClient::cancelPauseResumeUseTheirPaths()
{
    QFETCH(QString, action);
    QFETCH(QByteArray, expectedLine);
    QFETCH(QString, wireStatus);

    StubServer server(QByteArray(R"({"id":"ftjob-1","status":")") + wireStatus.toUtf8() + "\"}");
    Client client(server.baseUrl(), QStringLiteral("k"));

    const QString jobId = QStringLiteral("ftjob-1");
    const auto reply = awaited(action == QLatin1String("cancel")
                                       ? client.cancelFineTuningJob(jobId)
                                       : (action == QLatin1String("pause")
                                                  ? client.pauseFineTuningJob(jobId)
                                                  : client.resumeFineTuningJob(jobId)));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith(expectedLine));
    QCOMPARE(reply->job().status(), fineTuningJobStatusFromString(wireStatus));
}

void TestFineTuningClient::listsEvents()
{
    StubServer server(QByteArray(R"({"object":"list","data":[)"
                                 R"({"id":"ft-event-1","level":"info","message":"created"},)"
                                 R"({"id":"ft-event-2","level":"info","message":"running"}],)"
                                 R"("has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 2;
    const auto reply = awaited(client.listFineTuningEvents(QStringLiteral("ftjob-1"), params));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/fine_tuning/jobs/ftjob-1/events?"));
    QVERIFY(server.requestLine().contains("limit=2"));
    QCOMPARE(reply->list().size(), 2);
    QCOMPARE(reply->list().data.first().message(), QStringLiteral("created"));
}

void TestFineTuningClient::listsCheckpoints()
{
    StubServer server(QByteArray(R"({"object":"list","data":[)"
                                 R"({"id":"ftckpt_1","step_number":88,)"
                                 R"("metrics":{"train_loss":0.4}}],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.listFineTuningCheckpoints(QStringLiteral("ftjob-1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("GET /v1/fine_tuning/jobs/ftjob-1/checkpoints"));
    QCOMPARE(reply->list().size(), 1);
    QCOMPARE(reply->list().data.first().stepNumber(), 88);
    QCOMPARE(reply->list().data.first().metrics().trainLoss, 0.4);
}

void TestFineTuningClient::listsCheckpointPermissions()
{
    StubServer server(QByteArray(R"({"object":"list","data":[)"
                                 R"({"id":"cp_1","project_id":"proj_a"}],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply
            = awaited(client.listFineTuningCheckpointPermissions(QStringLiteral("ftckpt_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith(
            "GET /v1/fine_tuning/checkpoints/ftckpt_1/permissions"));
    QCOMPARE(reply->list().data.first().projectId(), QStringLiteral("proj_a"));
}

void TestFineTuningClient::createsCheckpointPermissions()
{
    StubServer server(QByteArray(R"({"object":"list","data":[)"
                                 R"({"id":"cp_1","project_id":"proj_a"},)"
                                 R"({"id":"cp_2","project_id":"proj_b"}],"has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.createFineTuningCheckpointPermissions(
            QStringLiteral("ftckpt_1"), {QStringLiteral("proj_a"), QStringLiteral("proj_b")}));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith(
            "POST /v1/fine_tuning/checkpoints/ftckpt_1/permissions "));
    QVERIFY(server.requestBody().contains("\"project_ids\":[\"proj_a\",\"proj_b\"]"));
    QCOMPARE(reply->list().size(), 2);
}

void TestFineTuningClient::deletesCheckpointPermission()
{
    StubServer server(QByteArray(R"({"id":"cp_1","object":"checkpoint.permission.deleted",)"
                                 R"("deleted":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    const auto reply = awaited(client.deleteFineTuningCheckpointPermission(
            QStringLiteral("ftckpt_1"), QStringLiteral("cp_1")));
    QVERIFY(reply);

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith(
            "DELETE /v1/fine_tuning/checkpoints/ftckpt_1/permissions/cp_1 "));
    QCOMPARE(reply->permission().object(), QStringLiteral("checkpoint.permission.deleted"));
}

void TestFineTuningClient::pollsUntilSucceeded()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"id":"ftjob-1","status":"validating_files"})"},
            {R"({"id":"ftjob-1","status":"running"})"},
            {R"({"id":"ftjob-1","status":"succeeded",)"
             R"("fine_tuned_model":"ft:gpt-4o-mini:org::x"})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    FineTuningJobPoller *poller = client.pollFineTuningJob(QStringLiteral("ftjob-1"), 10);
    poller->setAutoDelete(false);

    QList<FineTuningJobStatus> observed;
    connect(poller, &FineTuningJobPoller::progressed, this,
            [&observed](const FineTuningJob &job) { observed.append(job.status()); });
    QSignalSpy completedSpy(poller, &FineTuningJobPoller::completed);

    poller->start();
    QVERIFY(completedSpy.wait(5000));

    QCOMPARE(completedSpy.count(), 1);
    QVERIFY(poller->isFinished());
    QCOMPARE(poller->job().status(), FineTuningJobStatus::Succeeded);
    QCOMPARE(poller->job().fineTunedModel(), QStringLiteral("ft:gpt-4o-mini:org::x"));
    QCOMPARE(observed.size(), 3);
    QCOMPARE(observed.first(), FineTuningJobStatus::ValidatingFiles);
    QVERIFY(server.requestLines().first().startsWith("GET /v1/fine_tuning/jobs/ftjob-1 "));
    delete poller;
}

QTEST_MAIN(TestFineTuningClient)
#include "tst_finetuning.moc"
