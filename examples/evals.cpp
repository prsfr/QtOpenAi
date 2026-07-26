// SPDX-License-Identifier: MIT
//
// Score a model against your own test set (the /evals endpoints). An eval is a
// reusable definition — the shape of the items plus the graders — and a run
// executes it against a data source:
//   1. createEval()             — POST /evals
//   2. createEvalRun()          — POST /evals/{id}/runs
//   3. pollEvalRun()            — GET .../runs/{run_id} until terminal
//   4. listEvalRunOutputItems() — the per-item verdicts
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./evals
//
// The eval below checks that the model answers a handful of capital-city
// questions exactly, using the built-in `string_check` grader.

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

// One test item: the question to ask and the answer it must produce.
QJsonObject item(const QString &question, const QString &answer)
{
    return QJsonObject {
            {QStringLiteral("item"), QJsonObject {{QStringLiteral("question"), question},
                                                  {QStringLiteral("answer"), answer}}},
    };
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

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    auto reportError = [&out, &app](const Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.exit(1);
    };

    // 1. The config describes the items; the criteria describe the grading.
    // Both are pass-through JSON, matching the open unions in the API.
    const QJsonObject dataSourceConfig {
            {QStringLiteral("type"), QStringLiteral("custom")},
            {QStringLiteral("item_schema"),
             QJsonObject {
                     {QStringLiteral("type"), QStringLiteral("object")},
                     {QStringLiteral("properties"),
                      QJsonObject {
                              {QStringLiteral("question"),
                               QJsonObject {{QStringLiteral("type"), QStringLiteral("string")}}},
                              {QStringLiteral("answer"),
                               QJsonObject {{QStringLiteral("type"), QStringLiteral("string")}}}}},
                     {QStringLiteral("required"),
                      QJsonArray {QStringLiteral("question"), QStringLiteral("answer")}}}},
            {QStringLiteral("include_sample_schema"), true},
    };
    const QJsonArray testingCriteria {QJsonObject {
            {QStringLiteral("type"), QStringLiteral("string_check")},
            {QStringLiteral("name"), QStringLiteral("exact match")},
            {QStringLiteral("operation"), QStringLiteral("eq")},
            {QStringLiteral("input"), QStringLiteral("{{sample.output_text}}")},
            {QStringLiteral("reference"), QStringLiteral("{{item.answer}}")},
    }};

    Core::CreateEvalRequest request(dataSourceConfig, testingCriteria);
    request.setName(QStringLiteral("Capital cities"));

    Client::EvalReply *created = client->createEval(request);
    QObject::connect(created, &Client::EvalReply::failed, reportError);

    QObject::connect(
            created, &Client::EvalReply::finished,
            [&out, &app, client, model, reportError](const Core::Eval &eval) {
                out << "Created eval " << eval.id() << " (" << eval.name() << ")\n";
                out.flush();

                // 2. Run it: the data source pairs the items with the model to
                // test and the prompt template that turns an item into a call.
                const QJsonObject dataSource {
                        {QStringLiteral("type"), QStringLiteral("completions")},
                        {QStringLiteral("model"), model},
                        {QStringLiteral("input_messages"),
                         QJsonObject {
                                 {QStringLiteral("type"), QStringLiteral("template")},
                                 {QStringLiteral("template"),
                                  QJsonArray {QJsonObject {
                                          {QStringLiteral("role"), QStringLiteral("user")},
                                          {QStringLiteral("content"),
                                           QStringLiteral("{{item.question}} Answer with the city "
                                                          "name only.")}}}}}},
                        {QStringLiteral("source"),
                         QJsonObject {
                                 {QStringLiteral("type"), QStringLiteral("file_content")},
                                 {QStringLiteral("content"),
                                  QJsonArray {item(QStringLiteral("What is the capital of France?"),
                                                   QStringLiteral("Paris")),
                                              item(QStringLiteral("What is the capital of Japan?"),
                                                   QStringLiteral("Tokyo")),
                                              item(QStringLiteral("What is the capital of Norway?"),
                                                   QStringLiteral("Oslo"))}}}},
                };

                Core::CreateEvalRunRequest runRequest(dataSource);
                runRequest.setName(QStringLiteral("example run"));

                Client::EvalRunReply *started = client->createEvalRun(eval.id(), runRequest);
                QObject::connect(started, &Client::EvalRunReply::failed, reportError);

                QObject::connect(
                        started, &Client::EvalRunReply::finished,
                        [&out, &app, client, reportError](const Core::EvalRun &run) {
                            out << "Started run " << run.id() << "\n";
                            out.flush();

                            // 3. Wait for the graders to finish.
                            Client::EvalRunPoller *poller
                                    = client->pollEvalRun(run.evalId(), run.id(), 5000);
                            QObject::connect(poller, &Client::EvalRunPoller::failed, reportError);

                            QObject::connect(poller, &Client::EvalRunPoller::progressed,
                                             [&out](const Core::EvalRun &state) {
                                                 const Core::EvalResultCounts counts
                                                         = state.resultCounts();
                                                 out << "  "
                                                     << Core::evalRunStatusToString(state.status())
                                                     << " — " << counts.passed << " passed, "
                                                     << counts.failed << " failed of "
                                                     << counts.total << "\n";
                                                 out.flush();
                                             });

                            QObject::connect(
                                    poller, &Client::EvalRunPoller::completed,
                                    [&out, &app, client, reportError](const Core::EvalRun &done) {
                                        if (!done.reportUrl().isEmpty())
                                            out << "Report: " << done.reportUrl() << "\n";
                                        out.flush();

                                        // 4. Which items passed, and why.
                                        Client::EvalRunOutputItemListReply *items
                                                = client->listEvalRunOutputItems(done.evalId(),
                                                                                 done.id());
                                        QObject::connect(
                                                items, &Client::EvalRunOutputItemListReply::failed,
                                                reportError);
                                        QObject::connect(
                                                items,
                                                &Client::EvalRunOutputItemListReply::finished,
                                                [&out,
                                                 &app](const Core::EvalRunOutputItemList &list) {
                                                    out << "--- items ---\n";
                                                    for (const Core::EvalRunOutputItem &item :
                                                         list.data)
                                                        out << "  " << item.id() << ": "
                                                            << item.status() << "\n";
                                                    app.quit();
                                                });
                                    });

                            poller->start();
                        });
            });

    return app.exec();
}
