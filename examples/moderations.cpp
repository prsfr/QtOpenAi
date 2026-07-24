// SPDX-License-Identifier: MIT
//
// Classify text against the moderation policy (POST /moderations) and report
// whether it was flagged.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./moderations "some text to check"

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
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
            = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("I want to hug a puppy.");

    Client::Client client(QUrl(baseUrl), apiKey);

    Core::ModerationRequest request(text);

    Client::ModerationReply *reply = client.createModeration(request);

    QObject::connect(reply, &Client::ModerationReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.exit(1);
                     });

    QObject::connect(reply, &Client::ModerationReply::finished,
                     [&out, &app](const Core::ModerationResponse &response) {
                         const Core::ModerationResult result = response.firstResult();
                         out << "flagged: " << (result.flagged() ? "yes" : "no") << "\n";
                         app.quit();
                     });

    return app.exec();
}
