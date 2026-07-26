// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/CreateFineTuningJobRequest.h>
#include <QtOpenAi/Core/FineTuningCheckpoint.h>
#include <QtOpenAi/Core/FineTuningJob.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Fine-tuning types (#21): job parsing/round-trip including
// the nested method/hyperparameters and error objects, the terminal-state
// classification the poller relies on, events, checkpoints and checkpoint
// permissions, plus the create-request body.
class TestFineTuning : public QObject
{
    Q_OBJECT
private slots:
    void parsesJob();
    void jobRoundTrip();
    void readsAutoHyperparametersAsUnset();
    void parsesJobError();
    void reportsTerminalStatus_data();
    void reportsTerminalStatus();
    void parsesEvent();
    void parsesCheckpoint();
    void parsesPermission();
    void createRequestSerialisesBody();
    void createRequestOmitsUnsetFields();
};

void TestFineTuning::parsesJob()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("ftjob-abc123")},
            {QStringLiteral("object"), QStringLiteral("fine_tuning.job")},
            {QStringLiteral("model"), QStringLiteral("gpt-4o-mini-2024-07-18")},
            {QStringLiteral("created_at"), 1721764800},
            {QStringLiteral("finished_at"), 1721765000},
            {QStringLiteral("fine_tuned_model"), QStringLiteral("ft:gpt-4o-mini:org::id")},
            {QStringLiteral("organization_id"), QStringLiteral("org-123")},
            {QStringLiteral("training_file"), QStringLiteral("file-train")},
            {QStringLiteral("validation_file"), QStringLiteral("file-valid")},
            {QStringLiteral("result_files"), QJsonArray {QStringLiteral("file-result")}},
            {QStringLiteral("status"), QStringLiteral("succeeded")},
            {QStringLiteral("trained_tokens"), 5768},
            {QStringLiteral("seed"), 42},
            {QStringLiteral("method"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("supervised")},
                          {QStringLiteral("supervised"),
                           QJsonObject {{QStringLiteral("hyperparameters"),
                                         QJsonObject {{QStringLiteral("n_epochs"), 4},
                                                      {QStringLiteral("batch_size"), 1},
                                                      {QStringLiteral("learning_rate_multiplier"),
                                                       1.8}}}}}}},
    };

    const FineTuningJob job = FineTuningJob::fromJson(json);
    QCOMPARE(job.id(), QStringLiteral("ftjob-abc123"));
    QCOMPARE(job.object(), QStringLiteral("fine_tuning.job"));
    QCOMPARE(job.model(), QStringLiteral("gpt-4o-mini-2024-07-18"));
    QCOMPARE(job.createdAt(), Q_INT64_C(1721764800));
    QCOMPARE(job.finishedAt(), Q_INT64_C(1721765000));
    QCOMPARE(job.fineTunedModel(), QStringLiteral("ft:gpt-4o-mini:org::id"));
    QCOMPARE(job.organizationId(), QStringLiteral("org-123"));
    QCOMPARE(job.trainingFile(), QStringLiteral("file-train"));
    QCOMPARE(job.validationFile(), QStringLiteral("file-valid"));
    QCOMPARE(job.resultFiles(), QStringList {QStringLiteral("file-result")});
    QCOMPARE(job.status(), FineTuningJobStatus::Succeeded);
    QCOMPARE(job.trainedTokens(), Q_INT64_C(5768));
    QCOMPARE(job.seed(), 42);
    QVERIFY(job.isTerminal());

    // The hyperparameters sit two levels down, under the method's own type.
    QCOMPARE(job.methodType(), QStringLiteral("supervised"));
    const FineTuningHyperparameters hyper = job.hyperparameters();
    QCOMPARE(hyper.nEpochs.value_or(0), 4);
    QCOMPARE(hyper.batchSize.value_or(0), 1);
    QCOMPARE(hyper.learningRateMultiplier.value_or(0.0), 1.8);
}

void TestFineTuning::jobRoundTrip()
{
    FineTuningJob job;
    job.setId(QStringLiteral("ftjob-1"));
    job.setObject(QStringLiteral("fine_tuning.job"));
    job.setModel(QStringLiteral("gpt-4o-mini"));
    job.setCreatedAt(1700000000);
    job.setFinishedAt(1700000900);
    job.setFineTunedModel(QStringLiteral("ft:gpt-4o-mini:org::x"));
    job.setOrganizationId(QStringLiteral("org-1"));
    job.setTrainingFile(QStringLiteral("file-train"));
    job.setValidationFile(QStringLiteral("file-valid"));
    job.setResultFiles({QStringLiteral("file-a"), QStringLiteral("file-b")});
    job.setStatus(FineTuningJobStatus::Paused);
    job.setTrainedTokens(1234);
    job.setSeed(7);
    job.setMethodType(QStringLiteral("dpo"));
    FineTuningHyperparameters hyper;
    hyper.nEpochs = 3;
    hyper.batchSize = 8;
    hyper.learningRateMultiplier = 0.5;
    job.setHyperparameters(hyper);
    job.setErrorCode(QStringLiteral("invalid_training_file"));
    job.setErrorMessage(QStringLiteral("bad line 3"));
    job.setErrorParam(QStringLiteral("training_file"));
    job.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});

    QCOMPARE(FineTuningJob::fromJson(job.toJson()), job);
}

void TestFineTuning::readsAutoHyperparametersAsUnset()
{
    // The API sends the string "auto" for hyperparameters it picks itself; those
    // map to an unset optional rather than a made-up number.
    const QJsonObject json {
            {QStringLiteral("method"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("supervised")},
                          {QStringLiteral("supervised"),
                           QJsonObject {{QStringLiteral("hyperparameters"),
                                         QJsonObject {{QStringLiteral("n_epochs"),
                                                       QStringLiteral("auto")},
                                                      {QStringLiteral("batch_size"), 4},
                                                      {QStringLiteral("learning_rate_multiplier"),
                                                       QStringLiteral("auto")}}}}}}},
    };

    const FineTuningHyperparameters hyper = FineTuningJob::fromJson(json).hyperparameters();
    QVERIFY(!hyper.nEpochs.has_value());
    QVERIFY(!hyper.learningRateMultiplier.has_value());
    QCOMPARE(hyper.batchSize.value_or(0), 4);
}

void TestFineTuning::parsesJobError()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("ftjob-1")},
            {QStringLiteral("status"), QStringLiteral("failed")},
            {QStringLiteral("error"),
             QJsonObject {{QStringLiteral("code"), QStringLiteral("invalid_training_file")},
                          {QStringLiteral("message"), QStringLiteral("line 3 is malformed")},
                          {QStringLiteral("param"), QStringLiteral("training_file")}}},
    };

    const FineTuningJob job = FineTuningJob::fromJson(json);
    QCOMPARE(job.status(), FineTuningJobStatus::Failed);
    QCOMPARE(job.errorCode(), QStringLiteral("invalid_training_file"));
    QCOMPARE(job.errorMessage(), QStringLiteral("line 3 is malformed"));
    QCOMPARE(job.errorParam(), QStringLiteral("training_file"));
    QVERIFY(job.isTerminal());
}

void TestFineTuning::reportsTerminalStatus_data()
{
    QTest::addColumn<QString>("wireStatus");
    QTest::addColumn<bool>("terminal");

    QTest::newRow("validating_files") << QStringLiteral("validating_files") << false;
    QTest::newRow("queued") << QStringLiteral("queued") << false;
    QTest::newRow("running") << QStringLiteral("running") << false;
    QTest::newRow("paused") << QStringLiteral("paused") << false;
    QTest::newRow("succeeded") << QStringLiteral("succeeded") << true;
    QTest::newRow("failed") << QStringLiteral("failed") << true;
    QTest::newRow("cancelled") << QStringLiteral("cancelled") << true;
    // An unknown value decodes to the initial state, so polling continues.
    QTest::newRow("unknown") << QStringLiteral("something_new") << false;
}

void TestFineTuning::reportsTerminalStatus()
{
    QFETCH(QString, wireStatus);
    QFETCH(bool, terminal);

    const FineTuningJob job
            = FineTuningJob::fromJson(QJsonObject {{QStringLiteral("status"), wireStatus}});
    QCOMPARE(job.isTerminal(), terminal);
}

void TestFineTuning::parsesEvent()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("ft-event-1")},
            {QStringLiteral("object"), QStringLiteral("fine_tuning.job.event")},
            {QStringLiteral("created_at"), 1721764800},
            {QStringLiteral("level"), QStringLiteral("info")},
            {QStringLiteral("message"), QStringLiteral("Step 10/100: loss=0.42")},
            {QStringLiteral("type"), QStringLiteral("metrics")},
            {QStringLiteral("data"), QJsonObject {{QStringLiteral("step"), 10}}},
    };

    const FineTuningJobEvent event = FineTuningJobEvent::fromJson(json);
    QCOMPARE(event.id(), QStringLiteral("ft-event-1"));
    QCOMPARE(event.createdAt(), Q_INT64_C(1721764800));
    QCOMPARE(event.level(), QStringLiteral("info"));
    QCOMPARE(event.message(), QStringLiteral("Step 10/100: loss=0.42"));
    QCOMPARE(event.type(), QStringLiteral("metrics"));
    QCOMPARE(event.data().value(QStringLiteral("step")).toInt(), 10);
    QCOMPARE(FineTuningJobEvent::fromJson(event.toJson()), event);
}

void TestFineTuning::parsesCheckpoint()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("ftckpt_1")},
            {QStringLiteral("object"), QStringLiteral("fine_tuning.job.checkpoint")},
            {QStringLiteral("created_at"), 1721764867},
            {QStringLiteral("fine_tuned_model_checkpoint"),
             QStringLiteral("ft:gpt-4o-mini:org::id:ckpt-step-88")},
            {QStringLiteral("fine_tuning_job_id"), QStringLiteral("ftjob-1")},
            {QStringLiteral("step_number"), 88},
            {QStringLiteral("metrics"),
             QJsonObject {{QStringLiteral("step"), 88.0},
                          {QStringLiteral("train_loss"), 0.478},
                          {QStringLiteral("train_mean_token_accuracy"), 0.924},
                          {QStringLiteral("valid_loss"), 10.02},
                          {QStringLiteral("valid_mean_token_accuracy"), 0.0},
                          {QStringLiteral("full_valid_loss"), 0.567},
                          {QStringLiteral("full_valid_mean_token_accuracy"), 0.944}}},
    };

    const FineTuningJobCheckpoint checkpoint = FineTuningJobCheckpoint::fromJson(json);
    QCOMPARE(checkpoint.id(), QStringLiteral("ftckpt_1"));
    QCOMPARE(checkpoint.fineTuningJobId(), QStringLiteral("ftjob-1"));
    QCOMPARE(checkpoint.stepNumber(), 88);
    QCOMPARE(checkpoint.fineTunedModelCheckpoint(),
             QStringLiteral("ft:gpt-4o-mini:org::id:ckpt-step-88"));
    QCOMPARE(checkpoint.metrics().trainLoss, 0.478);
    QCOMPARE(checkpoint.metrics().fullValidMeanTokenAccuracy, 0.944);
    QCOMPARE(FineTuningJobCheckpoint::fromJson(checkpoint.toJson()), checkpoint);
}

void TestFineTuning::parsesPermission()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("cp_1")},
            {QStringLiteral("object"), QStringLiteral("checkpoint.permission")},
            {QStringLiteral("created_at"), 1721764867},
            {QStringLiteral("project_id"), QStringLiteral("proj_abc")},
    };

    const FineTuningCheckpointPermission permission
            = FineTuningCheckpointPermission::fromJson(json);
    QCOMPARE(permission.id(), QStringLiteral("cp_1"));
    QCOMPARE(permission.projectId(), QStringLiteral("proj_abc"));
    QCOMPARE(permission.createdAt(), Q_INT64_C(1721764867));
    QCOMPARE(FineTuningCheckpointPermission::fromJson(permission.toJson()), permission);
}

void TestFineTuning::createRequestSerialisesBody()
{
    CreateFineTuningJobRequest request(QStringLiteral("gpt-4o-mini-2024-07-18"),
                                       QStringLiteral("file-train"));
    request.setValidationFile(QStringLiteral("file-valid"));
    request.setSuffix(QStringLiteral("my-run"));
    request.setSeed(42);
    request.setMethodType(QStringLiteral("supervised"));
    FineTuningHyperparameters hyper;
    hyper.nEpochs = 3;
    request.setHyperparameters(hyper);
    request.setMetadata(QJsonObject {{QStringLiteral("owner"), QStringLiteral("qa")}});

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("model")).toString(),
             QStringLiteral("gpt-4o-mini-2024-07-18"));
    QCOMPARE(json.value(QStringLiteral("training_file")).toString(), QStringLiteral("file-train"));
    QCOMPARE(json.value(QStringLiteral("validation_file")).toString(),
             QStringLiteral("file-valid"));
    QCOMPARE(json.value(QStringLiteral("suffix")).toString(), QStringLiteral("my-run"));
    QCOMPARE(json.value(QStringLiteral("seed")).toInt(), 42);
    // The hyperparameters are nested under method.<type>.hyperparameters, and
    // only the ones the caller set are sent — the rest stay "auto" server-side.
    const QJsonObject method = json.value(QStringLiteral("method")).toObject();
    QCOMPARE(method.value(QStringLiteral("type")).toString(), QStringLiteral("supervised"));
    const QJsonObject hyperJson = method.value(QStringLiteral("supervised"))
                                          .toObject()
                                          .value(QStringLiteral("hyperparameters"))
                                          .toObject();
    QCOMPARE(hyperJson.value(QStringLiteral("n_epochs")).toInt(), 3);
    QVERIFY(!hyperJson.contains(QStringLiteral("batch_size")));
}

void TestFineTuning::createRequestOmitsUnsetFields()
{
    const CreateFineTuningJobRequest request(QStringLiteral("gpt-4o-mini"),
                                             QStringLiteral("file-train"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.size(), 2);
    QVERIFY(!json.contains(QStringLiteral("validation_file")));
    QVERIFY(!json.contains(QStringLiteral("method")));
    QVERIFY(!json.contains(QStringLiteral("seed")));
    QVERIFY(!json.contains(QStringLiteral("metadata")));
}

QTEST_MAIN(TestFineTuning)
#include "tst_finetuning.moc"
