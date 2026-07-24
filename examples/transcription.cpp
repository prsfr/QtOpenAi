// SPDX-License-Identifier: MIT
//
// Transcribe an audio file to text (POST /audio/transcriptions). The audio bytes
// are uploaded as multipart/form-data; the reply carries the transcript.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./transcribe clip.mp3

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
    if (argc < 2) {
        out << "Usage: transcribe <audio-file>\n";
        return 1;
    }

    const QString path = QString::fromLocal8Bit(argv[1]);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        out << "Cannot read " << path << "\n";
        return 1;
    }
    const QByteArray audio = file.readAll();

    Client::Client client(QUrl(baseUrl), apiKey);

    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("whisper-1"));
    Core::TranscriptionRequest request(audio, QFileInfo(path).fileName(), model);

    Client::TranscriptionReply *reply = client.createTranscription(request);

    QObject::connect(reply, &Client::TranscriptionReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.exit(1);
                     });

    QObject::connect(reply, &Client::TranscriptionReply::finished,
                     [&out, &app](const Core::TranscriptionResponse &response) {
                         out << response.text() << "\n";
                         app.quit();
                     });

    return app.exec();
}
