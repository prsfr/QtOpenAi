// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

class TestModerationsClient : public QObject
{
    Q_OBJECT
private slots:
    void createPostsAndParsesFlagged();
};

void TestModerationsClient::createPostsAndParsesFlagged()
{
    StubServer server(R"({"id":"modr_1","model":"omni-moderation-latest","results":[
        {"flagged":true,"categories":{"violence":true},
         "category_scores":{"violence":0.91}}]})");
    Client client(server.baseUrl(), QStringLiteral("k"));

    ModerationReply *reply = client.createModeration(ModerationRequest(QStringLiteral("bad text")));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(reply->isSuccess());
    QVERIFY(server.requestLine().startsWith("POST /v1/moderations "));
    const ModerationResult result = reply->response().firstResult();
    QVERIFY(result.flagged());
    QCOMPARE(result.score(QStringLiteral("violence")), 0.91);
    delete reply;
}

QTEST_MAIN(TestModerationsClient)
#include "tst_moderations.moc"
