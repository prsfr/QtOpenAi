// SPDX-License-Identifier: MIT
//
// Create a response with the modern Responses API (POST /responses) and print
// its aggregated output text.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./responses "Give me one fun fact about the Qt framework."

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

    const QString input = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                   : QStringLiteral("Give me one fun fact about the Qt framework.");

    Client::Client client(QUrl(baseUrl), apiKey);

    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));
    Core::ResponseRequest request(model, input);

    Client::ResponseReply *reply = client.createResponse(request);

    QObject::connect(reply, &Client::ResponseReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.exit(1);
                     });

    QObject::connect(reply, &Client::ResponseReply::finished,
                     [&out, &app](const Core::Response &response) {
                         out << response.outputText() << "\n";
                         app.quit();
                     });

    return app.exec();
}
