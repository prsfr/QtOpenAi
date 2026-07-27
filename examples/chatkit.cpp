// SPDX-License-Identifier: MIT
//
// Back OpenAI's hosted ChatKit UI (the /chatkit endpoints). This is the server
// half of a ChatKit app: it mints the ephemeral secret the browser needs, then
// reads back what the user and the workflow said.
//   1. createChatKitSession()   — POST /chatkit/sessions (the client secret)
//   2. listChatKitThreads()     — GET /chatkit/threads, scoped to that user
//   3. listChatKitThreadItems() — GET /chatkit/threads/{id}/items (transcript)
//   4. cancelChatKitSession()   — POST /chatkit/sessions/{id}/cancel
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   export CHATKIT_WORKFLOW_ID=wf_...    # the workflow the session runs
//   ./chatkit [end-user-id]              # defaults to "qtopenai-example-user"
//
// The point of step 1 is that the API key never leaves this process: the
// browser gets a short-lived secret scoped to one workflow and one end user.
// Threads are created by the ChatKit frontend as the user talks — there is no
// create call here — so a first run finds none, and re-running after a chat
// shows the transcript.

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

class ChatKitDemo
{
public:
    ChatKitDemo(Client::Client *client, QString workflowId, QString user)
        : m_client(client)
        , m_workflowId(std::move(workflowId))
        , m_user(std::move(user))
    { }

    // 1. A session is a scoped, expiring credential — not a conversation.
    void start()
    {
        Core::CreateChatKitSessionRequest request(m_workflowId, m_user);
        request.setExpiresAfter(600);
        request.setMaxRequestsPerMinute(30);

        Client::ChatKitSessionReply *reply = m_client->createChatKitSession(request);
        watch(reply);
        QObject::connect(
                reply, &Client::ChatKitSessionReply::finished,
                [this](const Core::ChatKitSession &session) {
                    m_sessionId = session.id();
                    print(QStringLiteral("Session %1 for %2").arg(session.id(), session.user()));
                    // Hand this — never the API key — to the browser.
                    print(QStringLiteral("  client secret: %1").arg(session.clientSecret()));
                    print(QStringLiteral("  expires at:    %1")
                                  .arg(QDateTime::fromSecsSinceEpoch(session.expiresAt())
                                               .toString(Qt::ISODate)));
                    listThreads();
                });
    }

private:
    // 2. Whatever this user has said so far, newest first.
    void listThreads()
    {
        Client::ListParams params;
        params.limit = 5;
        Client::ChatKitThreadListReply *reply = m_client->listChatKitThreads(params, m_user);
        watch(reply);
        QObject::connect(
                reply, &Client::ChatKitThreadListReply::finished,
                [this](const Core::ChatKitThreadList &list) {
                    print(QStringLiteral("Threads (%1):").arg(list.size()));
                    for (const Core::ChatKitThread &thread : list.data) {
                        print(QStringLiteral("  %1  %2  [%3]")
                                      .arg(thread.id(), thread.title(),
                                           Core::chatKitThreadStatusToString(thread.status())));
                    }
                    if (list.isEmpty()) {
                        print(QStringLiteral("  (none yet — the ChatKit UI creates "
                                             "threads as the user talks)"));
                        cancelSession();
                        return;
                    }
                    readTranscript(list.data.first().id());
                });
    }

    // 3. A thread mixes messages with widgets, tool calls and tasks, so only
    // the message variants have text to print.
    void readTranscript(const QString &threadId)
    {
        Client::ChatKitThreadItemListReply *reply = m_client->listChatKitThreadItems(threadId);
        watch(reply);
        QObject::connect(reply, &Client::ChatKitThreadItemListReply::finished,
                         [this](const Core::ChatKitThreadItemList &list) {
                             print(QStringLiteral("--- transcript ---"));
                             // Newest first, so walk it backwards to read in order.
                             for (int i = list.data.size() - 1; i >= 0; --i) {
                                 const Core::ChatKitThreadItem &item = list.data.at(i);
                                 if (item.isUserMessage() || item.isAssistantMessage()) {
                                     print(QStringLiteral("  %1: %2")
                                                   .arg(item.isUserMessage()
                                                                ? QStringLiteral("user")
                                                                : QStringLiteral("assistant"),
                                                        item.text()));
                                 } else {
                                     print(QStringLiteral("  [%1]").arg(item.type()));
                                 }
                             }
                             cancelSession();
                         });
    }

    // 4. Cancelling stops the secret from authenticating anything else, so a
    // demo run leaves no usable credential behind.
    void cancelSession()
    {
        Client::ChatKitSessionReply *reply = m_client->cancelChatKitSession(m_sessionId);
        watch(reply);
        QObject::connect(
                reply, &Client::ChatKitSessionReply::finished,
                [this](const Core::ChatKitSession &session) {
                    print(QStringLiteral("Session %1")
                                  .arg(Core::chatKitSessionStatusToString(session.status())));
                    qApp->quit();
                });
    }

    // Every request in the chain reports a failure the same way.
    template <typename Reply>
    void watch(Reply *reply)
    {
        QObject::connect(reply, &Reply::failed, [this](const Client::ClientError &error) {
            print(QStringLiteral("Error: %1").arg(error.message()));
            qApp->exit(1);
        });
    }

    void print(const QString &line)
    {
        QTextStream out(stdout);
        out << line << "\n";
    }

    Client::Client *m_client;
    QString m_workflowId;
    QString m_user;
    QString m_sessionId;
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
    const QString workflowId = env.value(QStringLiteral("CHATKIT_WORKFLOW_ID"));
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example.\n";
        return 1;
    }
    if (workflowId.isEmpty()) {
        out << "Set CHATKIT_WORKFLOW_ID to the workflow the session should run.\n";
        return 1;
    }

    const QString user
            = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("qtopenai-example-user");

    Client::Client client(QUrl(baseUrl), apiKey, &app);
    ChatKitDemo demo(&client, workflowId, user);
    demo.start();

    return app.exec();
}
