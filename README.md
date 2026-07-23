# QtOpenAi

A modular **Qt 6** client library for **OpenAI-compatible** chat completion
APIs, with first-class **tool calling** wired through Qt's meta-object system
(signals/slots and `QMetaObject::invokeMethod`).

The data model is derived from the official
[OpenAI OpenAPI specification](https://github.com/openai/openai-openapi) and
follows Qt conventions throughout: implicitly-shared value types, `d`-pointer
(pimpl) implementation hiding, no `get` prefix on getters, and a clean
`QtOpenAi::` namespace split into modules.

> Targets Qt 6.4+ (developed against the Qt 6.11 series). Works with OpenAI,
> Azure OpenAI, and local servers such as Ollama, vLLM and LM Studio.

## Modules

| Namespace            | Library           | Responsibility                                             |
|----------------------|-------------------|------------------------------------------------------------|
| `QtOpenAi::Core`     | `QtOpenAiCore`    | Value types & JSON (de)serialisation for the chat API.     |
| `QtOpenAi::Client`   | `QtOpenAiClient`  | Async networking `Client`, replies, and the `ToolRegistry`.|

`QtOpenAi::Client` depends on `QtOpenAi::Core`; `Core` has no dependency beyond
`Qt6::Core`.

## Design

* **SOTA data structures** — every model type (`Message`, `ToolCall`, `Tool`,
  `ChatCompletionRequest`, `ChatCompletionResponse`, …) is an *implicitly
  shared* (copy-on-write) value type backed by `QSharedDataPointer`. Cheap to
  copy, safe to pass by value, `==`/`!=` comparable.
* **Hidden implementation** — QObject-derived types (`Client`,
  `ChatCompletionReply`, `ToolRegistry`) use the classic `Q_DECLARE_PRIVATE`
  `d`-pointer so the ABI stays stable and headers stay clean.
* **Qt coding style** — getters are `content()` not `getContent()`; setters are
  `setContent()`; enums are exposed via `Q_ENUM`/`Q_NAMESPACE`.

## Tool calling via the Qt meta-object system

`QtOpenAi::Client::ToolRegistry` maps a model's tool calls back onto local C++
code in two interchangeable ways:

```cpp
using namespace QtOpenAi;

Client::ToolRegistry registry;

// The JSON-Schema is what advertises the parameters to the model: it tells the
// model that this tool takes integer arguments named "a" and "b". The handler's
// args keys must match these property names.
const QJsonObject addSchema {
    { "type", "object" },
    { "properties", QJsonObject {
        { "a", QJsonObject {{ "type", "integer" }, { "description", "First addend" }} },
        { "b", QJsonObject {{ "type", "integer" }, { "description", "Second addend" }} },
    }},
    { "required", QJsonArray { "a", "b" } },
};

// 1) std::function handler — reads the same "a"/"b" the schema declared
registry.registerFunction("add", "Add two integers", addSchema,
    [](const QJsonObject &args) {
        return QString::number(args["a"].toInt() + args["b"].toInt());
    });

// 2) A QObject slot dispatched by name through QMetaObject::invokeMethod
registry.registerMethod(weatherTool, weatherProvider, "getWeather");

// React to execution via signals
connect(&registry, &Client::ToolRegistry::toolInvoked,
        this, [](const QString &id, const QString &name, const QString &result) {
            qInfo() << name << "->" << result;
        });
```

> Hand-writing the schema is only needed until #39 lands — it will derive the
> `parameters` schema straight from a `Q_GADGET`/QObject via the meta-object
> system, so the property names can't drift from the handler.

Advertise the tools in a request and dispatch the model's calls:

```cpp
Core::ChatCompletionRequest request("gpt-4o-mini", { Core::Message::user(prompt) });
request.setTools(registry.tools());

auto *reply = client.createChatCompletion(request);
connect(reply, &Client::ChatCompletionReply::finished, this,
    [&](const Core::ChatCompletionResponse &response) {
        const auto calls = response.toolCalls();
        if (!calls.isEmpty()) {
            request.addMessage(response.firstMessage());        // assistant turn
            for (const auto &m : registry.invokeAll(calls))     // tool results
                request.addMessage(m);
            client.createChatCompletion(request);               // follow-up
        }
    });
```

A complete, runnable version lives in [`examples/tool_loop.cpp`](examples/tool_loop.cpp).

## Multimodal input

A `Message` can carry a plain string (as above) or an array of typed
`Core::ContentPart`s — text, images, audio, or files. The string API is
unchanged, so existing code keeps working:

```cpp
using namespace QtOpenAi::Core;

auto message = Message::user({
    ContentPart::text("What's in this image?"),
    ContentPart::imageUrl("https://example.com/cat.png", /*detail=*/"high"),
    // or a base64 data URI: ContentPart::imageUrl("data:image/png;base64,...")
});

ChatCompletionRequest request("gpt-4o", { message });
```

Audio (`ContentPart::inputAudio(base64, "wav")`) and file
(`ContentPart::file(fileId)`) parts work the same way. Assistant audio output on
a response message is exposed via `Message::audioId()` / `audioData()` /
`audioTranscript()`. A message serialises `content` as a string when it holds a
single string and as an array once it has parts.

## Streaming (Server-Sent Events)

Call `createChatCompletionStream()` for token-by-token output. The reply emits
`contentDelta()` for each text fragment and, when the stream ends, `finished()`
with the fully reassembled response (content concatenated, tool calls merged by
index):

```cpp
Core::ChatCompletionRequest request("gpt-4o-mini", { Core::Message::user(prompt) });

auto *stream = client.createChatCompletionStream(request);   // sets stream: true
connect(stream, &Client::ChatCompletionStreamReply::contentDelta,
        this, [](const QString &text) { std::cout << text.toStdString() << std::flush; });
connect(stream, &Client::ChatCompletionStreamReply::finished, this,
        [](const Core::ChatCompletionResponse &full) {
            // full.firstMessage().content() / full.toolCalls() are complete here
        });
connect(stream, &Client::ChatCompletionStreamReply::failed, this,
        [](const Client::ClientError &e) { qWarning() << e.message(); });
```

`Client::ChatCompletionAccumulator` performs the chunk→response reassembly and
can also be driven directly over `ChatCompletionChunk` deltas you collect
yourself.

The legacy `/completions` endpoint streams too: `createCompletionStream()`
returns a `CompletionStreamReply` emitting `textDelta()` and, on completion,
`finished(CompletionResponse)`. All three streaming replies share one internal
SSE-framing parser.

## Responses API (`/responses`)

The modern, unified Responses API is supported alongside Chat Completions. A
request takes an `input` (a plain string or a structured item array) and returns
a `Response` whose `output` is a list of typed items — assistant messages,
function calls, and reasoning summaries:

```cpp
Core::ResponseRequest request("gpt-5", "Tell me a joke");
request.setInstructions("Be concise");
request.setReasoningEffort("low");

auto *reply = client.createResponse(request);
connect(reply, &Client::ResponseReply::finished, this,
        [](const Core::Response &response) {
            qInfo().noquote() << response.outputText();   // assistant text
            for (const auto &call : response.functionCalls())
                qInfo() << call.name() << call.arguments();
        });
connect(reply, &Client::ResponseReply::failed, this,
        [](const Client::ClientError &e) { qWarning() << e.message(); });
```

Stored responses can be retrieved, cancelled, or deleted by id via
`getResponse()`, `cancelResponse()`, and `deleteResponse()`. Each returns a
`ResponseReply` sharing the same retry and rate-limit machinery as
`ChatCompletionReply`.

For streaming, `createResponseStream()` returns a `ResponseStreamReply` that
surfaces the typed event sequence — `outputTextDelta()` and
`functionCallArgumentsDelta()` for the common cases, `event(type, data)` for
everything else — and `finished(Response)` on `response.completed`:

```cpp
auto *stream = client.createResponseStream(request);   // sets stream: true
connect(stream, &Client::ResponseStreamReply::outputTextDelta,
        this, [](const QString &text) { std::cout << text.toStdString() << std::flush; });
connect(stream, &Client::ResponseStreamReply::finished, this,
        [](const Core::Response &full) { /* full.outputText() is complete here */ });
```

> The auxiliary endpoints (`input_items`, `compact`, `input_tokens`) are tracked
> separately and land in a follow-up; the request/response types, the
> create/get/cancel/delete endpoints, and streaming are covered here.

## Conversations API (`/conversations`)

Stateful conversations persist item history server-side for use with the
Responses API. Items reuse the Responses item model (`Core::ResponseOutputItem`):

```cpp
auto *created = client.createConversation(/*metadata*/ {},
        { Core::ResponseOutputItem::message("Hello", "user") });
connect(created, &Client::ConversationReply::finished, this,
        [&](const Core::Conversation &conv) {
            auto *items = client.listConversationItems(conv.id());
            connect(items, &Client::ConversationItemsReply::finished, this,
                    [](const Core::ConversationItemList &page) {
                        for (const auto &item : page.items())
                            qInfo().noquote() << item.role() << item.text();
                    });
        });
```

`createConversation`, `getConversation`, `updateConversation`,
`deleteConversation`, `listConversationItems`, `createConversationItems`,
`getConversationItem`, and `deleteConversationItem` are available. The typed
replies are built on a shared internal request engine (retries, rate-limit
headers) so every endpoint gets the same resilience for free.

## Stored chat completions

When a completion is created with `store: true`, the management surface lets you
list, retrieve, update, and delete it — with cursor pagination via `ListParams`:

```cpp
Client::ListParams params;
params.limit = 20;
auto *list = client.listChatCompletions(params);
connect(list, &Client::ChatCompletionListReply::finished, this,
        [](const Core::ChatCompletionList &page) {
            for (const auto &completion : page.data)
                qInfo() << completion.id() << completion.model();
            // page.hasMore / page.lastId drive the next request's `after`
        });
```

`getChatCompletion`, `updateChatCompletion`, `deleteChatCompletion`, and
`listChatCompletionMessages` round out the set. List results use the generic
`Core::ListPage<T>` (aliased as `ChatCompletionList` / `ChatCompletionMessageList`),
the shared page type reused by every list endpoint.

## Embeddings & models

Turn text into vectors and enumerate the available models:

```cpp
auto *embed = client.createEmbeddings(
        Core::EmbeddingRequest("text-embedding-3-small", "hello world"));
connect(embed, &Client::EmbeddingReply::finished, this,
        [](const Core::EmbeddingResponse &response) {
            const QList<double> vector = response.firstVector();
            qInfo() << "dims:" << vector.size();
        });

auto *models = client.listModels();   // also getModel(id)
connect(models, &Client::ModelListReply::finished, this,
        [](const Core::ModelList &list) {
            for (const auto &model : list.data)
                qInfo() << model.id() << model.ownedBy();
        });
```

`Core::ModelList` is `ListPage<Model>`, the same page type the other list
endpoints return.

## Resilience & configuration

The `Client` can retry transient failures, surface rate-limit headroom, and
adapt to different providers:

```cpp
Client::Client client(QUrl("https://api.openai.com/v1"), apiKey);

// Automatic retries with exponential backoff + jitter, honouring Retry-After.
Client::RetryPolicy policy;              // defaults: 2 retries, 429/5xx/network
policy.maxRetries = 3;
client.setRetryPolicy(policy);

client.setRequestTimeoutMs(30000);       // per-request transfer timeout
client.setUserAgent("MyApp/1.0");
client.setDefaultHeader("X-My-Header", "value");

// Rate-limit headroom from the last response's headers:
connect(reply, &Client::ChatCompletionReply::finished, this, [reply] {
    const auto rl = reply->rateLimit();  // remainingRequests / remainingTokens / ...
});
```

**Azure OpenAI** (and other `api-key`-style providers):

```cpp
client.setAuthScheme(Client::Client::AuthScheme::AzureApiKey);  // api-key: <key>
client.setApiVersion("2024-06-01");                             // ?api-version=...
```

## Building

Requirements: CMake ≥ 3.21, a C++17 compiler, and Qt 6 (`Core`, `Network`,
plus `Test` for the test suite).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Options

| Option                    | Default        | Description                          |
|---------------------------|----------------|--------------------------------------|
| `QTOPENAI_BUILD_TESTS`    | `ON` top-level | Build the QtTest unit tests.         |
| `QTOPENAI_BUILD_EXAMPLES` | `ON` top-level | Build the example programs.          |
| `QTOPENAI_BUILD_SHARED`   | `ON`           | Build shared (vs. static) libraries. |

### Using it from another CMake project

```cmake
add_subdirectory(QtOpenAi)
target_link_libraries(myapp PRIVATE QtOpenAi::Client)   # pulls in Core
```

## Testing

The suite is written with **QtTest** and runs entirely offline — the networking
tests spin up a local stub HTTP server, so no API key or internet access is
required. CI builds and tests on Linux, macOS and Windows (see
[`.github/workflows/ci.yml`](.github/workflows/ci.yml)).

## License

[MIT](LICENSE).
