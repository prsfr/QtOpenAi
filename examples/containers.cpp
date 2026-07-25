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

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
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

    // Either upload the file named on the command line or a small generated one.
    QByteArray payload;
    QString fileName;
    if (argc > 1) {
        QFile file(QString::fromLocal8Bit(argv[1]));
        if (!file.open(QIODevice::ReadOnly)) {
            out << "Cannot read " << file.fileName() << "\n";
            return 1;
        }
        payload = file.readAll();
        fileName = QFileInfo(file).fileName();
    } else {
        payload = QByteArray("city,population\nBerlin,3600000\nHamburg,1800000\n");
        fileName = QStringLiteral("cities.csv");
    }

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    auto reportError = [&out, &app](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.exit(1);
    };

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
