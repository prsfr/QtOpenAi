// SPDX-License-Identifier: MIT
//
// Throttling the client so the provider does not have to.
//
//   RateLimiter limiter;
//   limiter.setMaxConcurrent(3);
//   limiter.setRequestsPerMinute(20);
//   client.setRateLimiter(&limiter);
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./rate_limiting 12          # how many questions to ask at once
//
// Fires every request at once and lets the limiter sort it out. Calling code
// does not change at all -- each call returns its reply immediately, the reply
// simply has not started yet. The queue depth is printed as it drains, which is
// the only visible difference between this and the unthrottled version.
//
// A 429 carrying Retry-After pauses the whole client rather than just the reply
// that received it: the provider is saying that *you* are going too fast.

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/RateLimiter.h>

#include "ExampleSupport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const std::optional<ExampleEnv> env = exampleEnv(out);
    if (!env)
        return 1;

    const int count = argc > 1 ? QString::fromLocal8Bit(argv[1]).toInt() : 12;
    const QString model = env->model(QStringLiteral("gpt-4o-mini"));

    Client::Client client(env->baseUrl, env->key);

    Client::RateLimiter limiter;
    // Three at a time. Also bounds how many sockets and how much memory a burst
    // of a hundred questions can take, which matters even where the provider
    // would have allowed it.
    limiter.setMaxConcurrent(3);
    limiter.setRequestsPerMinute(20);
    client.setRateLimiter(&limiter);

    QObject::connect(&limiter, &Client::RateLimiter::queueChanged,
                     [&out](int queued, int inFlight) {
                         out << "[limiter] " << inFlight << " running, " << queued << " waiting\n";
                         out.flush();
                     });
    QObject::connect(&limiter, &Client::RateLimiter::pausedFor, [&out](int msecs) {
        out << "[limiter] the provider asked for " << msecs << " ms; holding everything back\n";
    });

    QElapsedTimer elapsed;
    elapsed.start();
    int settled = 0;
    int failures = 0;

    for (int i = 0; i < count; ++i) {
        const Core::ChatCompletionRequest request(
                model,
                {Core::Message::user(QStringLiteral("In one word: what is %1 squared?").arg(i))});

        Client::ChatCompletionReply *reply = client.createChatCompletion(request);
        QObject::connect(reply, &Client::ChatCompletionReply::finished,
                         [&out, i](const Core::ChatCompletionResponse &response) {
                             out << i << ": " << response.choices().value(0).message().content()
                                 << "\n";
                         });
        QObject::connect(reply, &Client::ChatCompletionReply::failed,
                         [&out, &failures, i](const Client::ClientError &error) {
                             out << i << ": failed -- " << error.message() << "\n";
                             ++failures;
                         });
        QObject::connect(reply, &Client::ChatCompletionReply::done, [&]() {
            if (++settled < count)
                return;
            out << "\n"
                << count << " requests in " << elapsed.elapsed() << " ms, never more than "
                << limiter.maxConcurrent() << " at once.\n";
            app.exit(failures > 0 ? 1 : 0);
        });
    }

    return app.exec();
}
