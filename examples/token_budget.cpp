// SPDX-License-Identifier: MIT
//
// What a prompt will cost, before sending it.
//
// Two things the library knows without a network round trip:
//
//   1. Core::ModelCatalog -- the model's context window, output limit,
//      capabilities and price.
//   2. Core::TokenCounter -- how many tokens the messages come to, exactly if a
//      vocabulary has been loaded and by estimate otherwise.
//
// Together they answer the question worth asking before a request goes out:
// does this fit, and what will it cost?
//
// Usage:
//   ./token_budget [model] [prompt]
//   OPENAI_ENCODING_FILE=/path/to/o200k_base.tiktoken ./token_budget
//
// No API key and no network: everything here is local. Without an encoding
// file the counts are the one-token-per-four-characters estimate, which the
// output labels as such -- the `.tiktoken` files are published by the tiktoken
// project and are not bundled here.

#include <QtOpenAi/Core/Message.h>
#include <QtOpenAi/Core/ModelCatalog.h>
#include <QtOpenAi/Core/TokenCounter.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QString model
            = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("gpt-4o-mini-2024-07-18");
    const QString prompt
            = argc > 2 ? QString::fromLocal8Bit(argv[2])
                       : QStringLiteral("Summarise the design of the Qt meta-object system.");

    // --- What the model is ------------------------------------------------
    const Core::ModelInfo info = Core::ModelCatalog::shared().model(model);

    out << "Model:    " << info.id() << (info.isKnown() ? "" : "  (unknown -- assuming defaults)")
        << "\n";
    out << "Context:  " << info.contextWindow() << " tokens, up to " << info.maxOutputTokens()
        << " out\n";
    out << "Encoding: " << info.encoding() << "\n";
    out << "Price:    $" << info.inputPrice() << " in / $" << info.outputPrice()
        << " out per 1M tokens\n";
    out << "Tools:    " << (info.supports(Core::ModelCapability::Tools) ? "yes" : "no")
        << ", vision: " << (info.supports(Core::ModelCapability::Vision) ? "yes" : "no") << "\n\n";

    // --- What the prompt costs --------------------------------------------
    const QString encodingFile = QProcessEnvironment::systemEnvironment().value(
            QStringLiteral("OPENAI_ENCODING_FILE"));
    if (!encodingFile.isEmpty()
        && !Core::TokenCounter::loadEncodingFile(info.encoding(), encodingFile)) {
        out << "Could not read " << encodingFile << "; falling back to the estimate.\n";
    }

    const Core::TokenCounter counter = Core::TokenCounter::forModel(model);
    const QList<Core::Message> messages {
            Core::Message::system(QStringLiteral("You are a concise technical writer.")),
            Core::Message::user(prompt),
    };

    const int tokens = counter.count(messages);
    out << "Prompt:   " << tokens << " tokens"
        << (counter.isExact() ? " (exact)" : " (estimated -- set OPENAI_ENCODING_FILE)") << "\n";

    // Prices are per million tokens, which is the unit the price list uses.
    out << "Input:    $" << tokens * info.inputPrice() / 1e6 << "\n";
    out << "Headroom: " << info.contextWindow() - tokens << " tokens for the reply\n";

    return 0;
}
