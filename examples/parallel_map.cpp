// SPDX-License-Identifier: MIT
//
// Running many prompts at once and collecting the answers in order.
//
//   ChatMap map(&client);
//   map.setConcurrency(4);
//   auto *run = map.map(model, prompts);
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./parallel_map reviews.txt        # one prompt per line
//   ./parallel_map                    # a small built-in set
//
// The shape of classification over a dataset, fan-out summarisation and
// offline evals: N requests that have nothing to do with each other, which
// should go out together but not all at once.
//
// A failed item does not fail the run. One row of a thousand hitting a content
// filter is not a reason to throw away the other nine hundred and ninety-nine,
// so the error is recorded against its index and the run carries on -- which is
// why the results are printed with their index and their outcome. Only the exit
// status says that something failed, so a script running this can tell.

#include <QtOpenAi/Client/ChatMap.h>
#include <QtOpenAi/Client/Client.h>

#include "ExampleSupport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

QStringList readPrompts(const QString &path, QTextStream &out)
{
    if (path.isEmpty()) {
        return {QStringLiteral("Classify in one word: 'The battery lasts forever.'"),
                QStringLiteral("Classify in one word: 'It broke on day two.'"),
                QStringLiteral("Classify in one word: 'Arrived on time, does the job.'"),
                QStringLiteral("Classify in one word: 'I want my money back.'"),
                QStringLiteral("Classify in one word: 'Better than I expected.'"),
                QStringLiteral("Classify in one word: 'The manual is in the wrong language.'")};
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        out << "Cannot read " << path << "\n";
        return {};
    }
    QStringList prompts;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.isEmpty())
            prompts.append(line);
    }
    return prompts;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const std::optional<ExampleEnv> env = exampleEnv(out);
    if (!env)
        return 1;

    const QStringList prompts
            = readPrompts(argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString(), out);
    if (prompts.isEmpty())
        return 1;

    const QString model = env->model(QStringLiteral("gpt-4o-mini"));

    Client::Client client(env->baseUrl, env->key);

    Client::ChatMap map(&client);
    map.setConcurrency(4);
    // If everything is failing, something is wrong with the setup rather than
    // with the data -- stop rather than spending the rest of the list finding
    // that out.
    map.setMaxFailures(5);

    QElapsedTimer elapsed;
    elapsed.start();

    Client::ChatMapReply *run = map.map(model, prompts);

    QObject::connect(run, &Client::ChatMapReply::progress, [&out](int finished, int total) {
        out << "\r[" << finished << "/" << total << "]";
        out.flush();
    });

    QObject::connect(run, &Client::ChatMapReply::allFinished, [&]() {
        out << "\r";
        // In input order, with the failures still in their own places: a report
        // that closed the gaps would misalign every row after the first error.
        const QList<Client::ChatMapItem> results = run->results();
        for (const Client::ChatMapItem &item : results) {
            out << item.index << ": ";
            if (item.isSuccess())
                out << item.content() << "\n";
            else
                out << "(failed: " << item.error.message() << ")\n";
        }
        out << "\n"
            << run->successCount() << " of " << run->count() << " in " << elapsed.elapsed()
            << " ms";
        if (run->isAborted())
            out << " -- stopped early after " << run->failureCount() << " failures";
        out << "\n";
        app.exit(run->failureCount() > 0 ? 1 : 0);
    });

    return app.exec();
}
