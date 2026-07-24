// SPDX-License-Identifier: MIT
//
// Synthesise speech from text (POST /audio/speech) and write the audio blob to
// a file. Unlike the JSON endpoints this returns binary audio, so SpeechReply
// (a BinaryReply) hands back the raw bytes plus their Content-Type.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./tts "Hello from Qt!" [out.mp3]

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

    const QString text
            = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("Hello from Qt!");
    const QString path = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QStringLiteral("out.mp3");

    Client::Client client(QUrl(baseUrl), apiKey);

    Core::SpeechRequest request(QStringLiteral("gpt-4o-mini-tts"), text, QStringLiteral("alloy"));
    request.setResponseFormat(QStringLiteral("mp3")); // opus / aac / flac / wav / pcm

    Client::SpeechReply *reply = client.createSpeech(request);

    QObject::connect(reply, &Client::SpeechReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.exit(1);
                     });

    QObject::connect(reply, &Client::SpeechReply::finished,
                     [&out, &app, path, reply](const QByteArray &audio) {
                         QFile file(path);
                         if (!file.open(QIODevice::WriteOnly)) {
                             out << "Cannot write " << path << "\n";
                             app.exit(1);
                             return;
                         }
                         file.write(audio);
                         out << "Wrote " << audio.size() << " bytes (" << reply->contentType()
                             << ") to " << path << "\n";
                         app.quit();
                     });

    return app.exec();
}
