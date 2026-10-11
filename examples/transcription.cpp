// SPDX-License-Identifier: MIT
//
// Transcribe an audio file to text (POST /audio/transcriptions). The audio bytes
// are uploaded as multipart/form-data; the reply carries the transcript.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./transcribe clip.mp3

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
    if (argc < 2) {
        out << "Usage: transcribe <audio-file>\n";
        return 1;
    }

    const QString path = QString::fromLocal8Bit(argv[1]);
    QByteArray audio;
    QString fileName;
    if (!readFileArg(path, audio, fileName, out))
        return 1;

    Client::Client client(env->baseUrl, env->key);

    const QString model = env->model(QStringLiteral("whisper-1"));
    Core::TranscriptionRequest request(audio, fileName, model);

    Client::TranscriptionReply *reply = client.createTranscription(request);

    QObject::connect(reply, &Client::TranscriptionReply::failed, failAndExit(out, app));

    QObject::connect(reply, &Client::TranscriptionReply::finished,
                     [&out, &app](const Core::TranscriptionResponse &response) {
                         out << response.text() << "\n";
                         app.quit();
                     });

    return app.exec();
}
