// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/FileObject.h>
#include <QtOpenAi/Core/FileUploadRequest.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Files types (#16): FileObject parsing/round-trip, the
// FileList (ListPage) shape, and the multipart form fields of an upload request.
class TestFiles : public QObject
{
    Q_OBJECT
private slots:
    void parsesFileObject();
    void fileObjectRoundTrip();
    void parsesDeletionAcknowledgement();
    void parsesFileList();
    void uploadRequestFormFields();
    void uploadRequestOmitsUnsetFields();
};

static QString fieldValue(const QList<FileUploadRequest::FormField> &fields, const QString &name)
{
    for (const auto &field : fields)
        if (field.first == name)
            return field.second;
    return QString();
}

void TestFiles::parsesFileObject()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("file-abc123")},
            {QStringLiteral("object"), QStringLiteral("file")},
            {QStringLiteral("bytes"), 120000},
            {QStringLiteral("created_at"), 1699061776},
            {QStringLiteral("expires_at"), 1699065376},
            {QStringLiteral("filename"), QStringLiteral("training.jsonl")},
            {QStringLiteral("purpose"), QStringLiteral("fine-tune")},
            {QStringLiteral("status"), QStringLiteral("processed")},
    };

    const FileObject file = FileObject::fromJson(json);
    QCOMPARE(file.id(), QStringLiteral("file-abc123"));
    QCOMPARE(file.object(), QStringLiteral("file"));
    QCOMPARE(file.bytes(), Q_INT64_C(120000));
    QCOMPARE(file.createdAt(), Q_INT64_C(1699061776));
    QCOMPARE(file.expiresAt(), Q_INT64_C(1699065376));
    QCOMPARE(file.filename(), QStringLiteral("training.jsonl"));
    QCOMPARE(file.purpose(), QStringLiteral("fine-tune"));
    QCOMPARE(file.status(), QStringLiteral("processed"));
    QVERIFY(file.statusDetails().isEmpty());
}

void TestFiles::fileObjectRoundTrip()
{
    FileObject file;
    file.setId(QStringLiteral("file-1"));
    file.setObject(QStringLiteral("file"));
    file.setBytes(42);
    file.setCreatedAt(1700000000);
    file.setExpiresAt(1700003600);
    file.setFilename(QStringLiteral("notes.txt"));
    file.setPurpose(QStringLiteral("user_data"));
    file.setStatus(QStringLiteral("error"));
    file.setStatusDetails(QStringLiteral("unsupported encoding"));

    QCOMPARE(FileObject::fromJson(file.toJson()), file);
}

void TestFiles::parsesDeletionAcknowledgement()
{
    // DELETE /files/{id} answers with the deletion acknowledgement, which shares
    // the file shape (id + object) — the same convention as the other endpoints.
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("file-abc123")},
            {QStringLiteral("object"), QStringLiteral("file.deleted")},
            {QStringLiteral("deleted"), true},
    };

    const FileObject file = FileObject::fromJson(json);
    QCOMPARE(file.id(), QStringLiteral("file-abc123"));
    QCOMPARE(file.object(), QStringLiteral("file.deleted"));
    QCOMPARE(file.bytes(), Q_INT64_C(0));
}

void TestFiles::parsesFileList()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("list")},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), QStringLiteral("file-1")},
                                      {QStringLiteral("purpose"), QStringLiteral("assistants")}},
                         QJsonObject {{QStringLiteral("id"), QStringLiteral("file-2")},
                                      {QStringLiteral("purpose"), QStringLiteral("assistants")}}}},
            {QStringLiteral("first_id"), QStringLiteral("file-1")},
            {QStringLiteral("last_id"), QStringLiteral("file-2")},
            {QStringLiteral("has_more"), true},
    };

    const FileList list = FileList::fromJson(json);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.data.at(1).id(), QStringLiteral("file-2"));
    QCOMPARE(list.firstId, QStringLiteral("file-1"));
    QVERIFY(list.hasMore);
    QCOMPARE(FileList::fromJson(list.toJson()), list);
}

void TestFiles::uploadRequestFormFields()
{
    FileUploadRequest request(QByteArray("{\"a\":1}\n"), QStringLiteral("data.jsonl"),
                              QStringLiteral("fine-tune"));
    request.setExpiresAfter(QStringLiteral("created_at"), 3600);

    const auto fields = request.formFields();
    QCOMPARE(fieldValue(fields, QStringLiteral("purpose")), QStringLiteral("fine-tune"));
    QCOMPARE(fieldValue(fields, QStringLiteral("expires_after[anchor]")),
             QStringLiteral("created_at"));
    QCOMPARE(fieldValue(fields, QStringLiteral("expires_after[seconds]")), QStringLiteral("3600"));

    // The bytes travel out-of-band as the multipart `file` part.
    QCOMPARE(request.fileData(), QByteArray("{\"a\":1}\n"));
    QCOMPARE(request.fileName(), QStringLiteral("data.jsonl"));
}

void TestFiles::uploadRequestOmitsUnsetFields()
{
    FileUploadRequest request(QByteArray("hi"), QStringLiteral("a.txt"),
                              QStringLiteral("user_data"));

    const auto fields = request.formFields();
    QCOMPARE(fields.size(), 1);
    QCOMPARE(fields.first().first, QStringLiteral("purpose"));
    QVERIFY(!request.expiresAfterSeconds().has_value());
}

QTEST_MAIN(TestFiles)
#include "tst_files.moc"
