// SPDX-License-Identifier: MIT
//
// What the client is costing you, and how it is behaving.
//
// One MetricsCollector attached to a Client records every request it makes --
// duration, outcome, HTTP status, retries, rate-limit headroom -- without any
// of the calling code knowing it is there:
//
//   metrics.attach(&client);
//
// Tokens and cost need one thing more. A reply is generic; only the typed
// response knows which model answered and what it spent, so observe() wraps the
// call:
//
//   metrics.observe(client.createChatCompletion(request));
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./metrics "Explain the Qt meta-object system."
//
// Streams the answer, so the report includes time to first token -- the wait a
// user actually perceives, as opposed to how long the whole stream ran.

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/MetricsCollector.h>

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

    const QString question = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                      : QStringLiteral("Explain the Qt meta-object system.");
    const QString model = env->model(QStringLiteral("gpt-4o-mini"));

    Client::Client client(env->baseUrl, env->key);

    Client::MetricsCollector metrics;
    metrics.attach(&client);

    QObject::connect(&metrics, &Client::MetricsCollector::requestRecorded,
                     [&out](const Client::RequestMetrics &request) {
                         out << "\n[" << (request.ok ? "ok" : "failed") << "] "
                             << request.durationMs << " ms";
                         if (request.timeToFirstTokenMs >= 0)
                             out << ", first token after " << request.timeToFirstTokenMs << " ms";
                         if (request.retryCount > 0)
                             out << ", " << request.retryCount << " retries";
                         if (request.rateLimit.remainingRequests >= 0)
                             out << ", " << request.rateLimit.remainingRequests
                                 << " requests left in the window";
                         out << "\n";
                     });

    Core::ChatCompletionRequest request(model, {Core::Message::user(question)});
    // Ask for usage on the stream as well; without it a streamed response
    // reports no tokens and there is nothing to cost.
    request.setStreamOptions({{QStringLiteral("include_usage"), true}});

    Client::ChatCompletionStreamReply *reply = client.createChatCompletionStream(request);
    // The typed reply is where the model and its usage appear.
    metrics.observe(reply);

    QObject::connect(reply, &Client::ChatCompletionStreamReply::contentDelta,
                     [&out](const QString &text) {
                         out << text;
                         out.flush();
                     });

    // done() follows failed(), so the exit status is decided there: the report
    // is printed either way, and a failed request still fails the run.
    bool failed = false;
    QObject::connect(reply, &Client::ChatCompletionStreamReply::failed,
                     [&out, &failed](const Client::ClientError &error) {
                         out << "\nError: " << error.message() << "\n";
                         failed = true;
                     });

    QObject::connect(reply, &Client::ChatCompletionStreamReply::done, [&]() {
        const Client::MetricsSnapshot snapshot = metrics.snapshot();
        const Client::ModelMetrics totals = snapshot.totals();

        out << "\n--- session ---\n";
        out << "Requests: " << snapshot.requests << " (" << snapshot.successes << " ok, "
            << snapshot.failures << " failed)\n";
        out << "Latency:  " << snapshot.averageDurationMs() << " ms average";
        if (snapshot.streamedRequests > 0)
            out << ", " << snapshot.averageTimeToFirstTokenMs() << " ms to first token";
        out << "\n";
        out << "Tokens:   " << totals.promptTokens << " in, " << totals.completionTokens
            << " out\n";
        // Zero here means the catalog has no price for this model, not that it
        // was free.
        out << "Cost:     $" << snapshot.cost() << "\n";

        app.exit(failed ? 1 : 0);
    });

    return app.exec();
}
