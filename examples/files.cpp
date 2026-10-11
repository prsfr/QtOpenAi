// SPDX-License-Identifier: MIT
//
// Manage stored files (the /files endpoints). Files are the currency of the
// fine-tuning, batch, assistants and vector-store endpoints, so this walks the
// whole lifecycle of one upload:
//   1. uploadFile()          — POST /files (multipart)
//   2. listFiles()           — GET /files, filtered by purpose
//   3. downloadFileContent() — GET /files/{id}/content (raw bytes)
//   4. deleteFile()          — DELETE /files/{id}
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./files [path-to-upload]        # defaults to a small generated text file

#include <QtOpenAi/Client/Client.h>

#include "ExampleSupport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const std::optional<ExampleEnv> env = exampleEnv(out);
    if (!env)
        return 1;

    // Either upload the file named on the command line or a small generated one.
    QByteArray payload;
    QString fileName;
    if (argc > 1) {
        if (!readFileArg(QString::fromLocal8Bit(argv[1]), payload, fileName, out))
            return 1;
    } else {
        payload = QByteArray("QtOpenAi example upload.\n");
        fileName = QStringLiteral("qtopenai-example.txt");
    }

    // "user_data" is the general-purpose bucket for files fed to a model as
    // input; fine-tuning and batch jobs use their own purposes.
    const QString purpose = QStringLiteral("user_data");

    auto *client = new Client::Client(env->baseUrl, env->key, &app);

    auto reportError = failAndExit(out, app);

    Core::FileUploadRequest request(payload, fileName, purpose);
    Client::FileReply *upload = client->uploadFile(request);
    QObject::connect(upload, &Client::FileReply::failed, reportError);

    QObject::connect(
            upload, &Client::FileReply::finished,
            [&out, &app, client, purpose, reportError](const Core::FileObject &uploaded) {
                out << "Uploaded " << uploaded.filename() << " as " << uploaded.id() << " ("
                    << uploaded.bytes() << " bytes, purpose " << uploaded.purpose() << ")\n";
                out.flush();

                Client::ListParams params;
                params.limit = 5;
                Client::FileListReply *listing = client->listFiles(params, purpose);
                QObject::connect(listing, &Client::FileListReply::failed, reportError);

                QObject::connect(
                        listing, &Client::FileListReply::finished,
                        [&out, &app, client, id = uploaded.id(),
                         reportError](const Core::FileList &list) {
                            out << "Files with this purpose (" << list.size() << "):\n";
                            for (const Core::FileObject &file : list.data)
                                out << "  " << file.id() << "  " << file.filename() << "\n";
                            out.flush();

                            Client::BinaryReply *content = client->downloadFileContent(id);
                            QObject::connect(content, &Client::BinaryReply::failed, reportError);

                            QObject::connect(
                                    content, &Client::BinaryReply::finished,
                                    [&out, &app, client, id, reportError](const QByteArray &bytes) {
                                        out << "Downloaded " << bytes.size() << " bytes back.\n";
                                        out.flush();

                                        // Clean up so repeated runs do not pile up.
                                        Client::FileReply *removal = client->deleteFile(id);
                                        QObject::connect(removal, &Client::FileReply::failed,
                                                         reportError);
                                        QObject::connect(removal, &Client::FileReply::finished,
                                                         [&out, &app](const Core::FileObject &) {
                                                             out << "Deleted.\n";
                                                             app.quit();
                                                         });
                                    });
                        });
            });

    return app.exec();
}
