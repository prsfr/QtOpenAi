// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/PageWalker.h>

#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

// Coverage for the shared cursor-pagination helper (#29): PageWalker turns any
// list endpoint into an iterate-all, following `last_id` until `has_more` is
// false.
class TestPagination : public QObject
{
    Q_OBJECT
private slots:
    void walksTwoPagesAndFinishes();
    void carriesCallerParamsOntoEveryPage();
    void stopsOnSinglePage();
    void surfacesRequestFailure();
    void stopsWhenAsked();
};

void TestPagination::walksTwoPagesAndFinishes()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"object":"list","data":[{"id":"file-1"},{"id":"file-2"}],)"
             R"("first_id":"file-1","last_id":"file-2","has_more":true})"},
            {R"({"object":"list","data":[{"id":"file-3"}],)"
             R"("first_id":"file-3","last_id":"file-3","has_more":false})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    auto *walker = new PageWalker<FileListReply, FileList>(
            [&client](const ListParams &params) { return client.listFiles(params); });
    walker->setAutoDelete(false);

    QStringList seen;
    walker->setPageHandler([&seen](const FileList &page) {
        for (const FileObject &file : page.data)
            seen.append(file.id());
    });
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(finishedSpy.wait(5000));

    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(walker->pageCount(), 2);
    QCOMPARE(seen, QStringList({QStringLiteral("file-1"), QStringLiteral("file-2"),
                                QStringLiteral("file-3")}));
    // The second request continues after the first page's last id.
    QCOMPARE(server.requestCount(), 2);
    QVERIFY(!server.requestLines().at(0).contains("after="));
    QVERIFY(server.requestLines().at(1).contains("after=file-2"));
    delete walker;
}

void TestPagination::carriesCallerParamsOntoEveryPage()
{
    StubServer server(QList<StubServer::Response> {
            {R"({"object":"list","data":[{"id":"file-1"}],)"
             R"("last_id":"file-1","has_more":true})"},
            {R"({"object":"list","data":[{"id":"file-2"}],)"
             R"("last_id":"file-2","has_more":false})"},
    });
    Client client(server.baseUrl(), QStringLiteral("k"));

    ListParams params;
    params.limit = 1;
    params.order = QStringLiteral("asc");

    auto *walker = new PageWalker<FileListReply, FileList>(
            [&client](const ListParams &p) { return client.listFiles(p); }, params);
    walker->setAutoDelete(false);
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(finishedSpy.wait(5000));

    for (const QByteArray &line : server.requestLines()) {
        QVERIFY(line.contains("limit=1"));
        QVERIFY(line.contains("order=asc"));
    }
    delete walker;
}

void TestPagination::stopsOnSinglePage()
{
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"file-1"}],)"
                                 R"("last_id":"file-1","has_more":false})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    auto *walker = new PageWalker<FileListReply, FileList>(
            [&client](const ListParams &params) { return client.listFiles(params); });
    walker->setAutoDelete(false);
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(finishedSpy.wait(5000));

    QCOMPARE(walker->pageCount(), 1);
    QCOMPARE(server.requestCount(), 1);
    QVERIFY(walker->isFinished());
    delete walker;
}

void TestPagination::surfacesRequestFailure()
{
    // 400 rather than a 5xx: the default retry policy would otherwise re-issue
    // the request with backoff and drag the test out.
    StubServer server(400, QByteArray(R"({"error":{"message":"nope"}})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    auto *walker = new PageWalker<FileListReply, FileList>(
            [&client](const ListParams &params) { return client.listFiles(params); });
    walker->setAutoDelete(false);
    QSignalSpy failedSpy(walker, &PageWalkerBase::failed);
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(failedSpy.wait(5000));

    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);
    QVERIFY(walker->isFinished());
    delete walker;
}

void TestPagination::stopsWhenAsked()
{
    // A handler that has seen enough can stop the walk; no further request goes
    // out and no terminal signal is emitted.
    StubServer server(QByteArray(R"({"object":"list","data":[{"id":"file-1"}],)"
                                 R"("last_id":"file-1","has_more":true})"));
    Client client(server.baseUrl(), QStringLiteral("k"));

    auto *walker = new PageWalker<FileListReply, FileList>(
            [&client](const ListParams &params) { return client.listFiles(params); });
    walker->setAutoDelete(false);
    walker->setPageHandler([walker](const FileList &) { walker->stop(); });
    QSignalSpy finishedSpy(walker, &PageWalkerBase::finished);

    walker->start();
    QVERIFY(QTest::qWaitFor([walker] { return !walker->isWalking(); }, 5000));
    QTest::qWait(100); // give a stray follow-up request time to show up

    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(finishedSpy.count(), 0);
    delete walker;
}

QTEST_MAIN(TestPagination)
#include "tst_pagination.moc"
