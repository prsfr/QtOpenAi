// SPDX-License-Identifier: MIT
//
// Make one video out of another, and register a reusable cameo (the /videos
// sub-resources):
//
//   client.editVideo(request);              // POST /videos/edits
//   client.extendVideo(request);            // POST /videos/extensions
//   client.createVideoCharacter(...);       // POST /videos/characters
//   client.getVideoCharacter(id);           // GET  /videos/characters/{id}
//
// **None of these changes the source.** Each answers with a *new* job that has
// to be polled and downloaded like an original generation, and the video it was
// made from is left exactly as it was. The new job's remixedFromVideoId() is the
// only link back to it, so a program that discards that link cannot reconstruct
// the chain later.
//
// **An extension's `seconds` is not the resulting length.** It is how much to
// add; the job comes back with the stitched total. Reading it as the former is
// how a caller ends up downloading a two-minute file it budgeted four seconds
// for.
//
// **A character is not a job.** It is created synchronously, has no status to
// poll, and there is no list endpoint and no delete endpoint -- getVideoCharacter()
// is the only way back to one, so an id that is lost is lost. The likeness in
// the uploaded footage belongs to somebody, which is a consent question and not
// a technical one.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./video_derivatives edit      video_abc123 "make it night"
//   ./video_derivatives extend    video_abc123 "the wave breaks" [seconds]
//   ./video_derivatives character "Ada" ada.mp4
//
// See examples/video_generation.cpp for the generate -> poll -> download flow this builds
// on; the id to pass here is one that flow produced.

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

// The length of the *new segment*. The published schema enumerates "4", "8" and
// "12" while the prose beside it says 4, 8, 12, 16 and 20, so the library passes
// the value through rather than validating it -- and so does this.
constexpr auto kDefaultExtensionSeconds = "8";

void usage(QTextStream &out, const QString &program)
{
    out << "Usage:\n"
        << "  " << program << " edit      <video-id> <prompt>\n"
        << "  " << program << " extend    <video-id> <prompt> [seconds]\n"
        << "  " << program << " character <name> <video-file>\n";
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

    const QStringList args = app.arguments();
    const QString action = args.value(1);
    const QString second = args.value(2);
    const QString third = args.value(3);
    if (action.isEmpty() || second.isEmpty() || third.isEmpty()) {
        usage(out, args.value(0));
        return 1;
    }

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.exit(1);
    };

    // --- Characters -------------------------------------------------------
    if (action == QLatin1String("character")) {
        QFile videoFile(third);
        if (!videoFile.open(QIODevice::ReadOnly)) {
            out << "Cannot read " << videoFile.fileName() << "\n";
            return 1;
        }

        Client::VideoCharacterReply *created = client->createVideoCharacter(
                second, QFileInfo(videoFile).fileName(), videoFile.readAll());
        QObject::connect(created, &Client::VideoCharacterReply::failed, onError);
        QObject::connect(created, &Client::VideoCharacterReply::finished,
                         [&](const Core::VideoCharacter &character) {
                             out << "Character " << character.id() << " (" << character.name()
                                 << ") created "
                                 << QDateTime::fromSecsSinceEpoch(character.createdAt())
                                             .toString(Qt::ISODate)
                                 << "\n";
                             // Worth saying plainly: there is no list endpoint,
                             // so this line is the record.
                             out << "Keep that id -- getVideoCharacter() is the only way back to\n"
                                 << "it, and nothing enumerates the characters you own.\n";
                             app.quit();
                         });
        return app.exec();
    }

    // --- Edits and extensions ---------------------------------------------
    const bool extending = action == QLatin1String("extend");
    if (!extending && action != QLatin1String("edit")) {
        usage(out, args.value(0));
        return 1;
    }

    // Naming the source keeps this a JSON request. Handing over bytes instead
    // -- request.setSourceVideo("clip.mp4", data) -- would switch it to a
    // multipart upload and is how footage the API has never seen gets edited.
    // The two are exclusive: setting either clears the other.
    Core::VideoSourceRequest request(second, third);
    if (extending)
        request.setSeconds(args.value(4, QString::fromLatin1(kDefaultExtensionSeconds)));

    Client::VideoReply *derived
            = extending ? client->extendVideo(request) : client->editVideo(request);
    QObject::connect(derived, &Client::VideoReply::failed, onError);
    QObject::connect(derived, &Client::VideoReply::finished, [&](const Core::VideoJob &job) {
        out << (extending ? "Extension " : "Edit ") << job.id() << " queued, derived from "
            << (job.remixedFromVideoId().isEmpty() ? second : job.remixedFromVideoId()) << "\n";

        Client::VideoPoller *poller = client->pollVideo(job.id(), 2000);
        QObject::connect(poller, &Client::VideoPoller::progressed,
                         [&](const Core::VideoJob &v) { out << "  " << v.progress() << "%\n"; });
        QObject::connect(poller, &Client::VideoPoller::completed, [&](const Core::VideoJob &done) {
            if (done.status() != Core::VideoStatus::Completed) {
                out << "Render failed: " << done.errorMessage() << "\n";
                app.exit(1);
                return;
            }
            out << "Done: " << done.id() << ", " << done.seconds() << "s";
            if (extending)
                out << "  (the stitched total, not the segment that was added)";
            out << "\n";
            if (done.expiresAt() > 0) {
                // A completed job is not a permanent one; the bytes stop being
                // downloadable at this point.
                out << "Download before "
                    << QDateTime::fromSecsSinceEpoch(done.expiresAt()).toString(Qt::ISODate)
                    << " with:\n"
                    << "  client.downloadVideoContent(\"" << done.id() << "\");\n";
            }
            app.quit();
        });
        poller->start();
    });

    return app.exec();
}
