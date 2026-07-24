// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>

#include <QtCore/QJsonDocument>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

#include "support/StubServer.h"

class TestReply : public QObject
{
    Q_OBJECT
private slots:
    void successfulCompletionEmitsFinished();
    void httpErrorEmitsFailedWithDetails();
    void requestBodyContainsModelAndMessages();
};

void TestReply::successfulCompletionEmitsFinished()
{
    const QByteArray body = R"({
        "id": "chatcmpl-1", "object": "chat.completion", "created": 1, "model": "gpt-4o",
        "choices": [{"index": 0, "message": {"role": "assistant", "content": "Hi!"},
                     "finish_reason": "stop"}],
        "usage": {"prompt_tokens": 1, "completion_tokens": 1, "total_tokens": 2}
    })";
    StubServer server(200, body);
    Client client(server.baseUrl(), QStringLiteral("test-key"));

    ChatCompletionReply *reply = client.createChatCompletion(
            ChatCompletionRequest(QStringLiteral("gpt-4o"), {Message::user(QStringLiteral("hi"))}));
    reply->setAutoDelete(false);

    QSignalSpy finishedSpy(reply, &ChatCompletionReply::finished);
    QSignalSpy failedSpy(reply, &ChatCompletionReply::failed);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(reply->isSuccess());
    QCOMPARE(reply->response().firstMessage().content(), QStringLiteral("Hi!"));
    delete reply;
}

void TestReply::httpErrorEmitsFailedWithDetails()
{
    const QByteArray body = R"({
        "error": {"message": "Invalid API key", "type": "invalid_request_error", "code": "invalid_api_key"}
    })";
    StubServer server(401, body);
    Client client(server.baseUrl(), QStringLiteral("bad-key"));

    ChatCompletionReply *reply = client.createChatCompletion(
            ChatCompletionRequest(QStringLiteral("gpt-4o"), {Message::user(QStringLiteral("hi"))}));
    reply->setAutoDelete(false);

    QSignalSpy failedSpy(reply, &ChatCompletionReply::failed);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    QVERIFY(!reply->isSuccess());
    QCOMPARE(failedSpy.count(), 1);
    const ClientError error = reply->error();
    QCOMPARE(error.kind(), ClientError::Kind::Http);
    QCOMPARE(error.httpStatus(), 401);
    QCOMPARE(error.message(), QStringLiteral("Invalid API key"));
    QCOMPARE(error.type(), QStringLiteral("invalid_request_error"));
    QCOMPARE(error.code(), QStringLiteral("invalid_api_key"));
    delete reply;
}

void TestReply::requestBodyContainsModelAndMessages()
{
    const QByteArray body = R"({
        "id": "x", "object": "chat.completion", "created": 1, "model": "gpt-4o",
        "choices": [], "usage": {"prompt_tokens": 0, "completion_tokens": 0, "total_tokens": 0}
    })";
    StubServer server(200, body);
    Client client(server.baseUrl(), QStringLiteral("k"));

    ChatCompletionReply *reply = client.createChatCompletion(ChatCompletionRequest(
            QStringLiteral("gpt-4o-mini"), {Message::user(QStringLiteral("ping"))}));
    reply->setAutoDelete(false);
    QVERIFY(QTest::qWaitFor([reply] { return reply->isFinished(); }, 5000));

    const QJsonObject sent = QJsonDocument::fromJson(server.requestBody()).object();
    QCOMPARE(sent.value(QStringLiteral("model")).toString(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(sent.value(QStringLiteral("messages")).toArray().size(), 1);
    delete reply;
}

QTEST_MAIN(TestReply)
#include "tst_reply.moc"
