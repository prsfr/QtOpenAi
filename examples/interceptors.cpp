// SPDX-License-Identifier: MIT
//
// Hooks around every request the client makes: a redacting log, a per-request
// trace header, and a cache that answers the second identical question without
// asking the provider again.
//
//   client.addInterceptor(&logger);
//   client.addInterceptor(&trace);
//   client.addInterceptor(&cache);
//
// Interceptors run in installation order on the way out and in reverse on the
// way back, so the first one installed is the outermost.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./interceptors "What is the capital of France?"
//
// Asks the same question twice. The first goes to the provider; the second is
// answered from the cache -- same answer, no round trip, no bill.
//
// Note what the log does *not* contain: the API key is replaced with
// <redacted> before anything is written. Bodies are off by default because they
// carry the prompt; this example turns them on to show the truncation, which is
// a decision to make deliberately rather than by accident.

#include <QtOpenAi/Client/CachingInterceptor.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/Interceptor.h>
#include <QtOpenAi/Client/LoggingInterceptor.h>
#include <QtOpenAi/Client/ResponseCache.h>

#include "ExampleSupport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QUuid>

using namespace QtOpenAi;

// A header whose value differs per request is the case Client::setDefaultHeader()
// cannot serve -- and the whole reason beforeRequest() may modify the request.
// A constant header does not belong here.
class TraceInterceptor : public Client::Interceptor
{
public:
    std::optional<Client::InterceptedResponse>
    beforeRequest(Client::InterceptedRequest &request) override
    {
        request.request.setRawHeader("X-Trace-Id",
                                     QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());
        return std::nullopt;
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const std::optional<ExampleEnv> env = exampleEnv(out);
    if (!env)
        return 1;

    const QString question = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                      : QStringLiteral("What is the capital of France?");
    const QString model = env->model(QStringLiteral("gpt-4o-mini"));

    Client::Client client(env->baseUrl, env->key);

    // Outermost, so it sees the request after the others have had their say and
    // the response before they unwind.
    Client::LoggingInterceptor logger;
    logger.setLogBodies(true);
    logger.setMaxBodyLength(120);
    QObject::connect(&logger, &Client::LoggingInterceptor::logged,
                     [&out](const QString &line) { out << line << "\n"; });
    client.addInterceptor(&logger);

    TraceInterceptor trace;
    client.addInterceptor(&trace);

    Client::CachingInterceptor cache;
    // Deterministic answers stay valid longer than chatty ones; five minutes is
    // the default and this is the knob for saying otherwise.
    static_cast<Client::MemoryResponseCache *>(cache.cache())->setTtlSeconds(60);
    QObject::connect(&cache, &Client::CachingInterceptor::hit,
                     [&out](const QUrl &) { out << "[cache] hit -- no round trip\n"; });
    QObject::connect(&cache, &Client::CachingInterceptor::missed,
                     [&out](const QUrl &) { out << "[cache] miss\n"; });
    client.addInterceptor(&cache);

    const Core::ChatCompletionRequest request(model, {Core::Message::user(question)});

    // temperature is left at the provider's default deliberately: caching a
    // deliberately random answer would be the wrong thing to demonstrate as
    // well as the wrong thing to do.
    const auto askTwice = [&](auto &&self, int remaining) -> void {
        Client::ChatCompletionReply *reply = client.createChatCompletion(request);
        QObject::connect(reply, &Client::ChatCompletionReply::finished,
                         [&out, &self, remaining](const Core::ChatCompletionResponse &response) {
                             out << "\n"
                                 << response.choices().value(0).message().content() << "\n\n";
                             if (remaining > 1)
                                 self(self, remaining - 1);
                             else
                                 QCoreApplication::quit();
                         });
        QObject::connect(reply, &Client::ChatCompletionReply::failed, failAndExit(out, app));
    };
    askTwice(askTwice, 2);

    return app.exec();
}
