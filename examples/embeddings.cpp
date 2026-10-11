// SPDX-License-Identifier: MIT
//
// Create an embedding vector for a piece of text (POST /embeddings) and print
// its dimensionality plus the first few components.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./embeddings "The quick brown fox."

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

    const QString text
            = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("The quick brown fox.");

    Client::Client client(env->baseUrl, env->key);

    const QString model = env->model(QStringLiteral("text-embedding-3-small"));
    Core::EmbeddingRequest request(model, text);

    Client::EmbeddingReply *reply = client.createEmbeddings(request);

    QObject::connect(reply, &Client::EmbeddingReply::failed, failAndExit(out, app));

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
