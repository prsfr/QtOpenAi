// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/SemanticIndex.h"

#include "QtOpenAi/Client/Client.h"

#include <QtCore/QJsonArray>
#include <QtCore/QPointer>

namespace QtOpenAi {
namespace Client {

namespace {

constexpr QLatin1String kDefaultModel("text-embedding-3-small");

// An id for a text the caller did not name. Its position: enough to look the
// entry back up, and honest that nobody said what this thing is.
QString positionalId(int index) { return QString::number(index); }

} // namespace

class SemanticIndexReplyPrivate
{
public:
    QStringList ids;
    ClientError error;
    bool finished = false;
};

SemanticIndexReply::SemanticIndexReply(QObject *parent)
    : QObject(parent)
    , d_ptr(new SemanticIndexReplyPrivate)
{ }

SemanticIndexReply::~SemanticIndexReply() = default;

bool SemanticIndexReply::isFinished() const
{
    Q_D(const SemanticIndexReply);
    return d->finished;
}

QStringList SemanticIndexReply::ids() const
{
    Q_D(const SemanticIndexReply);
    return d->ids;
}

ClientError SemanticIndexReply::error() const
{
    Q_D(const SemanticIndexReply);
    return d->error;
}

class SemanticQueryReplyPrivate
{
public:
    QList<Core::VectorMatch> matches;
    ClientError error;
    bool finished = false;
};

SemanticQueryReply::SemanticQueryReply(QObject *parent)
    : QObject(parent)
    , d_ptr(new SemanticQueryReplyPrivate)
{ }

SemanticQueryReply::~SemanticQueryReply() = default;

bool SemanticQueryReply::isFinished() const
{
    Q_D(const SemanticQueryReply);
    return d->finished;
}

QList<Core::VectorMatch> SemanticQueryReply::matches() const
{
    Q_D(const SemanticQueryReply);
    return d->matches;
}

ClientError SemanticQueryReply::error() const
{
    Q_D(const SemanticQueryReply);
    return d->error;
}

class SemanticIndexPrivate
{
public:
    QPointer<Client> client;
    Core::VectorIndex index;
    QString model = QString(kDefaultModel);

    // Embed a batch and hand the vectors back, or report why not. Both entry
    // points need exactly this, and they must ask for the vectors the same way
    // or a query would rank against vectors built differently from the corpus.
    template <typename Reply, typename OnVectors>
    Reply *embed(SemanticIndex *owner, const QStringList &texts, OnVectors onVectors)
    {
        auto *reply = new Reply(owner);

        const auto fail = [reply](const ClientError &error) {
            reply->d_func()->error = error;
            reply->d_func()->finished = true;
            Q_EMIT reply->failed(error);
            Q_EMIT reply->done();
        };

        if (!client) {
            QMetaObject::invokeMethod(
                    reply,
                    [fail]() {
                        fail(ClientError(ClientError::Kind::InvalidRequest,
                                         QStringLiteral("no client to embed with")));
                    },
                    Qt::QueuedConnection);
            return reply;
        }

        // One request for the whole batch: the endpoint takes an array, and a
        // hundred paragraphs is not a hundred requests.
        Core::EmbeddingRequest request;
        request.setModel(model);
        request.setInput(QJsonArray::fromStringList(texts));

        EmbeddingReply *embedding = client->createEmbeddings(request);
        QObject::connect(embedding, &EmbeddingReply::finished, reply,
                         [reply, onVectors](const Core::EmbeddingResponse &response) {
                             QList<QList<double>> vectors;
                             const QList<Core::Embedding> data = response.data();
                             vectors.reserve(data.size());
                             for (const Core::Embedding &item : data)
                                 vectors.append(item.vector());
                             onVectors(reply, vectors);
                         });
        QObject::connect(embedding, &EmbeddingReply::failed, reply, fail);
        return reply;
    }
};

SemanticIndex::SemanticIndex(Client *client, QObject *parent)
    : QObject(parent)
    , d_ptr(new SemanticIndexPrivate)
{
    Q_D(SemanticIndex);
    d->client = client;
}

SemanticIndex::~SemanticIndex() = default;

Client *SemanticIndex::client() const
{
    Q_D(const SemanticIndex);
    return d->client;
}

QString SemanticIndex::model() const
{
    Q_D(const SemanticIndex);
    return d->model;
}

void SemanticIndex::setModel(const QString &model)
{
    Q_D(SemanticIndex);
    d->model = model;
}

Core::VectorIndex SemanticIndex::index() const
{
    Q_D(const SemanticIndex);
    return d->index;
}

void SemanticIndex::setIndex(const Core::VectorIndex &index)
{
    Q_D(SemanticIndex);
    d->index = index;
    Q_EMIT indexChanged();
}

SemanticIndexReply *SemanticIndex::add(const QStringList &texts)
{
    QStringList ids;
    ids.reserve(texts.size());
    // Positions within *this batch* plus what is already there, so a second
    // add() does not silently overwrite the first batch's entries.
    const int offset = index().size();
    for (int i = 0; i < texts.size(); ++i)
        ids.append(positionalId(offset + i));
    return add(ids, texts);
}

SemanticIndexReply *SemanticIndex::add(const QStringList &ids, const QStringList &texts)
{
    Q_D(SemanticIndex);

    if (texts.isEmpty() || ids.isEmpty()) {
        // Nothing to embed is not a request. Answered on the next turn so the
        // caller still gets to connect first.
        auto *empty = new SemanticIndexReply(this);
        QMetaObject::invokeMethod(
                empty,
                [empty]() {
                    empty->d_func()->finished = true;
                    Q_EMIT empty->finished(QStringList());
                    Q_EMIT empty->done();
                },
                Qt::QueuedConnection);
        return empty;
    }

    return d->embed<SemanticIndexReply>(
            this, texts,
            [this, ids, texts](SemanticIndexReply *reply, const QList<QList<double>> &vectors) {
                Q_D(SemanticIndex);
                QStringList added;
                // Zipped by position, which is the order the endpoint answers
                // in; a short answer indexes what came back rather than pairing
                // texts with the wrong vectors.
                const int count = int(qMin(vectors.size(), qMin(ids.size(), texts.size())));
                for (int i = 0; i < count; ++i) {
                    if (d->index.add(ids.at(i), vectors.at(i), texts.at(i)))
                        added.append(ids.at(i));
                }
                reply->d_func()->ids = added;
                reply->d_func()->finished = true;
                if (!added.isEmpty())
                    Q_EMIT indexChanged();
                Q_EMIT reply->finished(added);
                Q_EMIT reply->done();
            });
}

SemanticQueryReply *SemanticIndex::query(const QString &text, int k, double minScore)
{
    Q_D(SemanticIndex);

    if (d->index.isEmpty() || text.isEmpty()) {
        // Searching nothing has one possible answer, and it is not worth a
        // request to find out.
        auto *reply = new SemanticQueryReply(this);
        QMetaObject::invokeMethod(
                reply,
                [reply]() {
                    reply->d_func()->finished = true;
                    Q_EMIT reply->finished(QList<Core::VectorMatch>());
                    Q_EMIT reply->done();
                },
                Qt::QueuedConnection);
        return reply;
    }

    return d->embed<SemanticQueryReply>(
            this, {text},
            [this, k, minScore](SemanticQueryReply *reply, const QList<QList<double>> &vectors) {
                Q_D(SemanticIndex);
                const QList<Core::VectorMatch> matches
                        = vectors.isEmpty() ? QList<Core::VectorMatch>()
                                            : d->index.search(vectors.first(), k, minScore);
                reply->d_func()->matches = matches;
                reply->d_func()->finished = true;
                Q_EMIT reply->finished(matches);
                Q_EMIT reply->done();
            });
}

} // namespace Client
} // namespace QtOpenAi
