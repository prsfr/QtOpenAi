// SPDX-License-Identifier: MIT
//
// Walk every page of a cursor-paginated endpoint (#29). List endpoints return
// one page at a time — `has_more` plus a `last_id` cursor — and PageWalker turns
// that into an iterate-all: it feeds each page's last id back as the next
// `after` and stops when the server clears `has_more`.
//
// The same walker drives any list endpoint; only the two template arguments and
// the one-line fetch lambda change.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./pagination [purpose]        # optional Files-API purpose filter

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/PageWalker.h>

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

    const QString purpose = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();

    Client::Client client(env->baseUrl, env->key);

    // Every POST already carries a generated Idempotency-Key so the automatic
    // retries cannot be charged twice; this is on by default and shown here
    // only to make the knob visible.
    client.setIdempotencyKeysEnabled(true);

    // Small pages, so a modest account still shows the walker doing its job.
    Client::ListParams params;
    params.limit = 20;

    auto *walker = new Client::PageWalker<Client::FileListReply, Core::FileList>(
            [&client, purpose](const Client::ListParams &p) {
                return client.listFiles(p, purpose);
            },
            params, &app);

    int total = 0;
    walker->setPageHandler([&out, &total](const Core::FileList &page) {
        for (const Core::FileObject &file : page.data) {
            out << "  " << file.id() << "  " << file.filename() << "\n";
            ++total;
        }
        out.flush();
    });

    QObject::connect(walker, &Client::PageWalkerBase::finished, [&out, &app, walker, &total] {
        out << "Done: " << total << " files across " << walker->pageCount() << " page(s).\n";
        app.quit();
    });
    QObject::connect(walker, &Client::PageWalkerBase::failed, failAndExit(out, app));

    walker->start();
    return app.exec();
}
