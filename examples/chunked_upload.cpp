// SPDX-License-Identifier: MIT
//
// Upload a large file with the Uploads API (the /uploads endpoints). Files
// beyond the single-request limit of POST /files are sent as a sequence of
// parts: create the upload, post each chunk, then complete it — at which point
// the server assembles the parts into one regular file object.
//
// `uploadInChunks` runs that whole flow, streaming straight from the QFile so
// only one chunk is in memory at a time.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./chunked_upload <path> [purpose] [chunk-bytes]

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value(QStringLiteral("OPENAI_API_KEY"));
    const QString baseUrl = env.value(QStringLiteral("OPENAI_BASE_URL"),
                                      QStringLiteral("https://api.openai.com/v1"));
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example.\n";
        return 1;
    }
    if (argc < 2) {
        out << "Usage: chunked_upload <path> [purpose] [chunk-bytes]\n";
        return 1;
    }

    QFile source(QString::fromLocal8Bit(argv[1]));
    if (!source.open(QIODevice::ReadOnly)) {
        out << "Cannot read " << source.fileName() << "\n";
        return 1;
    }

    const QString purpose = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QStringLiteral("batch");
    // A small default so the example shows several parts even for modest files;
    // production code can leave this at ChunkedUploader::defaultChunkSize (64 MB).
    const qint64 chunkSize = argc > 3 ? QByteArray(argv[3]).toLongLong() : 1024 * 1024;

    const QFileInfo info(source);
    const QString mimeType = QMimeDatabase().mimeTypeForFile(info).name();

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    Core::CreateUploadRequest request(info.fileName(), purpose, source.size(), mimeType);
    Client::ChunkedUploader *uploader = client->uploadInChunks(request, &source, chunkSize);

    QObject::connect(uploader, &Client::ChunkedUploader::progressed,
                     [&out](qint64 sent, qint64 total) {
                         out << "  " << sent << " / " << total << " bytes\n";
                         out.flush();
                     });

    QObject::connect(uploader, &Client::ChunkedUploader::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.exit(1);
                     });

    QObject::connect(uploader, &Client::ChunkedUploader::completed,
                     [&out, &app](const Core::Upload &upload) {
                         out << "Upload " << upload.id() << " completed.\n";
                         if (upload.file())
                             out << "Assembled file: " << upload.file()->id() << " ("
                                 << upload.file()->bytes() << " bytes)\n";
                         app.quit();
                     });

    out << "Uploading " << info.fileName() << " (" << source.size() << " bytes) in " << chunkSize
        << "-byte parts...\n";
    out.flush();
    uploader->start();

    return app.exec();
}
