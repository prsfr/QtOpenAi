// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/VectorIndex.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Client {

class Client;
class SemanticIndexPrivate;
class SemanticIndexReplyPrivate;
class SemanticQueryReplyPrivate;

// The pending result of indexing a batch of texts.
class QTOPENAI_CLIENT_EXPORT SemanticIndexReply : public QObject
{
    Q_OBJECT
public:
    ~SemanticIndexReply() override;

    bool isFinished() const;
    // The ids that were added, in input order.
    QStringList ids() const;
    ClientError error() const;

Q_SIGNALS:
    void finished(const QStringList &ids);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

private:
    friend class SemanticIndex;
    // The embed-then-act step lives there, so it needs the state it is filling.
    friend class SemanticIndexPrivate;
    explicit SemanticIndexReply(QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(SemanticIndexReply)
    QScopedPointer<SemanticIndexReplyPrivate> d_ptr;
};

// The pending result of a semantic query.
class QTOPENAI_CLIENT_EXPORT SemanticQueryReply : public QObject
{
    Q_OBJECT
public:
    ~SemanticQueryReply() override;

    bool isFinished() const;
    QList<Core::VectorMatch> matches() const;
    ClientError error() const;

Q_SIGNALS:
    void finished(const QList<QtOpenAi::Core::VectorMatch> &matches);
    void failed(const QtOpenAi::Client::ClientError &error);
    void done();

private:
    friend class SemanticIndex;
    friend class SemanticIndexPrivate;
    explicit SemanticQueryReply(QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(SemanticQueryReply)
    QScopedPointer<SemanticQueryReplyPrivate> d_ptr;
};

// Text in, ranked text out: Core::VectorIndex with the embedding step attached.
//
//     SemanticIndex index(&client);
//     index.add(paragraphs);                       // one request, embedded in a batch
//     auto *hits = index.query("how do I cancel?", 3);
//
// This is the retrieval half of retrieval-augmented generation, and everything
// it does is two steps a caller could have written: embed the text, then search
// the vectors. It exists because those two steps have to agree about the model
// -- vectors from two different embedding models rank against each other as
// convincing nonsense -- and keeping the model next to the index is the way
// they cannot drift apart.
//
// Indexing embeds a whole batch in one request, because the embeddings endpoint
// takes an array and one request for a hundred paragraphs is not a hundred
// requests.
//
// The index itself is a plain value: take a copy with index(), write it out
// with toJson(), and load it next time rather than paying to embed the same
// corpus again.
class QTOPENAI_CLIENT_EXPORT SemanticIndex : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString model READ model WRITE setModel)
public:
    explicit SemanticIndex(Client *client, QObject *parent = nullptr);
    ~SemanticIndex() override;

    Client *client() const;

    // The embedding model. Defaults to text-embedding-3-small: the cheap one,
    // which is the right default for a corpus a caller may re-embed while they
    // are still working out what they want.
    QString model() const;
    void setModel(const QString &model);

    // The vectors. Copyable, serialisable, and replaceable -- which is how a
    // corpus survives a restart.
    Core::VectorIndex index() const;
    void setIndex(const Core::VectorIndex &index);

    // Embed and index, in one request. Ids are the texts' positions as strings
    // unless given: enough to look an entry up, and honest that the caller has
    // not said what these things are.
    SemanticIndexReply *add(const QStringList &texts);
    SemanticIndexReply *add(const QStringList &ids, const QStringList &texts);

    // Embed the query and search. Returns immediately with an empty result if
    // the index is empty, rather than spending a request to search nothing.
    SemanticQueryReply *query(const QString &text, int k = 5,
                              double minScore = -std::numeric_limits<double>::infinity());

Q_SIGNALS:
    // The index changed -- for anything tracking what is in it.
    void indexChanged();

private:
    Q_DECLARE_PRIVATE(SemanticIndex)
    QScopedPointer<SemanticIndexPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
