// SPDX-License-Identifier: MIT
//
// Generate a video with Sora and download it (the /videos endpoints). Rendering
// is asynchronous, so this shows the full flow:
//   1. createVideo() starts a job (returned in the `queued` state).
//   2. pollVideo() drives GET /videos/{id} on a timer, reporting progress until
//      the job is terminal.
//   3. downloadVideoContent() fetches the rendered bytes (a BinaryReply).
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./video "a cat surfing a wave at sunset" [out.mp4]

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
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

    const QString prompt = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                    : QStringLiteral("a cat surfing a wave at sunset");
    const QString path = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QStringLiteral("out.mp4");

    // Keep the client alive for the whole flow; it parents nothing here.
    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("sora-2"));
    Core::CreateVideoRequest request(prompt, model);
    request.setSize(QStringLiteral("720x1280"));
    request.setSeconds(QStringLiteral("8"));

    Client::VideoReply *job = client->createVideo(request);

    QObject::connect(job, &Client::VideoReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.exit(1);
                     });

    QObject::connect(job, &Client::VideoReply::finished,
                     [&out, &app, client, path](const Core::VideoJob &queued) {
                         out << "Job " << queued.id() << " queued; rendering...\n";
                         out.flush();

                         Client::VideoPoller *poller = client->pollVideo(queued.id());

                         QObject::connect(poller, &Client::VideoPoller::progressed,
                                          [&out](const Core::VideoJob &j) {
                                              out << "  progress: " << j.progress() << "%\n";
                                              out.flush();
                                          });

                         QObject::connect(poller, &Client::VideoPoller::failed,
                                          [&out, &app](const Client::ClientError &error) {
                                              out << "Poll error: " << error.message() << "\n";
                                              app.exit(1);
                                          });

                         QObject::connect(
                                 poller, &Client::VideoPoller::completed,
                                 [&out, &app, client, path](const Core::VideoJob &done) {
                                     if (done.status() != Core::VideoStatus::Completed) {
                                         out << "Render failed: " << done.errorMessage() << "\n";
                                         app.exit(1);
                                         return;
                                     }
                                     Client::VideoContentReply *content
                                             = client->downloadVideoContent(done.id());
                                     QObject::connect(
                                             content, &Client::VideoContentReply::failed,
                                             [&out, &app](const Client::ClientError &error) {
                                                 out << "Download error: " << error.message()
                                                     << "\n";
                                                 app.exit(1);
                                             });
                                     QObject::connect(content, &Client::VideoContentReply::finished,
                                                      [&out, &app, path](const QByteArray &mp4) {
                                                          QFile file(path);
                                                          if (file.open(QIODevice::WriteOnly))
                                                              file.write(mp4);
                                                          out << "Wrote " << mp4.size()
                                                              << " bytes to " << path << "\n";
                                                          app.quit();
                                                      });
                                 });

                         poller->start();
                     });

    return app.exec();
}
