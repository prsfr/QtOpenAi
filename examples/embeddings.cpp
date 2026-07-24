// SPDX-License-Identifier: MIT
//
// Create an embedding vector for a piece of text (POST /embeddings) and print
// its dimensionality plus the first few components.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./embeddings "The quick brown fox."

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
            = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("The quick brown fox.");

    Client::Client client(QUrl(baseUrl), apiKey);

    const QString model
            = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("text-embedding-3-small"));
    Core::EmbeddingRequest request(model, text);

    Client::EmbeddingReply *reply = client.createEmbeddings(request);

    QObject::connect(reply, &Client::EmbeddingReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.exit(1);
                     });

    QObject::connect(reply, &Client::EmbeddingReply::finished,
                     [&out, &app](const Core::EmbeddingResponse &response) {
                         if (response.data().isEmpty()) {
                             out << "No embedding returned.\n";
                             app.exit(1);
                             return;
                         }
                         const QList<double> vector = response.data().first().vector();
                         out << "dimensions: " << vector.size() << "\n";
                         out << "first values:";
                         for (int i = 0; i < qMin(5, int(vector.size())); ++i)
                             out << " " << vector.at(i);
                         out << " ...\n";
                         app.quit();
                     });

    return app.exec();
}
