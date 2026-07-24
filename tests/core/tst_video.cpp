// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/CreateVideoRequest.h>
#include <QtOpenAi/Core/VideoJob.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the video/Sora types (#15): job status parsing & terminality,
// VideoJob round-trip, the VideoList (ListPage) shape, and the create-request
// JSON body / multipart form fields.
class TestVideo : public QObject
{
    Q_OBJECT
private slots:
    void statusStringMapping();
    void statusFromUnknownDefaultsToQueued();
    void terminality();
    void parsesJobResponse();
    void parsesFailedJobError();
    void jobRoundTrip();
    void parsesVideoList();
    void createRequestJsonBody();
    void createRequestFormFields();
    void createRequestInputReference();
};

static QString fieldValue(const QList<CreateVideoRequest::FormField> &fields, const QString &name)
{
    for (const auto &field : fields)
        if (field.first == name)
            return field.second;
    return QString();
}

void TestVideo::statusStringMapping()
{
    QCOMPARE(videoStatusToString(VideoStatus::Queued), QStringLiteral("queued"));
    QCOMPARE(videoStatusToString(VideoStatus::InProgress), QStringLiteral("in_progress"));
    QCOMPARE(videoStatusToString(VideoStatus::Completed), QStringLiteral("completed"));
    QCOMPARE(videoStatusToString(VideoStatus::Failed), QStringLiteral("failed"));

    QCOMPARE(videoStatusFromString(QStringLiteral("queued")), VideoStatus::Queued);
    QCOMPARE(videoStatusFromString(QStringLiteral("in_progress")), VideoStatus::InProgress);
    QCOMPARE(videoStatusFromString(QStringLiteral("completed")), VideoStatus::Completed);
    QCOMPARE(videoStatusFromString(QStringLiteral("failed")), VideoStatus::Failed);
}

void TestVideo::statusFromUnknownDefaultsToQueued()
{
    QCOMPARE(videoStatusFromString(QStringLiteral("something_new")), VideoStatus::Queued);
    QCOMPARE(videoStatusFromString(QString()), VideoStatus::Queued);
}

void TestVideo::terminality()
{
    VideoJob job;
    job.setStatus(VideoStatus::Queued);
    QVERIFY(!job.isTerminal());
    job.setStatus(VideoStatus::InProgress);
    QVERIFY(!job.isTerminal());
    job.setStatus(VideoStatus::Completed);
    QVERIFY(job.isTerminal());
    job.setStatus(VideoStatus::Failed);
    QVERIFY(job.isTerminal());
}

void TestVideo::parsesJobResponse()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("video_abc")},
            {QStringLiteral("object"), QStringLiteral("video")},
            {QStringLiteral("model"), QStringLiteral("sora-2")},
            {QStringLiteral("status"), QStringLiteral("in_progress")},
            {QStringLiteral("progress"), 42},
            {QStringLiteral("size"), QStringLiteral("720x1280")},
            {QStringLiteral("seconds"), QStringLiteral("8")},
            {QStringLiteral("created_at"), 1700000000},
            {QStringLiteral("completed_at"), QJsonValue::Null},
    };
    const VideoJob job = VideoJob::fromJson(json);
    QCOMPARE(job.id(), QStringLiteral("video_abc"));
    QCOMPARE(job.model(), QStringLiteral("sora-2"));
    QCOMPARE(job.status(), VideoStatus::InProgress);
    QCOMPARE(job.progress(), 42);
    QCOMPARE(job.size(), QStringLiteral("720x1280"));
    QCOMPARE(job.seconds(), QStringLiteral("8"));
    QCOMPARE(job.createdAt(), Q_INT64_C(1700000000));
    QCOMPARE(job.completedAt(), Q_INT64_C(0));
    QVERIFY(!job.isTerminal());
    QVERIFY(job.errorMessage().isEmpty());
}

void TestVideo::parsesFailedJobError()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("video_x")},
            {QStringLiteral("status"), QStringLiteral("failed")},
            {QStringLiteral("progress"), 100},
            {QStringLiteral("error"),
             QJsonObject {{QStringLiteral("code"), QStringLiteral("moderation_blocked")},
                          {QStringLiteral("message"), QStringLiteral("content policy")}}},
    };
    const VideoJob job = VideoJob::fromJson(json);
    QCOMPARE(job.status(), VideoStatus::Failed);
    QVERIFY(job.isTerminal());
    QCOMPARE(job.errorCode(), QStringLiteral("moderation_blocked"));
    QCOMPARE(job.errorMessage(), QStringLiteral("content policy"));
}

void TestVideo::jobRoundTrip()
{
    VideoJob job;
    job.setId(QStringLiteral("video_rt"));
    job.setStatus(VideoStatus::Completed);
    job.setProgress(100);
    job.setModel(QStringLiteral("sora-2-pro"));
    job.setSize(QStringLiteral("1024x1792"));
    job.setSeconds(QStringLiteral("12"));
    job.setCreatedAt(1700000001);
    job.setCompletedAt(1700000123);
    job.setErrorCode(QStringLiteral("e"));
    job.setErrorMessage(QStringLiteral("m"));

    const VideoJob reparsed = VideoJob::fromJson(job.toJson());
    QCOMPARE(reparsed, job);
}

void TestVideo::parsesVideoList()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("list")},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), QStringLiteral("video_1")},
                                      {QStringLiteral("status"), QStringLiteral("completed")}},
                         QJsonObject {{QStringLiteral("id"), QStringLiteral("video_2")},
                                      {QStringLiteral("status"), QStringLiteral("queued")}}}},
            {QStringLiteral("first_id"), QStringLiteral("video_1")},
            {QStringLiteral("last_id"), QStringLiteral("video_2")},
            {QStringLiteral("has_more"), true},
    };
    const VideoList list = VideoList::fromJson(json);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.data.at(0).id(), QStringLiteral("video_1"));
    QCOMPARE(list.data.at(0).status(), VideoStatus::Completed);
    QCOMPARE(list.data.at(1).status(), VideoStatus::Queued);
    QCOMPARE(list.firstId, QStringLiteral("video_1"));
    QCOMPARE(list.lastId, QStringLiteral("video_2"));
    QVERIFY(list.hasMore);
}

void TestVideo::createRequestJsonBody()
{
    CreateVideoRequest request(QStringLiteral("a cat surfing"), QStringLiteral("sora-2"));
    request.setSeconds(QStringLiteral("8"));
    request.setSize(QStringLiteral("720x1280"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("prompt")).toString(), QStringLiteral("a cat surfing"));
    QCOMPARE(json.value(QStringLiteral("model")).toString(), QStringLiteral("sora-2"));
    QCOMPARE(json.value(QStringLiteral("seconds")).toString(), QStringLiteral("8"));
    QCOMPARE(json.value(QStringLiteral("size")).toString(), QStringLiteral("720x1280"));
    QVERIFY(!request.hasInputReference());

    // Unset optionals do not leak.
    CreateVideoRequest minimal(QStringLiteral("just a prompt"));
    const QJsonObject minimalJson = minimal.toJson();
    QVERIFY(!minimalJson.contains(QStringLiteral("model")));
    QVERIFY(!minimalJson.contains(QStringLiteral("seconds")));
    QVERIFY(!minimalJson.contains(QStringLiteral("size")));
}

void TestVideo::createRequestFormFields()
{
    CreateVideoRequest request(QStringLiteral("a cat surfing"), QStringLiteral("sora-2"));
    request.setSeconds(QStringLiteral("8"));
    request.setSize(QStringLiteral("720x1280"));

    const QList<CreateVideoRequest::FormField> fields = request.formFields();
    QCOMPARE(fieldValue(fields, QStringLiteral("prompt")), QStringLiteral("a cat surfing"));
    QCOMPARE(fieldValue(fields, QStringLiteral("model")), QStringLiteral("sora-2"));
    QCOMPARE(fieldValue(fields, QStringLiteral("seconds")), QStringLiteral("8"));
    QCOMPARE(fieldValue(fields, QStringLiteral("size")), QStringLiteral("720x1280"));
}

void TestVideo::createRequestInputReference()
{
    CreateVideoRequest request(QStringLiteral("extend this"), QStringLiteral("sora-2"));
    QVERIFY(!request.hasInputReference());
    request.setInputReference(QStringLiteral("ref.png"), QByteArray("PNGbytes"));
    QVERIFY(request.hasInputReference());
    QCOMPARE(request.inputReferenceFileName(), QStringLiteral("ref.png"));
    QCOMPARE(request.inputReferenceData(), QByteArray("PNGbytes"));
}

QTEST_MAIN(TestVideo)
#include "tst_video.moc"
