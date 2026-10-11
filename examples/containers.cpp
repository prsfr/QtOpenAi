// SPDX-License-Identifier: MIT
//
// Work with a code-interpreter container (the /containers endpoints). A
// container is a short-lived sandbox with its own filesystem; this walks the
// lifecycle of one:
//   1. createContainer()               — POST /containers
//   2. uploadContainerFile()           — put bytes into /mnt/data
//   3. listContainerFiles()            — see what is in there
//   4. downloadContainerFileContent()  — read a file back out (raw bytes)
//   5. deleteContainer()               — tear the sandbox down
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./containers [path-to-upload]

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
        payload = QByteArray("city,population\nBerlin,3600000\nHamburg,1800000\n");
        fileName = QStringLiteral("cities.csv");
    }

    auto *client = new Client::Client(env->baseUrl, env->key, &app);

    auto reportError = failAndExit(out, app);

    Core::CreateContainerRequest request(QStringLiteral("qtopenai-example"));
    // Keep the sandbox short-lived so repeated runs do not pile up.
    request.setExpiresAfter(QStringLiteral("last_active_at"), 20);

    Client::ContainerReply *created = client->createContainer(request);
    QObject::connect(created, &Client::ContainerReply::failed, reportError);

    QObject::connect(
            created, &Client::ContainerReply::finished,
            [&out, client, payload, fileName, reportError](const Core::Container &container) {
                out << "Container " << container.id() << " (" << container.status() << ")\n";
                out.flush();

                const QString containerId = container.id();
                Client::ContainerFileReply *upload
                        = client->uploadContainerFile(containerId, fileName, payload);
                QObject::connect(upload, &Client::ContainerFileReply::failed, reportError);

                QObject::connect(
                        upload, &Client::ContainerFileReply::finished,
                        [&out, client, containerId, reportError](const Core::ContainerFile &file) {
                            out << "Uploaded to " << file.path() << " (" << file.bytes()
                                << " bytes)\n";
                            out.flush();

                            Client::ContainerFileListReply *listing
                                    = client->listContainerFiles(containerId);
                            QObject::connect(listing, &Client::ContainerFileListReply::failed,
                                             reportError);

                            QObject::connect(
                                    listing, &Client::ContainerFileListReply::finished,
                                    [&out, client, containerId, fileId = file.id(),
                                     reportError](const Core::ContainerFileList &list) {
                                        out << "Files in the container (" << list.size() << "):\n";
                                        for (const Core::ContainerFile &entry : list.data)
                                            out << "  " << entry.path() << "\n";
                                        out.flush();

                                        Client::BinaryReply *content
                                                = client->downloadContainerFileContent(containerId,
                                                                                       fileId);
                                        QObject::connect(content, &Client::BinaryReply::failed,
                                                         reportError);
                                        QObject::connect(
                                                content, &Client::BinaryReply::finished,
                                                [&out, client, containerId,
                                                 reportError](const QByteArray &bytes) {
                                                    out << "Read back " << bytes.size()
                                                        << " bytes.\n";
                                                    out.flush();

                                                    Client::ContainerReply *removal
                                                            = client->deleteContainer(containerId);
                                                    QObject::connect(
                                                            removal,
                                                            &Client::ContainerReply::failed,
                                                            reportError);
                                                    QObject::connect(
                                                            removal,
                                                            &Client::ContainerReply::finished,
                                                            [&out](const Core::Container &) {
                                                                out << "Container deleted.\n";
                                                                QCoreApplication::quit();
                                                            });
                                                });
                                    });
                        });
            });

    return app.exec();
}
