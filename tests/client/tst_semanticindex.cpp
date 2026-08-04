// SPDX-License-Identifier: MIT
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/SemanticIndex.h>

#include <QtTest/QtTest>

#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;

namespace {

// An embeddings answer with one vector per input, in input order -- which is
// the property the whole zip-by-position step depends on.
QByteArray embeddings(const QList<QList<double>> &vectors)
{
    QJsonArray data;
    for (int i = 0; i < vectors.size(); ++i) {
        QJsonArray vector;
        for (double value : vectors.at(i))
            vector.append(value);
        data.append(QJsonObject {{QStringLiteral("object"), QStringLiteral("embedding")},
                                 {QStringLiteral("index"), i},
                                 {QStringLiteral("embedding"), vector}});
    }
    const QJsonObject response {{QStringLiteral("object"), QStringLiteral("list")},
                                {QStringLiteral("model"), QStringLiteral("text-embedding-3-small")},
                                {QStringLiteral("data"), data}};
    return QJsonDocument(response).toJson(QJsonDocument::Compact);
}

template <typename Reply>
bool settled(Reply *reply, int timeoutMs = 5000)
{
    if (!reply)
        return false;
    QSignalSpy done(reply, &Reply::done);
    return reply->isFinished() || done.wait(timeoutMs);
}

} // namespace

// Coverage for embed-index-query (#51).
class TestSemanticIndex : public QObject
{
    Q_OBJECT
private slots:
    void indexingEmbedsTheWholeBatchInOneRequest();
    void queryEmbedsAndRanks();
    void anEmptyIndexCostsNoRequest();
    void idsCanBeGivenAndDoNotCollideBetweenBatches();
    void theIndexIsAValueThatSurvivesARestart();
    void aFailedEmbeddingIsReported();
};

void TestSemanticIndex::indexingEmbedsTheWholeBatchInOneRequest()
{
    StubServer server(embeddings({{1.0, 0.0}, {0.0, 1.0}, {0.7, 0.7}}));
    Client client;
    client.setBaseUrl(server.baseUrl());

    SemanticIndex index(&client);
    QCOMPARE(index.model(), QStringLiteral("text-embedding-3-small"));
    QSignalSpy changed(&index, &SemanticIndex::indexChanged);

    auto *reply = index.add(
            {QStringLiteral("east"), QStringLiteral("north"), QStringLiteral("north-east")});
    QVERIFY(settled(reply));

    QCOMPARE(reply->ids().size(), 3);
    QCOMPARE(index.index().size(), 3);
    QCOMPARE(changed.count(), 1);

    // One request for three texts: the endpoint takes an array, and a hundred
    // paragraphs is not a hundred requests.
    QCOMPARE(server.requestCount(), 1);
    const QByteArray body = server.requestBody();
    QVERIFY(body.contains("\"east\""));
    QVERIFY(body.contains("\"north-east\""));
    QVERIFY(body.contains("text-embedding-3-small"));

    // The text rides along with the vector, so a hit is usable without a
    // second lookup into whatever the caller indexed from.
    QCOMPARE(index.index().text(reply->ids().at(0)), QStringLiteral("east"));
}

void TestSemanticIndex::queryEmbedsAndRanks()
{
    StubServer server(QList<StubServer::Response> {
            {embeddings({{1.0, 0.0}, {0.0, 1.0}, {0.7, 0.7}})}, // the corpus
            {embeddings({{1.0, 0.05}})}});                      // the query, pointing roughly east
    Client client;
    client.setBaseUrl(server.baseUrl());

    SemanticIndex index(&client);
    QVERIFY(settled(index.add(
            {QStringLiteral("east"), QStringLiteral("north"), QStringLiteral("north-east")})));

    auto *hits = index.query(QStringLiteral("which way is the sunrise?"), 2);
    QVERIFY(settled(hits));

    QCOMPARE(hits->matches().size(), 2);
    QCOMPARE(hits->matches().at(0).text, QStringLiteral("east"));
    QCOMPARE(hits->matches().at(1).text, QStringLiteral("north-east"));
    QVERIFY(hits->matches().at(0).score > hits->matches().at(1).score);
    QCOMPARE(server.requestCount(), 2);

    // The floor drops what is merely closest in favour of what is actually
    // about this -- for a retrieval prompt, the difference between context and
    // noise.
    auto *strict = index.query(QStringLiteral("which way is the sunrise?"), 5, 0.99);
    QVERIFY(settled(strict));
    QCOMPARE(strict->matches().size(), 1);
    QCOMPARE(strict->matches().at(0).text, QStringLiteral("east"));
}

void TestSemanticIndex::anEmptyIndexCostsNoRequest()
{
    StubServer server(embeddings({{1.0, 0.0}}));
    Client client;
    client.setBaseUrl(server.baseUrl());

    SemanticIndex index(&client);

    // Searching nothing has one possible answer, and it is not worth a request
    // to find out.
    auto *hits = index.query(QStringLiteral("anything"));
    QVERIFY(settled(hits, 2000));
    QVERIFY(hits->matches().isEmpty());

    // Nor is embedding nothing.
    auto *nothing = index.add(QStringList());
    QVERIFY(settled(nothing, 2000));
    QVERIFY(nothing->ids().isEmpty());

    QCOMPARE(server.requestCount(), 0);
}

void TestSemanticIndex::idsCanBeGivenAndDoNotCollideBetweenBatches()
{
    StubServer server(QList<StubServer::Response> {{embeddings({{1.0, 0.0}, {0.0, 1.0}})},
                                                   {embeddings({{0.7, 0.7}})}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    SemanticIndex index(&client);
    QVERIFY(settled(index.add({QStringLiteral("first"), QStringLiteral("second")})));
    // Generated ids continue past what is already there, so a second batch does
    // not silently overwrite the first.
    auto *second = index.add({QStringLiteral("third")});
    QVERIFY(settled(second));
    QCOMPARE(index.index().size(), 3);
    QCOMPARE(second->ids(), QStringList({QStringLiteral("2")}));

    // Or the caller names them, which is what an application with real
    // document ids does.
    StubServer named(embeddings({{1.0, 0.0}}));
    Client other;
    other.setBaseUrl(named.baseUrl());
    SemanticIndex byName(&other);
    auto *reply = byName.add({QStringLiteral("doc-42")}, {QStringLiteral("the text")});
    QVERIFY(settled(reply));
    QCOMPARE(reply->ids(), QStringList({QStringLiteral("doc-42")}));
    QCOMPARE(byName.index().text(QStringLiteral("doc-42")), QStringLiteral("the text"));
}

void TestSemanticIndex::theIndexIsAValueThatSurvivesARestart()
{
    StubServer server(QList<StubServer::Response> {{embeddings({{1.0, 0.0}, {0.0, 1.0}})},
                                                   {embeddings({{1.0, 0.02}})}});
    Client client;
    client.setBaseUrl(server.baseUrl());

    SemanticIndex first(&client);
    QVERIFY(settled(first.add({QStringLiteral("east"), QStringLiteral("north")})));

    // What an application would write to disk between runs.
    const QJsonObject saved = first.index().toJson();

    SemanticIndex restarted(&client);
    QSignalSpy changed(&restarted, &SemanticIndex::indexChanged);
    restarted.setIndex(VectorIndex::fromJson(saved));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(restarted.index().size(), 2);

    // One request to embed the query, and none to rebuild the corpus.
    auto *hits = restarted.query(QStringLiteral("sunrise"), 1);
    QVERIFY(settled(hits));
    QCOMPARE(hits->matches().at(0).text, QStringLiteral("east"));
    QCOMPARE(server.requestCount(), 2);
}

void TestSemanticIndex::aFailedEmbeddingIsReported()
{
    StubServer server(500, R"({"error":{"message":"embeddings are down"}})");
    Client client;
    client.setBaseUrl(server.baseUrl());
    client.setRetryPolicy(RetryPolicy::none());

    SemanticIndex index(&client);
    auto *reply = index.add({QStringLiteral("text")});
    QVERIFY(settled(reply));
    QCOMPARE(reply->error().message(), QStringLiteral("embeddings are down"));
    QVERIFY(reply->ids().isEmpty());
    QVERIFY(index.index().isEmpty());

    // And with no client at all it says so rather than never answering.
    SemanticIndex orphan(nullptr);
    auto *orphaned = orphan.add({QStringLiteral("text")});
    QVERIFY(settled(orphaned, 2000));
    QCOMPARE(orphaned->error().kind(), ClientError::Kind::InvalidRequest);
}

QTEST_MAIN(TestSemanticIndex)
#include "tst_semanticindex.moc"
