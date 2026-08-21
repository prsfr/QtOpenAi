// SPDX-License-Identifier: MIT
//
// Browse what this application has already sent (the /chat/completions
// management endpoints):
//
//   client.listChatCompletions(params);              // GET  /chat/completions
//   client.getChatCompletion(id);                    // GET  .../{id}
//   client.listChatCompletionMessages(id);           // GET  .../{id}/messages
//   client.updateChatCompletion(id, metadata);       // POST .../{id}
//   client.deleteChatCompletion(id);                 // DELETE .../{id}
//
// **Nothing is here unless it was stored.** A completion is retrievable only if
// the request that created it set `store: true`; the default is off, and a
// program that never set it will find this list empty however much it has sent.
// So this example creates one first, with the flag on, and then goes looking
// for it.
//
// **Metadata is the only thing about a stored completion you can change**, and
// it is a full replacement rather than a merge -- sending one key drops the
// others. It is also the thing worth setting at creation time: it is how a
// support ticket, a tenant or a feature flag gets attached to a completion, and
// retrofitting it means finding the completion again by hand.
//
// **Storage is not free of consequence.** A stored completion keeps the prompt
// and the answer on OpenAI's side until it is deleted, so anything that must
// not be retained should not be stored in the first place. This example deletes
// what it created unless told to keep it.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./stored_completions                # create one, list, tag, delete
//   ./stored_completions --keep         # ...and leave it there
//   ./stored_completions --list-only    # only browse what is already stored

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

// Stored completions are listed newest-first and there can be a great many of
// them, so the example shows a page rather than walking to the end. Iterating
// all of them is `Client::PageWalker` -- see examples/pagination.cpp.
constexpr int kPageSize = 5;

void printCompletion(QTextStream &out, const Core::ChatCompletionResponse &completion)
{
    out << "  " << completion.id() << "  " << completion.model() << "  "
        << QDateTime::fromSecsSinceEpoch(completion.created()).toString(Qt::ISODate) << "\n";

    const QJsonObject metadata = completion.metadata();
    if (!metadata.isEmpty()) {
        out << "      tags:";
        for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it)
            out << " " << it.key() << "=" << it.value().toString();
        out << "\n";
    }
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
    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example.\n";
        return 1;
    }

    const QStringList args = app.arguments();
    const bool keep = args.contains(QStringLiteral("--keep"));
    const bool listOnly = args.contains(QStringLiteral("--list-only"));

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    const auto onError = [&](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.exit(1);
    };

    // Last step: browse the page. Reached whether or not anything was created.
    const auto browse = [&] {
        Client::ListParams params;
        params.limit = kPageSize;
        Client::ChatCompletionListReply *list = client->listChatCompletions(params);
        QObject::connect(list, &Client::ChatCompletionListReply::failed, onError);
        QObject::connect(list, &Client::ChatCompletionListReply::finished,
                         [&](const Core::ChatCompletionList &page) {
                             out << "\nStored completions (" << page.size() << " of this page";
                             if (page.hasMore)
                                 out << ", more available";
                             out << "):\n";
                             for (const Core::ChatCompletionResponse &completion : page.data)
                                 printCompletion(out, completion);
                             if (page.data.isEmpty()) {
                                 out << "  (none -- a completion is only listed here if the\n"
                                     << "   request that created it set store: true)\n";
                             }
                             app.quit();
                         });
    };

    if (listOnly) {
        browse();
        return app.exec();
    }

    // Third step: delete what this run created, so the example leaves nothing
    // behind. --keep skips it.
    const auto cleanUp = [&](const QString &completionId) {
        if (keep) {
            out << "\nKept " << completionId << " (--keep). Delete it with:\n"
                << "  client.deleteChatCompletion(\"" << completionId << "\");\n";
            browse();
            return;
        }
        Client::ChatCompletionReply *gone = client->deleteChatCompletion(completionId);
        QObject::connect(gone, &Client::ChatCompletionReply::failed, onError);
        QObject::connect(gone, &Client::ChatCompletionReply::finished,
                         [&, completionId](const Core::ChatCompletionResponse &) {
                             out << "\nDeleted " << completionId << "\n";
                             browse();
                         });
    };

    // Second step: tag it. The reply carries the completion back with the tags
    // on it, which is how a caller confirms what actually stuck.
    const auto tag = [&](const QString &completionId) {
        QJsonObject metadata;
        metadata.insert(QStringLiteral("example"), QStringLiteral("stored_completions"));
        metadata.insert(QStringLiteral("run"),
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

        Client::ChatCompletionReply *tagged = client->updateChatCompletion(completionId, metadata);
        QObject::connect(tagged, &Client::ChatCompletionReply::failed, onError);
        QObject::connect(tagged, &Client::ChatCompletionReply::finished,
                         [&](const Core::ChatCompletionResponse &completion) {
                             out << "\nTagged:\n";
                             printCompletion(out, completion);
                             // A second update with a different key would drop
                             // both of these -- the write replaces the whole
                             // metadata object rather than merging into it.
                             cleanUp(completion.id());
                         });
    };

    // First step: produce something to find. `store` is what puts it on the
    // management surface; setting the metadata here rather than in the update
    // above is what a real application would do.
    Core::ChatCompletionRequest request(
            model, {Core::Message::user(QStringLiteral("Say hello in five words."))});
    request.setStore(true);
    request.setMaxCompletionTokens(32);

    Client::ChatCompletionReply *created = client->createChatCompletion(request);
    QObject::connect(created, &Client::ChatCompletionReply::failed, onError);
    QObject::connect(created, &Client::ChatCompletionReply::finished,
                     [&](const Core::ChatCompletionResponse &completion) {
                         out << "Created " << completion.id() << ": "
                             << completion.firstMessage().content() << "\n";
                         // The create reply does not echo metadata even when the
                         // request set it; the management endpoints do. That is
                         // why the tag below is read back rather than assumed.
                         tag(completion.id());
                     });

    return app.exec();
}
