// SPDX-License-Identifier: MIT
//
// Screening both sides of a conversation against the moderation policy.
//
//   Guardrail guardrail(&client);
//   guardrail.setAction("violence", GuardrailAction::Warn);
//   auto *reply = guardrail.createChatCompletion(request);
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./guardrails "Tell me about the French Revolution."
//
// Both sides are worth screening and for different reasons: the input, so the
// application does not spend a request relaying something it would refuse to
// show; the output, because a model can produce what its prompt did not ask
// for. That is three round trips for one answer, which is the honest cost.
//
// The policy is per category because the categories are not comparable -- an
// app for adults may reasonably allow what a children's app must block, and
// neither is a "level" of the other. Anything the policy does not name is
// blocked, so a category nobody thought about fails safe.

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/Guardrail.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

QString describe(Client::GuardedChatReply::Position position)
{
    return position == Client::GuardedChatReply::Position::Input
                   ? QStringLiteral("your question")
                   : QStringLiteral("the model's answer");
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

    const QString question = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                      : QStringLiteral("Tell me about the French Revolution.");
    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));

    Client::Client client(QUrl(baseUrl), apiKey);
    Client::Guardrail guardrail(&client);

    // A history question will mention violence; that is a reason to note it,
    // not a reason to refuse. Self-harm is the other way round.
    guardrail.setAction(QStringLiteral("violence"), Client::GuardrailAction::Warn);
    guardrail.setAction(QStringLiteral("harassment"), Client::GuardrailAction::Warn);
    guardrail.setAction(QStringLiteral("self-harm"), Client::GuardrailAction::Block);

    const Core::ChatCompletionRequest request(model, {Core::Message::user(question)});
    Client::GuardedChatReply *reply = guardrail.createChatCompletion(request);

    QObject::connect(reply, &Client::GuardedChatReply::flagged,
                     [&out](Client::GuardedChatReply::Position position,
                            const Client::GuardrailVerdict &verdict) {
                         // Before the content it is about, so it can be shown
                         // as a notice rather than as a correction.
                         out << "[note] " << describe(position) << " touches on "
                             << verdict.category() << " (" << verdict.score() << ")\n\n";
                     });

    QObject::connect(reply, &Client::GuardedChatReply::blocked,
                     [&out, &app](Client::GuardedChatReply::Position position,
                                  const Client::GuardrailVerdict &verdict) {
                         out << "Blocked: " << describe(position) << " matched "
                             << verdict.categories.join(QStringLiteral(", ")) << "\n";
                         app.quit();
                     });

    QObject::connect(reply, &Client::GuardedChatReply::finished,
                     [&out, &app](const Core::ChatCompletionResponse &response) {
                         out << response.choices().value(0).message().content() << "\n";
                         app.quit();
                     });

    QObject::connect(reply, &Client::GuardedChatReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         // A guardrail that treats "I could not check" as "it
                         // is fine" is not a guardrail.
                         out << "Error: " << error.message() << "\n";
                         app.quit();
                     });

    return app.exec();
}
