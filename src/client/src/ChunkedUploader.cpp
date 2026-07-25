// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ChunkedUploader.h"

#include "QtOpenAi/Client/Client.h"
#include "QtOpenAi/Client/UploadPartReply.h"
#include "QtOpenAi/Client/UploadReply.h"

#include <QtCore/QIODevice>
#include <QtCore/QPointer>

namespace QtOpenAi {
namespace Client {

class ChunkedUploaderPrivate
{
public:
    QPointer<Client> client;
    Core::CreateUploadRequest request;
    QIODevice *source = nullptr;
    bool ownsSource = false;
    qint64 chunkSize = ChunkedUploader::defaultChunkSize;
    qint64 bytesTotal = 0;
    qint64 bytesSent = 0;
    bool running = false;
    bool finished = false;
    bool autoDelete = true;
    QString uploadId;
    QStringList partIds;
    Core::Upload upload;
};

ChunkedUploader::ChunkedUploader(Client *client, Core::CreateUploadRequest request,
                                 QIODevice *source, bool ownsSource, qint64 chunkSize,
                                 QObject *parent)
    : QObject(parent)
    , d_ptr(new ChunkedUploaderPrivate)
{
    Q_D(ChunkedUploader);
    d->client = client;
    d->request = std::move(request);
    d->source = source;
    d->ownsSource = ownsSource;
    d->chunkSize = chunkSize > 0 ? chunkSize : defaultChunkSize;
    if (ownsSource && source)
        source->setParent(this);
}

ChunkedUploader::~ChunkedUploader() = default;

QString ChunkedUploader::uploadId() const
{
    Q_D(const ChunkedUploader);
    return d->uploadId;
}

qint64 ChunkedUploader::chunkSize() const
{
    Q_D(const ChunkedUploader);
    return d->chunkSize;
}

qint64 ChunkedUploader::bytesTotal() const
{
    Q_D(const ChunkedUploader);
    return d->bytesTotal;
}

qint64 ChunkedUploader::bytesSent() const
{
    Q_D(const ChunkedUploader);
    return d->bytesSent;
}

bool ChunkedUploader::isRunning() const
{
    Q_D(const ChunkedUploader);
    return d->running;
}

bool ChunkedUploader::isFinished() const
{
    Q_D(const ChunkedUploader);
    return d->finished;
}

Core::Upload ChunkedUploader::upload() const
{
    Q_D(const ChunkedUploader);
    return d->upload;
}

void ChunkedUploader::setAutoDelete(bool enabled)
{
    Q_D(ChunkedUploader);
    d->autoDelete = enabled;
}

bool ChunkedUploader::autoDelete() const
{
    Q_D(const ChunkedUploader);
    return d->autoDelete;
}

void ChunkedUploader::start()
{
    Q_D(ChunkedUploader);
    if (d->running || d->finished)
        return;

    if (!d->client) {
        fail(ClientError(ClientError::Kind::Network, QStringLiteral("client no longer available")));
        return;
    }
    if (!d->source || (!d->source->isOpen() && !d->source->open(QIODevice::ReadOnly))) {
        fail(ClientError(ClientError::Kind::InvalidRequest,
                         QStringLiteral("upload source is not readable")));
        return;
    }

    d->running = true;
    // The API needs the total size up front; derive it when the caller left it
    // unset, which is what the QByteArray overload of uploadInChunks() does.
    d->bytesTotal = d->request.bytes() > 0 ? d->request.bytes() : d->source->size();
    Core::CreateUploadRequest request = d->request;
    request.setBytes(d->bytesTotal);

    UploadReply *reply = d->client->createUpload(request);
    connect(reply, &UploadReply::finished, this, [this](const Core::Upload &upload) {
        Q_D(ChunkedUploader);
        if (!d->running)
            return;
        d->upload = upload;
        d->uploadId = upload.id();
        sendNextPart();
    });
    connect(reply, &UploadReply::failed, this, &ChunkedUploader::fail);
}

void ChunkedUploader::sendNextPart()
{
    Q_D(ChunkedUploader);
    if (!d->running || !d->client)
        return;

    const QByteArray chunk = d->source->read(d->chunkSize);
    if (chunk.isEmpty()) {
        // Source drained: assemble the parts, in the order they were accepted.
        UploadReply *reply = d->client->completeUpload(d->uploadId, d->partIds);
        connect(reply, &UploadReply::finished, this, [this](const Core::Upload &upload) {
            Q_D(ChunkedUploader);
            if (!d->running)
                return;
            d->upload = upload;
            finish();
            Q_EMIT completed(upload);
        });
        connect(reply, &UploadReply::failed, this, &ChunkedUploader::fail);
        return;
    }

    const qint64 chunkBytes = chunk.size();
    UploadPartReply *reply = d->client->addUploadPart(d->uploadId, chunk);
    connect(reply, &UploadPartReply::finished, this,
            [this, chunkBytes](const Core::UploadPart &part) {
                Q_D(ChunkedUploader);
                if (!d->running)
                    return;
                d->partIds.append(part.id());
                d->bytesSent += chunkBytes;
                Q_EMIT progressed(d->bytesSent, d->bytesTotal);
                sendNextPart();
            });
    connect(reply, &UploadPartReply::failed, this, &ChunkedUploader::fail);
}

void ChunkedUploader::cancel()
{
    Q_D(ChunkedUploader);
    if (d->finished)
        return;
    d->running = false;
    if (d->client && !d->uploadId.isEmpty())
        d->client->cancelUpload(d->uploadId);
    finish();
}

void ChunkedUploader::fail(const ClientError &error)
{
    Q_D(ChunkedUploader);
    if (d->finished)
        return;
    finish();
    Q_EMIT failed(error);
}

void ChunkedUploader::finish()
{
    Q_D(ChunkedUploader);
    d->running = false;
    d->finished = true;
    if (d->autoDelete)
        deleteLater();
}

} // namespace Client
} // namespace QtOpenAi
