// SPDX-License-Identifier: MIT
//
// Enumerate the models this key can reach (GET /models, GET /models/{id}) and
// hold them up against what the library knows about them locally:
//
//   client.listModels();                          // what the API offers
//   client.getModel(id);                          // one of them, by id
//   Core::ModelCatalog::shared().model(id);        // what it costs, offline
//
// **These are two different questions and they disagree in both directions.**
// `GET /models` says a model exists and who owns it -- an id, a creation date,
// nothing about context windows or price. `Core::ModelCatalog` says what a
// model can do and what it costs, from a table compiled into this build. So a
// model released after this build is listed by the API and unknown to the
// catalog, and a fine-tune the catalog has never heard of looks exactly the
// same. That is what `ModelInfo::isKnown()` distinguishes, and it is why the
// catalog hands back a fallback rather than nothing: a program that budgets
// tokens should not crash on an unfamiliar model, but it should be able to tell
// that its numbers are guesses.
//
// **The list is not a menu of chat models.** It includes embedding models,
// speech models, moderation models and fine-tunes, with nothing in the payload
// to tell them apart other than the id. Filtering by prefix is the practical
// approach and is what the `filter` argument below does.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./models                # every model, with what the catalog knows
//   ./models gpt-4o         # only ids containing "gpt-4o"

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Core/ModelCatalog.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

QString capabilityList(Core::ModelCapability::Flags flags)
{
    QStringList names;
    if (flags.testFlag(Core::ModelCapability::Tools))
        names << QStringLiteral("tools");
    if (flags.testFlag(Core::ModelCapability::Vision))
        names << QStringLiteral("vision");
    if (flags.testFlag(Core::ModelCapability::Audio))
        names << QStringLiteral("audio");
    if (flags.testFlag(Core::ModelCapability::StructuredOutputs))
        names << QStringLiteral("structured");
    if (flags.testFlag(Core::ModelCapability::Streaming))
        names << QStringLiteral("streaming");
    if (flags.testFlag(Core::ModelCapability::Reasoning))
        names << QStringLiteral("reasoning");
    return names.isEmpty() ? QStringLiteral("-") : names.join(QLatin1Char('/'));
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

    const QString filter = app.arguments().value(1);

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.exit(1);
    };

    // Second step: one model on its own. The single-model endpoint answers with
    // the same record as a row of the list -- it is there so a program that was
    // handed an id can check it exists without downloading the whole catalogue.
    const auto describeOne = [&](const QString &modelId) {
        Client::ModelReply *reply = client->getModel(modelId);
        QObject::connect(reply, &Client::ModelReply::failed, onError);
        QObject::connect(reply, &Client::ModelReply::finished, [&](const Core::Model &model) {
            const Core::ModelInfo info = Core::ModelCatalog::shared().model(model.id());
            out << "\n" << model.id() << "\n";
            out << "  owned by:    " << model.ownedBy() << "\n";
            if (info.isKnown()) {
                out << "  context:     " << info.contextWindow() << " tokens ("
                    << info.maxOutputTokens() << " out)\n";
                out << "  price:       $" << info.inputPrice() << " in / $" << info.outputPrice()
                    << " out per million tokens\n";
                out << "  encoding:    " << info.encoding() << "\n";
                out << "  can:         " << capabilityList(info.capabilities()) << "\n";
            } else {
                // Not an error, and worth saying out loud rather than printing
                // the fallback's numbers as if they were this model's.
                out << "  not in this build's catalog -- context window, price and\n"
                    << "  capabilities are unknown; ModelCatalog::model() answered\n"
                    << "  with its fallback so callers keep working.\n";
            }
            app.quit();
        });
    };

    // First step: everything the key can reach.
    Client::ModelListReply *list = client->listModels();
    QObject::connect(list, &Client::ModelListReply::failed, onError);
    QObject::connect(list, &Client::ModelListReply::finished, [&](const Core::ModelList &models) {
        int shown = 0;
        int known = 0;
        QString first;

        out << "id                                    catalog  context   $in/$out per Mtok\n";
        for (const Core::Model &model : models.data) {
            if (!filter.isEmpty() && !model.id().contains(filter, Qt::CaseInsensitive))
                continue;
            ++shown;
            if (first.isEmpty())
                first = model.id();

            const Core::ModelInfo info = Core::ModelCatalog::shared().model(model.id());
            out << model.id().leftJustified(38);
            if (info.isKnown()) {
                ++known;
                out << QStringLiteral("yes      %1  %2/%3")
                                .arg(info.contextWindow(), 8)
                                .arg(info.inputPrice())
                                .arg(info.outputPrice());
            } else {
                out << "no";
            }
            out << "\n";
        }

        out << "\n" << shown << " model(s), " << known << " with local pricing.\n";
        if (shown == 0) {
            out << "Nothing matched \"" << filter << "\".\n";
            app.quit();
            return;
        }
        // The catalog also knows models this key cannot reach -- it is a price
        // list, not an entitlement check. ids() is that side of it.
        out << "The catalog knows " << Core::ModelCatalog::shared().ids().size()
            << " model(s) in total, reachable or not.\n";

        describeOne(first);
    });

    return app.exec();
}
