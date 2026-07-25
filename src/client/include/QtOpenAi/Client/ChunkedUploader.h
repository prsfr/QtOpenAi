// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/CreateUploadRequest.h>
#include <QtOpenAi/Core/Upload.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

class QIODevice;

namespace QtOpenAi {
namespace Client {

class Client;
class ChunkedUploaderPrivate;

// A signal-based drive-the-whole-flow helper for the Uploads API.
//
// The raw endpoints only offer the three steps of the protocol; this helper runs
// them end to end: it creates the upload, reads the source device one chunk at a
// time and posts each as a part (never holding more than one chunk in memory),
// then completes the upload with the collected part ids in order. progressed()
// reports after every accepted part, completed() carries the finished upload —
// whose file() is the assembled file — and failed() is emitted once if any step
// fails, after which nothing further is sent.
//
// Created by Client::uploadInChunks(); auto-deletes once it stops unless
// disabled. A source QIODevice passed in by the caller must outlive the run.
class QTOPENAI_CLIENT_EXPORT ChunkedUploader : public QObject
{
    Q_OBJECT
public:
    // The largest part the Uploads API accepts (64 MB), and the default here.
    static constexpr qint64 defaultChunkSize = 64ll * 1024 * 1024;

    ~ChunkedUploader() override;

    // The id assigned by the server; empty until the upload has been created.
    QString uploadId() const;

    // Bytes per part (defaults to defaultChunkSize).
    qint64 chunkSize() const;

    // Total bytes to send, taken from the request or the source's size.
    qint64 bytesTotal() const;
    // Bytes accepted by the server so far.
    qint64 bytesSent() const;

    bool isRunning() const;
    bool isFinished() const;

    // The most recently observed upload state.
    Core::Upload upload() const;

    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    // Begin the flow (POSTs /uploads immediately). No-op once started.
    void start();

    // Stop sending and ask the server to discard the upload
    // (POST /uploads/{id}/cancel). Emits neither completed() nor failed().
    void cancel();

Q_SIGNALS:
    // Emitted after every part the server accepted.
    void progressed(qint64 bytesSent, qint64 bytesTotal);
    // Emitted once when the upload has been completed; upload.file() holds the
    // assembled file.
    void completed(const QtOpenAi::Core::Upload &upload);
    // Emitted once when any step fails (network/HTTP/parse or an unreadable
    // source).
    void failed(const QtOpenAi::Client::ClientError &error);

private:
    friend class Client;
    ChunkedUploader(Client *client, Core::CreateUploadRequest request, QIODevice *source,
                    bool ownsSource, qint64 chunkSize, QObject *parent = nullptr);

    // Post the next chunk, or complete the upload when the source is drained.
    void sendNextPart();
    // Mark the run as finished and honour the auto-delete policy. Callers emit
    // their terminal signal (completed/failed) around this.
    void finish();
    // Emit failed() once and stop.
    void fail(const ClientError &error);

    Q_DECLARE_PRIVATE(ChunkedUploader)
    QScopedPointer<ChunkedUploaderPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
