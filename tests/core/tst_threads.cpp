// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/CreateRunRequest.h>
#include <QtOpenAi/Core/CreateThreadRequest.h>
#include <QtOpenAi/Core/Run.h>
#include <QtOpenAi/Core/RunStep.h>
#include <QtOpenAi/Core/Thread.h>
#include <QtOpenAi/Core/ThreadMessage.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the thread half of the Assistants beta (#24): threads, their
// messages, runs — including the `requires_action` state that hands tool calls
// back to the client — and run steps, plus the request bodies.
class TestThreads : public QObject
{
    Q_OBJECT
private slots:
    void parsesThread();
    void threadRoundTrip();
    void parsesThreadDeletionAcknowledgement();
    void parsesMessage();
    void messageRoundTrip();
    void concatenatesTextParts();
    void parsesRun();
    void parsesRequiresActionRun();
    void runRoundTrip();
    void reportsTerminalStatus_data();
    void reportsTerminalStatus();
    void parsesRunStep();
    void runStepRoundTrip();
    void serialisesToolOutputs();
    void messageInputSerialisesTextOrParts();
    void createThreadRequestSerialisesBody();
    void createRunRequestSerialisesBody();
    void createRunRequestNestsThread();
    void createRequestsOmitUnsetFields();
};

namespace {

QJsonArray textContent(const QString &value)
{
    return QJsonArray {QJsonObject {
            {QStringLiteral("type"), QStringLiteral("text")},
            {QStringLiteral("text"), QJsonObject {{QStringLiteral("value"), value},
                                                  {QStringLiteral("annotations"), QJsonArray {}}}},
    }};
}

} // namespace

void TestThreads::parsesThread()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("thread_abc123")},
            {QStringLiteral("object"), QStringLiteral("thread")},
            {QStringLiteral("created_at"), 1716028800},
            {QStringLiteral("tool_resources"),
             QJsonObject {{QStringLiteral("code_interpreter"),
                           QJsonObject {{QStringLiteral("file_ids"),
                                         QJsonArray {QStringLiteral("file-1")}}}}}},
            {QStringLiteral("metadata"),
             QJsonObject {{QStringLiteral("user"), QStringLiteral("u1")}}},
    };

    const Thread thread = Thread::fromJson(json);
    QCOMPARE(thread.id(), QStringLiteral("thread_abc123"));
    QCOMPARE(thread.object(), QStringLiteral("thread"));
    QCOMPARE(thread.createdAt(), Q_INT64_C(1716028800));
    QVERIFY(thread.toolResources().contains(QStringLiteral("code_interpreter")));
    QCOMPARE(thread.metadata().value(QStringLiteral("user")).toString(), QStringLiteral("u1"));
}

void TestThreads::threadRoundTrip()
{
    Thread thread;
    thread.setId(QStringLiteral("thread_1"));
    thread.setObject(QStringLiteral("thread"));
    thread.setCreatedAt(1700000000);
    thread.setToolResources(QJsonObject {{QStringLiteral("file_search"), QJsonObject {}}});
    thread.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});

    QCOMPARE(Thread::fromJson(thread.toJson()), thread);
}

void TestThreads::parsesThreadDeletionAcknowledgement()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("thread_abc123")},
            {QStringLiteral("object"), QStringLiteral("thread.deleted")},
            {QStringLiteral("deleted"), true},
    };

    const Thread thread = Thread::fromJson(json);
    QCOMPARE(thread.id(), QStringLiteral("thread_abc123"));
    QCOMPARE(thread.object(), QStringLiteral("thread.deleted"));
}

void TestThreads::parsesMessage()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("msg_abc123")},
            {QStringLiteral("object"), QStringLiteral("thread.message")},
            {QStringLiteral("created_at"), 1716028900},
            {QStringLiteral("thread_id"), QStringLiteral("thread_abc123")},
            {QStringLiteral("status"), QStringLiteral("completed")},
            {QStringLiteral("completed_at"), 1716028950},
            {QStringLiteral("role"), QStringLiteral("assistant")},
            {QStringLiteral("content"), textContent(QStringLiteral("It is sunny."))},
            {QStringLiteral("assistant_id"), QStringLiteral("asst_1")},
            {QStringLiteral("run_id"), QStringLiteral("run_1")},
            {QStringLiteral("attachments"),
             QJsonArray {QJsonObject {{QStringLiteral("file_id"), QStringLiteral("file-1")}}}},
            {QStringLiteral("metadata"), QJsonObject {}},
    };

    const ThreadMessage message = ThreadMessage::fromJson(json);
    QCOMPARE(message.id(), QStringLiteral("msg_abc123"));
    QCOMPARE(message.threadId(), QStringLiteral("thread_abc123"));
    // Per-message delivery state is a string, unrelated to the run's status set.
    QCOMPARE(message.status(), QStringLiteral("completed"));
    QCOMPARE(message.completedAt(), Q_INT64_C(1716028950));
    QCOMPARE(message.role(), Role::Assistant);
    QCOMPARE(message.assistantId(), QStringLiteral("asst_1"));
    QCOMPARE(message.runId(), QStringLiteral("run_1"));
    QCOMPARE(message.attachments().size(), 1);
    QCOMPARE(message.text(), QStringLiteral("It is sunny."));
}

void TestThreads::messageRoundTrip()
{
    ThreadMessage message;
    message.setId(QStringLiteral("msg_1"));
    message.setObject(QStringLiteral("thread.message"));
    message.setCreatedAt(1700000000);
    message.setThreadId(QStringLiteral("thread_1"));
    message.setStatus(QStringLiteral("incomplete"));
    message.setIncompleteDetails(
            QJsonObject {{QStringLiteral("reason"), QStringLiteral("run_expired")}});
    message.setIncompleteAt(1700000100);
    message.setRole(Role::User);
    message.setContent(textContent(QStringLiteral("Hello")));
    message.setAssistantId(QStringLiteral("asst_1"));
    message.setRunId(QStringLiteral("run_1"));
    message.setAttachments(
            QJsonArray {QJsonObject {{QStringLiteral("file_id"), QStringLiteral("file-1")}}});
    message.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});

    QCOMPARE(ThreadMessage::fromJson(message.toJson()), message);
}

void TestThreads::concatenatesTextParts()
{
    // The Assistants text part nests its value under `text`, and a message may
    // carry several parts plus non-text ones.
    QJsonArray content = textContent(QStringLiteral("Hello "));
    content.append(
            QJsonObject {{QStringLiteral("type"), QStringLiteral("image_file")},
                         {QStringLiteral("image_file"),
                          QJsonObject {{QStringLiteral("file_id"), QStringLiteral("file-1")}}}});
    content.append(textContent(QStringLiteral("world")).first());

    ThreadMessage message;
    message.setContent(content);
    QCOMPARE(message.text(), QStringLiteral("Hello world"));

    // A message with no text part yields an empty string, not a null one.
    ThreadMessage imageOnly;
    imageOnly.setContent(
            QJsonArray {QJsonObject {{QStringLiteral("type"), QStringLiteral("image_file")}}});
    QVERIFY(imageOnly.text().isEmpty());
}

void TestThreads::parsesRun()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("run_abc123")},
            {QStringLiteral("object"), QStringLiteral("thread.run")},
            {QStringLiteral("created_at"), 1716028800},
            {QStringLiteral("thread_id"), QStringLiteral("thread_abc123")},
            {QStringLiteral("assistant_id"), QStringLiteral("asst_abc123")},
            {QStringLiteral("status"), QStringLiteral("completed")},
            {QStringLiteral("started_at"), 1716028810},
            {QStringLiteral("completed_at"), 1716028820},
            {QStringLiteral("model"), QStringLiteral("gpt-4o-mini")},
            {QStringLiteral("instructions"), QStringLiteral("Be concise.")},
            {QStringLiteral("tools"),
             QJsonArray {QJsonObject {{QStringLiteral("type"), QStringLiteral("file_search")}}}},
            {QStringLiteral("usage"), QJsonObject {{QStringLiteral("prompt_tokens"), 12},
                                                   {QStringLiteral("completion_tokens"), 8},
                                                   {QStringLiteral("total_tokens"), 20}}},
            {QStringLiteral("temperature"), 0.5},
            {QStringLiteral("max_completion_tokens"), 512},
            {QStringLiteral("truncation_strategy"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("auto")}}},
            {QStringLiteral("parallel_tool_calls"), true},
    };

    const Run run = Run::fromJson(json);
    QCOMPARE(run.id(), QStringLiteral("run_abc123"));
    QCOMPARE(run.threadId(), QStringLiteral("thread_abc123"));
    QCOMPARE(run.assistantId(), QStringLiteral("asst_abc123"));
    QCOMPARE(run.status(), RunStatus::Completed);
    QCOMPARE(run.startedAt(), Q_INT64_C(1716028810));
    QCOMPARE(run.completedAt(), Q_INT64_C(1716028820));
    QCOMPARE(run.model(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(run.tools().size(), 1);
    QCOMPARE(run.usage().totalTokens(), 20);
    QCOMPARE(run.temperature().value(), 0.5);
    QCOMPARE(run.maxCompletionTokens().value(), 512);
    QVERIFY(!run.maxPromptTokens().has_value());
    QCOMPARE(run.parallelToolCalls().value(), true);
    QVERIFY(run.isTerminal());
    QVERIFY(!run.requiresAction());
}

void TestThreads::parsesRequiresActionRun()
{
    // The state the whole tool loop turns on: the run parks and hands back the
    // calls it wants answered, in the same shape the chat path produces.
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("run_abc123")},
            {QStringLiteral("thread_id"), QStringLiteral("thread_abc123")},
            {QStringLiteral("status"), QStringLiteral("requires_action")},
            {QStringLiteral("required_action"),
             QJsonObject {
                     {QStringLiteral("type"), QStringLiteral("submit_tool_outputs")},
                     {QStringLiteral("submit_tool_outputs"),
                      QJsonObject {
                              {QStringLiteral("tool_calls"),
                               QJsonArray {
                                       QJsonObject {
                                               {QStringLiteral("id"), QStringLiteral("call_1")},
                                               {QStringLiteral("type"), QStringLiteral("function")},
                                               {QStringLiteral("function"),
                                                QJsonObject {
                                                        {QStringLiteral("name"),
                                                         QStringLiteral("get_weather")},
                                                        {QStringLiteral("arguments"),
                                                         QStringLiteral("{\"city\":\"Oslo\"}")}}}},
                               }}}}}},
    };

    const Run run = Run::fromJson(json);
    QCOMPARE(run.status(), RunStatus::RequiresAction);
    QVERIFY(run.requiresAction());
    // requires_action is neither transient nor terminal: polling must stop, but
    // the run is not over.
    QVERIFY(!run.isTerminal());
    QCOMPARE(run.requiredActionType(), QStringLiteral("submit_tool_outputs"));
    QCOMPARE(run.requiredToolCalls().size(), 1);
    const ToolCall call = run.requiredToolCalls().first();
    QCOMPARE(call.id(), QStringLiteral("call_1"));
    QCOMPARE(call.function().name(), QStringLiteral("get_weather"));
    QCOMPARE(call.function().arguments(), QStringLiteral("{\"city\":\"Oslo\"}"));
}

void TestThreads::runRoundTrip()
{
    Run run;
    run.setId(QStringLiteral("run_1"));
    run.setObject(QStringLiteral("thread.run"));
    run.setCreatedAt(1700000000);
    run.setThreadId(QStringLiteral("thread_1"));
    run.setAssistantId(QStringLiteral("asst_1"));
    run.setStatus(RunStatus::RequiresAction);
    run.setRequiredActionType(QStringLiteral("submit_tool_outputs"));
    run.setRequiredToolCalls(
            {ToolCall(QStringLiteral("call_1"),
                      FunctionCall(QStringLiteral("get_weather"), QStringLiteral("{}")))});
    run.setErrorCode(QStringLiteral("server_error"));
    run.setErrorMessage(QStringLiteral("it broke"));
    run.setIncompleteDetails(
            QJsonObject {{QStringLiteral("reason"), QStringLiteral("max_prompt_tokens")}});
    run.setExpiresAt(1700000600);
    run.setStartedAt(1700000010);
    run.setCancelledAt(1700000020);
    run.setFailedAt(1700000030);
    run.setCompletedAt(1700000040);
    run.setModel(QStringLiteral("gpt-4o-mini"));
    run.setInstructions(QStringLiteral("Be brief."));
    run.setTools(QJsonArray {QJsonObject {{QStringLiteral("type"), QStringLiteral("function")}}});
    run.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});
    Usage usage;
    usage.setPromptTokens(3);
    usage.setCompletionTokens(4);
    usage.setTotalTokens(7);
    run.setUsage(usage);
    run.setTemperature(0.3);
    run.setTopP(0.8);
    run.setMaxPromptTokens(1000);
    run.setMaxCompletionTokens(200);
    run.setTruncationStrategy(QJsonObject {{QStringLiteral("type"), QStringLiteral("auto")}});
    run.setToolChoice(QStringLiteral("auto"));
    run.setParallelToolCalls(false);
    run.setResponseFormat(QJsonObject {{QStringLiteral("type"), QStringLiteral("text")}});

    QCOMPARE(Run::fromJson(run.toJson()), run);
}

void TestThreads::reportsTerminalStatus_data()
{
    QTest::addColumn<QString>("wireStatus");
    QTest::addColumn<bool>("terminal");

    QTest::newRow("queued") << QStringLiteral("queued") << false;
    QTest::newRow("in_progress") << QStringLiteral("in_progress") << false;
    QTest::newRow("cancelling") << QStringLiteral("cancelling") << false;
    // Parked, not finished — the client has to act before the run continues.
    QTest::newRow("requires_action") << QStringLiteral("requires_action") << false;
    QTest::newRow("completed") << QStringLiteral("completed") << true;
    QTest::newRow("failed") << QStringLiteral("failed") << true;
    QTest::newRow("cancelled") << QStringLiteral("cancelled") << true;
    QTest::newRow("incomplete") << QStringLiteral("incomplete") << true;
    QTest::newRow("expired") << QStringLiteral("expired") << true;
    // An unknown value decodes to the initial state, so polling continues.
    QTest::newRow("unknown") << QStringLiteral("something_new") << false;
}

void TestThreads::reportsTerminalStatus()
{
    QFETCH(QString, wireStatus);
    QFETCH(bool, terminal);

    const Run run = Run::fromJson(QJsonObject {{QStringLiteral("status"), wireStatus}});
    QCOMPARE(run.isTerminal(), terminal);
}

void TestThreads::parsesRunStep()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("step_abc123")},
            {QStringLiteral("object"), QStringLiteral("thread.run.step")},
            {QStringLiteral("created_at"), 1716028800},
            {QStringLiteral("assistant_id"), QStringLiteral("asst_1")},
            {QStringLiteral("thread_id"), QStringLiteral("thread_1")},
            {QStringLiteral("run_id"), QStringLiteral("run_1")},
            {QStringLiteral("type"), QStringLiteral("tool_calls")},
            {QStringLiteral("status"), QStringLiteral("completed")},
            {QStringLiteral("step_details"),
             QJsonObject {{QStringLiteral("type"), QStringLiteral("tool_calls")},
                          {QStringLiteral("tool_calls"), QJsonArray {}}}},
            {QStringLiteral("last_error"),
             QJsonObject {{QStringLiteral("code"), QStringLiteral("server_error")},
                          {QStringLiteral("message"), QStringLiteral("tool crashed")}}},
            {QStringLiteral("completed_at"), 1716028830},
            {QStringLiteral("usage"), QJsonObject {{QStringLiteral("total_tokens"), 42}}},
    };

    const RunStep step = RunStep::fromJson(json);
    QCOMPARE(step.id(), QStringLiteral("step_abc123"));
    QCOMPARE(step.runId(), QStringLiteral("run_1"));
    QCOMPARE(step.type(), QStringLiteral("tool_calls"));
    QCOMPARE(step.status(), RunStepStatus::Completed);
    QCOMPARE(step.stepDetails().value(QStringLiteral("type")).toString(),
             QStringLiteral("tool_calls"));
    QCOMPARE(step.errorCode(), QStringLiteral("server_error"));
    QCOMPARE(step.errorMessage(), QStringLiteral("tool crashed"));
    QCOMPARE(step.completedAt(), Q_INT64_C(1716028830));
    QCOMPARE(step.usage().totalTokens(), 42);
    QVERIFY(step.isTerminal());

    const RunStep running = RunStep::fromJson(
            QJsonObject {{QStringLiteral("status"), QStringLiteral("in_progress")}});
    QVERIFY(!running.isTerminal());
}

void TestThreads::runStepRoundTrip()
{
    RunStep step;
    step.setId(QStringLiteral("step_1"));
    step.setObject(QStringLiteral("thread.run.step"));
    step.setCreatedAt(1700000000);
    step.setAssistantId(QStringLiteral("asst_1"));
    step.setThreadId(QStringLiteral("thread_1"));
    step.setRunId(QStringLiteral("run_1"));
    step.setType(QStringLiteral("message_creation"));
    step.setStatus(RunStepStatus::Failed);
    step.setStepDetails(QJsonObject {{QStringLiteral("type"), QStringLiteral("message_creation")}});
    step.setErrorCode(QStringLiteral("rate_limit_exceeded"));
    step.setErrorMessage(QStringLiteral("slow down"));
    step.setExpiredAt(1700000100);
    step.setCancelledAt(1700000200);
    step.setFailedAt(1700000300);
    step.setCompletedAt(1700000400);
    Usage usage;
    usage.setTotalTokens(9);
    step.setUsage(usage);
    step.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});

    QCOMPARE(RunStep::fromJson(step.toJson()), step);
}

void TestThreads::serialisesToolOutputs()
{
    ToolOutput output;
    output.toolCallId = QStringLiteral("call_1");
    output.output = QStringLiteral("{\"temp\":17}");

    const QJsonObject json = output.toJson();
    QCOMPARE(json.value(QStringLiteral("tool_call_id")).toString(), QStringLiteral("call_1"));
    QCOMPARE(json.value(QStringLiteral("output")).toString(), QStringLiteral("{\"temp\":17}"));
    QCOMPARE(ToolOutput::fromJson(json), output);
}

void TestThreads::messageInputSerialisesTextOrParts()
{
    // The common case: a plain user turn goes out as a bare `content` string.
    ThreadMessageInput plain;
    plain.text = QStringLiteral("What is the weather?");
    const QJsonObject plainJson = plain.toJson();
    QCOMPARE(plainJson.value(QStringLiteral("role")).toString(), QStringLiteral("user"));
    QCOMPARE(plainJson.value(QStringLiteral("content")).toString(),
             QStringLiteral("What is the weather?"));

    // Content parts win over the plain text when both are present.
    ThreadMessageInput multimodal;
    multimodal.text = QStringLiteral("ignored");
    multimodal.content = textContent(QStringLiteral("look at this"));
    multimodal.attachments
            = QJsonArray {QJsonObject {{QStringLiteral("file_id"), QStringLiteral("file-1")}}};
    const QJsonObject multimodalJson = multimodal.toJson();
    QVERIFY(multimodalJson.value(QStringLiteral("content")).isArray());
    QCOMPARE(multimodalJson.value(QStringLiteral("attachments")).toArray().size(), 1);

    QCOMPARE(ThreadMessageInput::fromJson(plainJson), plain);
    QCOMPARE(ThreadMessageInput::fromJson(multimodalJson).content, multimodal.content);
}

void TestThreads::createThreadRequestSerialisesBody()
{
    CreateThreadRequest request;
    QVERIFY(request.isEmpty());

    request.addUserMessage(QStringLiteral("Hello"));
    request.setToolResources(QJsonObject {{QStringLiteral("file_search"), QJsonObject {}}});
    request.setMetadata(QJsonObject {{QStringLiteral("k"), QStringLiteral("v")}});
    QVERIFY(!request.isEmpty());

    const QJsonObject json = request.toJson();
    const QJsonArray messages = json.value(QStringLiteral("messages")).toArray();
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages.first().toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("Hello"));
    QVERIFY(json.contains(QStringLiteral("tool_resources")));
    QVERIFY(json.contains(QStringLiteral("metadata")));
}

void TestThreads::createRunRequestSerialisesBody()
{
    CreateRunRequest request(QStringLiteral("asst_1"));
    request.setModel(QStringLiteral("gpt-4o-mini"));
    request.setInstructions(QStringLiteral("Answer in French."));
    request.setAdditionalInstructions(QStringLiteral("Be brief."));
    request.addUserMessage(QStringLiteral("Bonjour"));
    request.addTool(Tool::function(QStringLiteral("get_weather"), QString(), QJsonObject {}));
    request.setTemperature(0.2);
    request.setMaxPromptTokens(2000);
    request.setToolChoice(QStringLiteral("required"));
    request.setParallelToolCalls(false);
    request.setResponseFormat(ResponseFormat::jsonObject());

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("assistant_id")).toString(), QStringLiteral("asst_1"));
    QCOMPARE(json.value(QStringLiteral("model")).toString(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(json.value(QStringLiteral("additional_instructions")).toString(),
             QStringLiteral("Be brief."));
    QCOMPARE(json.value(QStringLiteral("additional_messages")).toArray().size(), 1);
    QCOMPARE(json.value(QStringLiteral("tools")).toArray().size(), 1);
    QCOMPARE(json.value(QStringLiteral("temperature")).toDouble(), 0.2);
    QCOMPARE(json.value(QStringLiteral("max_prompt_tokens")).toInt(), 2000);
    QCOMPARE(json.value(QStringLiteral("tool_choice")).toString(), QStringLiteral("required"));
    QCOMPARE(json.value(QStringLiteral("parallel_tool_calls")).toBool(), false);
    QVERIFY(json.contains(QStringLiteral("response_format")));
    // Streaming is off unless the streaming request path turns it on.
    QVERIFY(!json.contains(QStringLiteral("stream")));
}

void TestThreads::createRunRequestNestsThread()
{
    // POST /threads/runs creates the thread and the run in one body.
    CreateThreadRequest thread;
    thread.addUserMessage(QStringLiteral("Hello"));

    CreateRunRequest request(QStringLiteral("asst_1"));
    request.setThread(thread);

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("thread"))
                     .toObject()
                     .value(QStringLiteral("messages"))
                     .toArray()
                     .size(),
             1);

    // An empty thread is left out rather than sent as {}.
    CreateRunRequest plain(QStringLiteral("asst_1"));
    QVERIFY(!plain.toJson().contains(QStringLiteral("thread")));
}

void TestThreads::createRequestsOmitUnsetFields()
{
    QVERIFY(CreateThreadRequest().toJson().isEmpty());

    const CreateRunRequest run(QStringLiteral("asst_1"));
    const QJsonObject json = run.toJson();
    QCOMPARE(json.size(), 1);
    QCOMPARE(json.value(QStringLiteral("assistant_id")).toString(), QStringLiteral("asst_1"));
}

QTEST_MAIN(TestThreads)
#include "tst_threads.moc"
