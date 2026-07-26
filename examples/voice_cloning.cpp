// SPDX-License-Identifier: MIT
//
// Build a custom text-to-speech voice (the /audio/voices and
// /audio/voice_consents endpoints). Cloning a voice requires a recorded consent
// first, so the flow is:
//   1. createVoiceConsent() — POST /audio/voice_consents (multipart)
//   2. listVoiceConsents()  — GET /audio/voice_consents
//   3. createVoice()        — POST /audio/voices (multipart), citing the consent
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./voice_cloning consent.wav sample.wav ["Speaker name"]
//
// consent.wav is the spoken permission recording; sample.wav is the voice
// sample the new voice is built from. Both are uploaded verbatim.

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

bool readFile(const QString &path, QByteArray &bytes, QString &name, QTextStream &out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        out << "Cannot read " << path << "\n";
        return false;
    }
    bytes = file.readAll();
    name = QFileInfo(file).fileName();
    return true;
}

} // namespace

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
    if (argc < 3) {
        out << "Usage: voice_cloning <consent.wav> <sample.wav> [name]\n";
        return 1;
    }

    QByteArray consentBytes, sampleBytes;
    QString consentName, sampleName;
    if (!readFile(QString::fromLocal8Bit(argv[1]), consentBytes, consentName, out)
        || !readFile(QString::fromLocal8Bit(argv[2]), sampleBytes, sampleName, out)) {
        return 1;
    }
    const QString speaker
            = argc > 3 ? QString::fromLocal8Bit(argv[3]) : QStringLiteral("Example Speaker");

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    auto reportError = [&out, &app](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.exit(1);
    };

    // 1. The consent recording authorises cloning; a voice cannot be built
    // without one.
    Core::CreateVoiceConsentRequest consentRequest(speaker, QStringLiteral("en"), consentBytes,
                                                   consentName);
    Client::VoiceConsentReply *consent = client->createVoiceConsent(consentRequest);
    QObject::connect(consent, &Client::VoiceConsentReply::failed, reportError);

    QObject::connect(
            consent, &Client::VoiceConsentReply::finished,
            [&out, &app, client, speaker, sampleBytes, sampleName,
             reportError](const Core::VoiceConsent &recorded) {
                out << "Consent " << recorded.id() << " (" << recorded.consentStatus() << ")\n";
                out.flush();

                // 2. Show what is on file — consents are cursor-paginated like
                // every other list endpoint.
                Client::ListParams params;
                params.limit = 5;
                Client::VoiceConsentListReply *listing = client->listVoiceConsents(params);
                QObject::connect(listing, &Client::VoiceConsentListReply::failed, reportError);

                QObject::connect(
                        listing, &Client::VoiceConsentListReply::finished,
                        [&out, &app, client, speaker, sampleBytes, sampleName,
                         consentId = recorded.id(),
                         reportError](const Core::VoiceConsentList &list) {
                            out << "Consents on file (" << list.size() << "):\n";
                            for (const Core::VoiceConsent &item : list.data)
                                out << "  " << item.id() << "  " << item.name() << "  "
                                    << item.consentStatus() << "\n";
                            out.flush();

                            // 3. Build the voice from the sample.
                            Core::CreateVoiceRequest voiceRequest(speaker, consentId, sampleBytes,
                                                                  sampleName);
                            Client::VoiceReply *voice = client->createVoice(voiceRequest);
                            QObject::connect(voice, &Client::VoiceReply::failed, reportError);
                            QObject::connect(voice, &Client::VoiceReply::finished,
                                             [&out, &app](const Core::Voice &created) {
                                                 out << "Voice " << created.id() << " ("
                                                     << created.voiceStatus()
                                                     << ") — use its id as the `voice` of a "
                                                        "text-to-speech request once ready.\n";
                                                 app.quit();
                                             });
                        });
            });

    return app.exec();
}
