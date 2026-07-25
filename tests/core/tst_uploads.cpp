// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/CreateUploadRequest.h>
#include <QtOpenAi/Core/Upload.h>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Uploads types (#17): upload-status mapping and terminality,
// Upload/UploadPart parsing & round-trip (including the nested completed file),
// and the create-upload JSON body.
class TestUploads : public QObject
{
    Q_OBJECT
private slots:
    void statusStringMapping();
    void statusFromUnknownDefaultsToPending();
    void terminality();
    void parsesPendingUpload();
    void parsesCompletedUploadWithFile();
    void uploadRoundTrip();
    void parsesUploadPart();
    void createRequestJsonBody();
};

void TestUploads::statusStringMapping()
{
    QCOMPARE(uploadStatusToString(UploadStatus::Pending), QStringLiteral("pending"));
    QCOMPARE(uploadStatusToString(UploadStatus::Completed), QStringLiteral("completed"));
    QCOMPARE(uploadStatusToString(UploadStatus::Cancelled), QStringLiteral("cancelled"));
    QCOMPARE(uploadStatusToString(UploadStatus::Expired), QStringLiteral("expired"));

    QCOMPARE(uploadStatusFromString(QStringLiteral("pending")), UploadStatus::Pending);
    QCOMPARE(uploadStatusFromString(QStringLiteral("completed")), UploadStatus::Completed);
    QCOMPARE(uploadStatusFromString(QStringLiteral("cancelled")), UploadStatus::Cancelled);
    QCOMPARE(uploadStatusFromString(QStringLiteral("expired")), UploadStatus::Expired);
}

void TestUploads::statusFromUnknownDefaultsToPending()
{
    QCOMPARE(uploadStatusFromString(QStringLiteral("something_new")), UploadStatus::Pending);
    QCOMPARE(uploadStatusFromString(QString()), UploadStatus::Pending);
}

void TestUploads::terminality()
{
    Upload upload;
    upload.setStatus(UploadStatus::Pending);
    QVERIFY(!upload.isTerminal());
    upload.setStatus(UploadStatus::Completed);
    QVERIFY(upload.isTerminal());
    upload.setStatus(UploadStatus::Cancelled);
    QVERIFY(upload.isTerminal());
    upload.setStatus(UploadStatus::Expired);
    QVERIFY(upload.isTerminal());
}

void TestUploads::parsesPendingUpload()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("upload_abc")},
            {QStringLiteral("object"), QStringLiteral("upload")},
            {QStringLiteral("created_at"), 1719184911},
            {QStringLiteral("filename"), QStringLiteral("training.jsonl")},
            {QStringLiteral("bytes"), QJsonValue(Q_INT64_C(2147483648))},
            {QStringLiteral("purpose"), QStringLiteral("fine-tune")},
            {QStringLiteral("status"), QStringLiteral("pending")},
            {QStringLiteral("expires_at"), 1719127296},
    };

    const Upload upload = Upload::fromJson(json);
    QCOMPARE(upload.id(), QStringLiteral("upload_abc"));
    QCOMPARE(upload.filename(), QStringLiteral("training.jsonl"));
    // Beyond 32 bits: the size must survive as an exact 64-bit value.
    QCOMPARE(upload.bytes(), Q_INT64_C(2147483648));
    QCOMPARE(upload.purpose(), QStringLiteral("fine-tune"));
    QCOMPARE(upload.status(), UploadStatus::Pending);
    QCOMPARE(upload.expiresAt(), Q_INT64_C(1719127296));
    QVERIFY(!upload.file().has_value());
}

void TestUploads::parsesCompletedUploadWithFile()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("upload_abc")},
            {QStringLiteral("status"), QStringLiteral("completed")},
            {QStringLiteral("file"),
             QJsonObject {{QStringLiteral("id"), QStringLiteral("file-xyz")},
                          {QStringLiteral("object"), QStringLiteral("file")},
                          {QStringLiteral("bytes"), QJsonValue(Q_INT64_C(2147483648))},
                          {QStringLiteral("purpose"), QStringLiteral("fine-tune")}}},
    };

    const Upload upload = Upload::fromJson(json);
    QVERIFY(upload.isTerminal());
    QVERIFY(upload.file().has_value());
    QCOMPARE(upload.file()->id(), QStringLiteral("file-xyz"));
    QCOMPARE(upload.file()->bytes(), Q_INT64_C(2147483648));
}

void TestUploads::uploadRoundTrip()
{
    FileObject file;
    file.setId(QStringLiteral("file-xyz"));
    file.setPurpose(QStringLiteral("batch"));

    Upload upload;
    upload.setId(QStringLiteral("upload_1"));
    upload.setObject(QStringLiteral("upload"));
    upload.setCreatedAt(1700000000);
    upload.setFilename(QStringLiteral("big.jsonl"));
    upload.setBytes(1024);
    upload.setPurpose(QStringLiteral("batch"));
    upload.setStatus(UploadStatus::Completed);
    upload.setExpiresAt(1700003600);
    upload.setFile(file);

    QCOMPARE(Upload::fromJson(upload.toJson()), upload);
}

void TestUploads::parsesUploadPart()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("part_def")},
            {QStringLiteral("object"), QStringLiteral("upload.part")},
            {QStringLiteral("created_at"), 1719185911},
            {QStringLiteral("upload_id"), QStringLiteral("upload_abc")},
    };

    const UploadPart part = UploadPart::fromJson(json);
    QCOMPARE(part.id(), QStringLiteral("part_def"));
    QCOMPARE(part.object(), QStringLiteral("upload.part"));
    QCOMPARE(part.createdAt(), Q_INT64_C(1719185911));
    QCOMPARE(part.uploadId(), QStringLiteral("upload_abc"));
    QCOMPARE(UploadPart::fromJson(part.toJson()), part);
}

void TestUploads::createRequestJsonBody()
{
    CreateUploadRequest request(QStringLiteral("training.jsonl"), QStringLiteral("fine-tune"),
                                2147483648, QStringLiteral("text/jsonl"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("filename")).toString(), QStringLiteral("training.jsonl"));
    QCOMPARE(json.value(QStringLiteral("purpose")).toString(), QStringLiteral("fine-tune"));
    QCOMPARE(json.value(QStringLiteral("bytes")).toVariant().toLongLong(), Q_INT64_C(2147483648));
    QCOMPARE(json.value(QStringLiteral("mime_type")).toString(), QStringLiteral("text/jsonl"));
}

QTEST_MAIN(TestUploads)
#include "tst_uploads.moc"
