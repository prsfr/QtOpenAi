// SPDX-License-Identifier: MIT
//
// A conversation that remembers, trims itself, and branches.
//
// The other chat examples send one request. This one keeps the history:
//
//   1. Chat::Transcript accumulates the turns and builds each request from
//      them, so the model sees the conversation rather than the last line.
//   2. Chat::TrimPolicy keeps that context inside the model's window as it
//      grows, without ever dropping the system prompt.
//   3. Editing a past question forks instead of overwriting -- both answers
//      stay reachable, which is what the "‹ 2/2 ›" control in a chat UI does.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./conversation            # interactive: type, or /edit, /branch, /save
//
// Without a key it runs the branching walk-through offline, which needs no
// network at all.

#include <QtOpenAi/Chat/Transcript.h>
#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

void printContext(QTextStream &out, const Chat::Transcript &transcript)
{
    out << "  context now:";
    for (const Core::Message &message : transcript.messages())
        out << " [" << Core::roleToString(message.role()) << "] " << message.content();
    out << "\n";
}

// Everything the transcript does, without a server: what a branch is, what
// trimming keeps, and what survives being written to disk.
int walkThrough(QTextStream &out)
{
    Chat::Transcript transcript;
    transcript.setSystemPrompt(QStringLiteral("You are terse."));

    const auto question = transcript.addUserMessage(QStringLiteral("Why is the sky blue?"));
    transcript.addMessage(
            Core::Message(Core::Role::Assistant, QStringLiteral("Rayleigh scattering.")));
    out << "A linear conversation:\n";
    printContext(out, transcript);

    // Edit the question. The old branch is not lost -- it is a sibling.
    out << "\nEditing the question forks it:\n";
    transcript.fork(question, Core::Message::user(QStringLiteral("Why are sunsets red?")));
    transcript.addMessage(Core::Message(Core::Role::Assistant,
                                        QStringLiteral("The same scattering, longer path.")));
    printContext(out, transcript);
    out << "  the question now has " << transcript.siblings(question).size()
        << " versions, and the " << "tree holds " << transcript.count() << " messages\n";

    // Switch back: the first answer is still there.
    out << "\nSwitching back to the first branch:\n";
    transcript.setActiveLeaf(transcript.children(question).value(0));
    printContext(out, transcript);

    // Trim. The system prompt is pinned, so it survives whatever the limit is.
    out << "\nWith a two-message budget:\n";
    Chat::TrimPolicy policy;
    policy.setMaxMessages(2);
    transcript.setTrimPolicy(policy);
    printContext(out, transcript);

    // Persistence keeps the whole tree, not just the active path.
    const QByteArray saved = QJsonDocument(transcript.toJson()).toJson(QJsonDocument::Compact);
    const Chat::Transcript restored
            = Chat::Transcript::fromJson(QJsonDocument::fromJson(saved).object());
    out << "\nSaved and reloaded: " << restored.count() << " messages, "
        << restored.siblings(question).size() << " versions of the question still there\n";

    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value(QStringLiteral("OPENAI_API_KEY"));
    if (apiKey.isEmpty()) {
        out << "No OPENAI_API_KEY set -- running the offline walk-through.\n\n";
        return walkThrough(out);
    }

    const QString baseUrl = env.value(QStringLiteral("OPENAI_BASE_URL"),
                                      QStringLiteral("https://api.openai.com/v1"));
    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));

    Client::Client client(QUrl(baseUrl), apiKey);

    Chat::Transcript transcript;
    transcript.setSystemPrompt(QStringLiteral("You are a concise assistant."));
    // The budget comes from the model itself: its window, less the room its
    // reply needs.
    transcript.setTrimPolicy(Chat::TrimPolicy::forModel(model));

    QTextStream in(stdin);
    out << "Type a message, or /edit <text> to replace your last one. Ctrl-D to quit.\n";

    // Remembers which node the user's last question was, so /edit can fork it.
    Chat::Transcript::NodeId lastQuestion = Chat::Transcript::InvalidNode;

    std::function<void()> prompt;
    prompt = [&] {
        out << "\n> ";
        out.flush();
        const QString line = in.readLine();
        if (line.isNull()) {
            app.quit();
            return;
        }
        if (line.isEmpty()) {
            prompt();
            return;
        }

        if (line.startsWith(QStringLiteral("/edit "))
            && lastQuestion != Chat::Transcript::InvalidNode) {
            // A sibling of the old question: the previous answer stays in the
            // tree, just off the active path.
            lastQuestion = transcript.fork(lastQuestion, Core::Message::user(line.mid(6)));
        } else {
            lastQuestion = transcript.addUserMessage(line);
        }

        Client::ChatCompletionReply *reply
                = client.createChatCompletion(transcript.buildRequest(model));

        QObject::connect(reply, &Client::ChatCompletionReply::failed,
                         [&out, &prompt](const Client::ClientError &error) {
                             out << "Error: " << error.message() << "\n";
                             prompt();
                         });

        QObject::connect(reply, &Client::ChatCompletionReply::finished,
                         [&](const Core::ChatCompletionResponse &response) {
                             const Core::Message message = response.firstMessage();
                             // The answer goes back into the transcript, which
                             // is what makes the next request a conversation.
                             transcript.addMessage(message);
                             out << message.content() << "\n";
                             out << "  (" << transcript.count() << " in the tree, "
                                 << transcript.messages().size() << " sent)\n";
                             prompt();
                         });
    };

    prompt();
    return app.exec();
}
