// SPDX-License-Identifier: MIT
//
// Hold a live Realtime session (the /realtime WebSocket channel). Unlike every
// other example here there is no request and no reply: the channel stays open
// and both sides talk whenever they have something to say.
//   1. createRealtimeClientSecret() — POST /realtime/client_secrets (REST)
//   2. RealtimeConnection::open()    — the WebSocket, opened with that secret
//   3. sendText()                    — a user turn, then a request to answer it
//   4. textDelta / audioDelta        — the answer, as it is produced
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./realtime "Explain a WebSocket in one sentence."
//   ./realtime --audio "Say hello"    # ask for audio and write it to a file
//
// Step 1 is the point of the split: the API key stays in this process, and the
// channel is opened with a short-lived secret — exactly what a browser or a
// mobile client would be handed. A server-side program can skip it and set the
// API key on the connection directly.
//
// Audio is written to realtime-output.pcm rather than played: 24 kHz mono
// 16-bit PCM, so no audio device is needed to see it work.
//   ffplay -f s16le -ar 24000 -ac 1 realtime-output.pcm

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Realtime/RealtimeConnection.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>

using namespace QtOpenAi;

namespace {

class RealtimeDemo
{
public:
    RealtimeDemo(Client::Client *client, Realtime::RealtimeConnection *connection, QString model,
                 QString prompt, bool wantAudio)
        : m_client(client)
        , m_connection(connection)
        , m_model(std::move(model))
        , m_prompt(std::move(prompt))
        , m_wantAudio(wantAudio)
    { }

    // 1. Mint the credential the channel will be opened with.
    void start()
    {
        Core::RealtimeSessionConfig config;
        config.setType(QStringLiteral("realtime"));
        config.setModel(m_model);
        config.setInstructions(QStringLiteral("You are concise."));
        config.setOutputModalities(
                {m_wantAudio ? QStringLiteral("audio") : QStringLiteral("text")});

        Client::RealtimeClientSecretReply *reply
                = m_client->createRealtimeClientSecret(config, 600);
        QObject::connect(
                reply, &Client::RealtimeClientSecretReply::failed,
                [this](const Client::ClientError &error) {
                    fail(QStringLiteral("Could not mint a client secret: %1").arg(error.message()));
                });
        QObject::connect(reply, &Client::RealtimeClientSecretReply::finished,
                         [this](const Core::RealtimeClientSecret &secret) {
                             print(QStringLiteral("Client secret %1... (expires at %2)")
                                           .arg(secret.value().left(12))
                                           .arg(secret.expiresAt()));
                             openChannel(secret.value());
                         });
    }

private:
    // 2. From here on nothing is a request/reply: it is one channel, and the
    // signals below are how the session talks back.
    void openChannel(const QString &clientSecret)
    {
        m_connection->setApiKey(clientSecret);
        m_connection->setModel(m_model);
        wireSignals();
        m_connection->open();
        // 3. Queued until the handshake completes, so there is nothing to wait
        // for here.
        m_connection->sendText(m_prompt);
    }

    void wireSignals()
    {
        QObject::connect(m_connection, &Realtime::RealtimeConnection::sessionCreated,
                         [this](const Core::RealtimeSessionConfig &session) {
                             print(QStringLiteral("Session %1 open on %2")
                                           .arg(session.id(), session.model()));
                             print(QStringLiteral("> %1").arg(m_prompt));
                         });

        // 4. Text arrives token by token; the transcript of spoken audio does
        // too, so an audio session still prints something readable.
        QObject::connect(m_connection, &Realtime::RealtimeConnection::textDelta,
                         [](const QString &delta) {
                             QTextStream out(stdout);
                             out << delta;
                             out.flush();
                         });
        QObject::connect(m_connection, &Realtime::RealtimeConnection::transcriptDelta,
                         [](const QString &delta) {
                             QTextStream out(stdout);
                             out << delta;
                             out.flush();
                         });
        QObject::connect(m_connection, &Realtime::RealtimeConnection::audioDelta,
                         [this](const QByteArray &pcm) { m_audio.append(pcm); });

        QObject::connect(m_connection, &Realtime::RealtimeConnection::responseFinished,
                         [this](const Core::RealtimeEvent &) { finish(); });

        // An error the server reports on the channel leaves it open, so it is
        // reported and the demo still shuts down cleanly.
        QObject::connect(m_connection, &Realtime::RealtimeConnection::errorReceived,
                         [this](const Core::RealtimeEvent &event) {
                             fail(QStringLiteral("Server error: %1").arg(event.errorMessage()));
                         });
        QObject::connect(m_connection, &Realtime::RealtimeConnection::socketError,
                         [this](const QString &message) {
                             fail(QStringLiteral("Connection failed: %1").arg(message));
                         });
    }

    void finish()
    {
        print(QString());
        if (!m_audio.isEmpty()) {
            QFile file(QStringLiteral("realtime-output.pcm"));
            if (file.open(QIODevice::WriteOnly)) {
                file.write(m_audio);
                print(QStringLiteral("Wrote %1 bytes of 24 kHz mono PCM to %2")
                              .arg(m_audio.size())
                              .arg(file.fileName()));
            }
        }
        m_connection->close();
        qApp->quit();
    }

    void fail(const QString &message)
    {
        print(message);
        m_connection->close();
        qApp->exit(1);
    }

    void print(const QString &line)
    {
        QTextStream out(stdout);
        out << line << "\n";
    }

    Client::Client *m_client;
    Realtime::RealtimeConnection *m_connection;
    QString m_model;
    QString m_prompt;
    bool m_wantAudio;
    QByteArray m_audio;
};

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value(QStringLiteral("OPENAI_API_KEY"));
    const QString baseUrl = env.value(QStringLiteral("OPENAI_BASE_URL"),
                                      QStringLiteral("https://api.openai.com/v1"));
    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-realtime"));
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example.\n";
        return 1;
    }

    bool wantAudio = false;
    QString prompt = QStringLiteral("Explain a WebSocket in one sentence.");
    for (int i = 1; i < argc; ++i) {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        if (argument == QLatin1String("--audio"))
            wantAudio = true;
        else
            prompt = argument;
    }

    Client::Client client(QUrl(baseUrl), apiKey, &app);
    Realtime::RealtimeConnection connection(&app);
    RealtimeDemo demo(&client, &connection, model, prompt, wantAudio);
    demo.start();

    // A channel has no natural end, so the demo stops itself if the model never
    // answers.
    QTimer::singleShot(60000, &app, [&app] {
        QTextStream(stdout) << "Timed out waiting for the session.\n";
        app.exit(1);
    });

    return app.exec();
}
