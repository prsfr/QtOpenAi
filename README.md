# QtOpenAi

A modular **Qt 6** client library for the **OpenAI API** and OpenAI-compatible
endpoints, with first-class **tool calling** wired through Qt's meta-object
system (signals/slots and `QMetaObject::invokeMethod`).

It covers the API rather than a corner of it: chat completions and the
Responses API, streaming, structured outputs, embeddings, images, speech and
transcription, video, files, uploads, vector stores, containers, batch,
fine-tuning, evals, the Assistants beta, ChatKit, Skills, and the Realtime
WebSocket channel.

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
| `QtOpenAi::Core`     | `QtOpenAiCore`    | Value types, JSON (de)serialisation, and JSON-Schema.      |
| `QtOpenAi::Client`   | `QtOpenAiClient`  | Async networking `Client`, replies, and the `ToolRegistry`.|
| `QtOpenAi::Chat`     | `QtOpenAiChat`    | Conversation history, branching, trimming, and the agent loop. |
| `QtOpenAi::Tools`    | `QtOpenAiTools`   | Ready-made tools for the `ToolRegistry`, sandboxed and opt-in. |
| `QtOpenAi::Storage`  | `QtOpenAiStorage` | Persisting conversations, cached responses and metrics.    |
| `QtOpenAi::Admin`    | `QtOpenAiAdmin`   | The `/organization` administration surface (admin key).    |
| `QtOpenAi::Realtime` | `QtOpenAiRealtime`| The Realtime WebSocket channel (optional).                 |
| `QtOpenAi::Sql`      | `QtOpenAiSql`     | The SQLite backend for `Storage` (optional).               |

`QtOpenAi::Client` depends on `QtOpenAi::Core`; `Core` has no dependency beyond
`Qt6::Core`. `QtOpenAi::Chat` sits on top of both, because `Agent` drives the
request loop — though `Transcript` and `TrimPolicy` themselves touch no
networking, so history can be built, trimmed and persisted with no `Client` in
sight. `QtOpenAi::Storage` is the one place that touches all three of
`Transcript`, `MetricsSnapshot` and `ResponseCache`, which is why it is its own
module rather than living in `Chat` or `Client` — either choice would have made
that module depend on the other. `QtOpenAi::Admin` is separate for a different
reason: it takes a different *credential*, and the type system is what keeps the
two apart — see [Administration](#administration-qtopenaiadmin).

The two optional modules are each a single Qt dependency that nothing else
needs. `QtOpenAi::Realtime` depends on `Core` and `Qt6::WebSockets`, and is built
only when that component is found (`QTOPENAI_BUILD_REALTIME`). `QtOpenAi::Sql`
depends on `Storage` and `Qt6::Sql`, same arrangement
(`QTOPENAI_BUILD_SQL`) — so an application that persists to JSON files, or does
not persist at all, links no database driver to get there.

### Headless, not UI-hostile

**The library does not depend on a GUI stack. Your application is free to.**

Those are two different statements and both matter. `Qt6::Core`, `Qt6::Network`
and — for the optional modules — `Qt6::WebSockets` and `Qt6::Sql` are the whole
dependency list; no module links `Qt6::Gui`, `Qt6::Widgets`, `Qt6::Quick`,
`Qt6::Qml` or `Qt6::OpenGL`, and none ever will. That is what lets the same
library run inside a daemon, a CLI, a test harness or a container with no
display stack anywhere near it.

It is emphatically **not** a restriction on the caller. A Widgets or Qt Quick
application links `QtOpenAi::Client` exactly like any other one, connects to its
signals from a widget or a `QObject` exposed to QML, and puts its value types in
a `QVariant` for a model or a property binding. Building a UI on this library is
the expected case — the library simply does not contain one, so that nothing
forces the dependency on callers who have no use for it.

Both halves are checked rather than intended, because both are the kind of thing
that erodes one convenient line at a time:

| Check | Where | What would otherwise slip through |
|---|---|---|
| Configure-time link-library guard | `CMakeLists.txt` | a GUI module named in any QtOpenAi target |
| `objdump -p` over the built libraries | CI | a GUI dependency arriving *transitively*, where no `CMakeLists` mentions it |
| `gui_consumer` — a real `QApplication` linking the installed package | CI | the library becoming unusable *from* a GUI application |

The last one is the converse of the first two, and it is there because "does not
depend on a GUI" would be worthless if it quietly came to mean "does not work
with one".

## Design

* **SOTA data structures** — every model type (`Message`, `ToolCall`, `Tool`,
  `ChatCompletionRequest`, `ChatCompletionResponse`, …) is an *implicitly
  shared* (copy-on-write) value type backed by `QSharedDataPointer`. Cheap to
  copy, safe to pass by value, `==`/`!=` comparable.
* **Hidden implementation** — QObject-derived types (`Client`,
  `ChatCompletionReply`, `ToolRegistry`) use the classic `Q_DECLARE_PRIVATE`
  `d`-pointer so the ABI stays stable and headers stay clean.
* **No boilerplate replies** — most endpoints decode into exactly one value
  type, so they derive from `TypedReply<T>`, which owns the parsed value and the
  decode step. A concrete reply is left with only what is genuinely its own: a
  typed `finished(...)` signal, a getter named the way that endpoint names its
  payload, and the one line that fires the signal. The streaming replies, which
  carry real state, still derive from `RestReplyBase` directly.
* **One request path** — the ~165 endpoint methods on `Client` are each a
  single call into one of four private helpers (`get`, `post`, `postMultipart`,
  `remove`). Building the request, merging the query, capturing a retry factory
  and attaching the `RetryPolicy` happen in exactly one place, so a change to
  how requests are made reaches every endpoint at once. Endpoint paths are
  composed from a table of collection constants rather than spelled out at each
  call site.
* **Qt coding style** — getters are `content()` not `getContent()`; setters are
  `setContent()`; enums are exposed via `Q_ENUM`/`Q_NAMESPACE`.

## Tool calling via the Qt meta-object system

`QtOpenAi::Client::ToolRegistry` maps a model's tool calls back onto local C++
code in three interchangeable ways:

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

// 3) An invokable method whose definition is derived from its own signature
registry.registerMethod(weatherProvider, "forecast");

// React to execution via signals
connect(&registry, &Client::ToolRegistry::toolInvoked,
        this, [](const QString &id, const QString &name, const QString &result) {
            qInfo() << name << "->" << result;
        });
```

### Schema without hand-writing

The third form needs no schema at all. `Core::MetaSchema` derives one from the
meta-object, so the names and types the model is told about are the names and
types the method actually has — rename an argument and the advertised schema
follows:

```cpp
class WeatherService : public QObject
{
    Q_OBJECT
    // The one thing the meta-object system does not know: what things mean.
    // The method is named once; its arguments follow as name/description pairs.
    QTOPENAI_DOC_METHOD(forecast, "Get the weather forecast for a city.",
                        location, "City name, e.g. Berlin")
public:
    Q_INVOKABLE QJsonObject forecast(const QString &location, int days);
};

registry.registerMethod(&service, "forecast");
// parameters: {"type":"object",
//              "properties":{"location":{"type":"string","description":"City name, e.g. Berlin"},
//                            "days":{"type":"integer"}},
//              "required":["location","days"],"additionalProperties":false}
```

### The description macros

The `QTOPENAI_DOC*` macros are the only hand-written part. They take
identifiers rather than a path and expand to the `Q_CLASSINFO` the schema reads
— `"doc"`, `"doc:<member>"`, `"doc:<method>:<argument>"` — so there is no
prefix to forget and no colon to misplace.

`QTOPENAI_DOC_METHOD` names the method **once** and takes its arguments after
the description, one `name, "description"` pair each. Put it **directly above
the declaration it describes** — the placement
[Cutelyst's `C_ATTR`](https://github.com/cutelyst/cutelyst/wiki/Tutorial_02_CutelystBasics)
uses, and the better one, because the description then lives where the signature
does:

```cpp
class FileTools : public QObject
{
    Q_OBJECT
    QTOPENAI_DOC("Read and inspect files inside an allowed set of directories.")
public:
    QTOPENAI_DOC_METHOD(write_file, "Write UTF-8 text to a file.",
                        path,    "Path to the file to write.",
                        content, "The text to write.")
    Q_INVOKABLE QString write_file(const QString &path, const QString &content);
};
```

`Q_CLASSINFO` does not care where in the class body it appears, so grouping the
annotations at the top still works and produces exactly the same meta-object — a
test pins that. Adjacent is the convention because renaming an argument and
forgetting its description then becomes a change in one place rather than two
screens apart.

One invocation, three `Q_CLASSINFO`. A method with no arguments is the same
macro with nothing after the description, so there is one macro to know rather
than two. Up to eight arguments; past that, or to describe an argument away from
its method, `QTOPENAI_DOC_ARGUMENT` is still there.

**Why the preprocessor, and not `constexpr` or anything else from C++17?**
Because `Q_CLASSINFO`'s key has to be a string literal *in the source text*: moc
reads the tokens rather than compiling them, so no `constexpr` function and no
`consteval` can take part in building one. Assembling that key is either the
preprocessor's job or the caller's, and the whole point is that it is not the
caller's. The dispatch counts the *variadic* arguments including the
description, so the count is never zero — counting a possibly-empty
`__VA_ARGS__` needs `__VA_OPT__` (C++20) or a compiler extension, and this is
C++17 that must also pass through moc's own preprocessor.

That preprocessor is weaker than a conforming one, which is why the arguments
are flat pairs and not `(name, "description")` tuples: it handles token pasting
and a macro expanding to several `Q_CLASSINFO`, but not the usual trick of
unparenthesising a parameter. All three were checked by running `moc` on a
probe rather than assumed.

### Writing each name once

`QTOPENAI_DOC_METHOD` describes a method declared on the next line, which leaves
the method name and every argument name written **twice** — once in the
description, once in the signature — with nothing checking that the two agree.
`QTOPENAI_DOC_INVOKABLE` is that macro and the `Q_INVOKABLE` declaration
together, which is what the two halves of its name mean. No separate
`Q_INVOKABLE` line, and each name written once:

```cpp
class Weather : public QObject
{
    Q_OBJECT
    QTOPENAI_DOC("Weather lookups.")
public:
    QTOPENAI_DOC_INVOKABLE(QJsonObject, forecast, "Get the weather forecast for a city.",
                           const QString &, location, "City name, e.g. Berlin",
                           int,             days,     "How many days ahead, 1 to 7");
};
```

That expands to the three `Q_CLASSINFO` **and** `Q_INVOKABLE QJsonObject
forecast(const QString &location, int days)`. A renamed argument renames its
description with it, because they are now the same token.

The expansion stops at the closing parenthesis of the signature, so what follows
decides which it is: a `;` leaves a declaration and the body goes out of line, or
a `{ ... }` follows directly and defines it inline. `examples/agent.cpp` does the
latter.

The library's own tool classes — `FileTools`, `HttpTools`, `UtilityTools` — are
written this way, so the form shipped here is the form under test.

**`QTOPENAI_DOC_METHOD` is not thereby obsolete.** It is what to reach for when
the declaration is not something a macro can produce: a `const` invokable, an
overload, a method with a default argument, one that is also a slot, or a
signature you simply want spelled out as plain C++ for the reader — the same
call [Cutelyst](https://github.com/cutelyst/cutelyst/wiki/Tutorial_02_CutelystBasics)
makes with `C_ATTR`. The two produce an identical meta-object, and a test pins
that, so it is never a behavioural choice.

Two limits, each reported as a sentence rather than as a puzzle about an
undeclared identifier:

* A parameter type containing a comma (`QMap<QString, int>`) needs a typedef,
  because the preprocessor splits on it.
* Up to six parameters.

```
error: static assertion failed: QTOPENAI_DOC_INVOKABLE: every parameter needs
three items -- type, name, description. A parameter type containing a comma,
such as QMap<QString, int>, needs a typedef first.
```

One cosmetic caveat: `clang-format` reads the invocation as a function call and
packs the arguments, so the type/name/description columns above survive only
while they fit. The formatting converges — it is not a fight with CI — but the
alignment is a courtesy, not a guarantee.

### What a missing description does, and what the type says instead

Nothing breaks. An annotation that is absent is *absent* — not an empty string —
so the schema comes out with its structure intact and no `description` key:

```json
{"type":"object",
 "properties":{"path":{"type":"string"},
               "maxBytes":{"type":"integer","minimum":0,"maximum":4294967295}},
 "required":["path","maxBytes"],"additionalProperties":false}
```

The model still has the method name, the argument names and their types, which
for `read_file(path, maxBytes)` is most of the story.

**Descriptions are not generated from names**, and that is deliberate rather
than unfinished. Derived from the identifiers, `read_file` yields "Read file."
and `path` yields "Path." — the model already has `"name": "read_file"` and
`"path"`, so such a description restates what it can see, costs tokens on every
request (about 32 across the tools this library ships), and, worst of the three,
*looks* like documentation: a reviewer and `danglingAnnotations()` both see a
described tool and stop looking. A blank is honest about being blank.

What the traits do contribute is **facts the name could never carry**, taken
from the type:

| Type | Contributes |
|---|---|
| `quint8`, `qint16`, `quint32`, … | `minimum` / `maximum` |
| `quint64`, `unsigned long` | `minimum: 0` — the ceiling is past what a double states exactly, so it is left unsaid rather than stated wrongly |
| `QDate`, `QTime`, `QDateTime` | `format: date` / `time` / `date-time` |
| `QUrl`, `QUuid` | `format: uri` / `uuid` |
| `Q_ENUM` | the closed set of its keys |
| `Q_GADGET`, QObject | a nested object schema |

These are not decoration. A model handed a `quint8` has no way to know it may
not answer `300`, and [`SchemaValidator`](#validating-what-the-model-sent)
enforces `minimum`/`maximum` — so stating the bound also means a wrong value is
rejected before it reaches the method.

### When a description names nothing

A name that matches nothing is still legal C++ and still silent — only the
meta-object knows the real names, so only a runtime check can answer it.
`MetaSchema::danglingAnnotations<T>()` lists every annotation describing
something the class does not have, and it costs one line per class in a test:

```cpp
QCOMPARE(MetaSchema::danglingAnnotations<WeatherService>(), QStringList());
```

The tools this library ships are checked that way, so renaming a method without
moving its description fails the suite rather than quietly shipping a tool the
model is told nothing about.

You do not have to remember to write that test, though: `registerMethod()` has
the meta-object and the annotations both in hand at the moment of registration,
so it **warns** there when a description names nothing on the method being
registered. Registration still succeeds — the tool works, it is simply missing a
description its author believed they had written — and the warning says which
key and which class.

The method is called with its parameters filled in from the model's JSON by
name — a `QString` stays a `QString`, an `int` an `int`, a `Q_ENUM` is matched
against its keys — and a `QJsonObject` return value is serialised into the tool
result. A method taking the whole arguments object (`QString(const QJsonObject
&)` or a `QVariantMap`) still receives it verbatim, so form 2 is unchanged.

`MetaSchema::fromType<T>()` does the same for a `Q_GADGET` or QObject, mapping
its `Q_PROPERTY`s to an object schema — which is also what
[typed structured outputs](#typed-structured-outputs) are built on.

### Validating what the model sent

Models can emit arguments that do not fit the schema they were given. Turn on
validation and a bad call never reaches the handler; it comes back as a tool
result naming what was wrong, which is what the model needs to correct itself:

```cpp
registry.setValidateArguments(true);   // off by default

connect(&registry, &Client::ToolRegistry::argumentsRejected,
        this, [](const QString &id, const QString &name, const QStringList &errors) {
            qInfo() << name << "rejected:" << errors;   // "/days: expected integer, got string"
        });
```

The check is `Core::SchemaValidator`, a small implementation of the JSON-Schema
keywords that describe data — `type` (with `integer` as a whole number), `enum`,
`const`, `required`, `properties`, `additionalProperties`, `items`, the numeric
bounds, `minLength`/`maxLength`/`pattern` and `minItems`/`maxItems`. Keywords it
does not implement constrain nothing, so a schema it only partly understands
never rejects valid data. It is public API, usable on its own:

```cpp
const QStringList errors = Core::SchemaValidator::validate(schema, value);
```

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

A complete, runnable version lives in [`examples/tool_loop.cpp`](examples/tool_loop.cpp),
and the derived-schema variant in [`examples/meta_tools.cpp`](examples/meta_tools.cpp)
(`./meta_tools --schema` prints the generated definition without calling the API).

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

## Structured outputs (`response_format`)

`Core::ResponseFormat` constrains decoding to plain text, a JSON object, or a
JSON schema. Build one with the static helpers and attach it to a request:

```cpp
using namespace QtOpenAi::Core;

QJsonObject schema {
    {"type", "object"},
    {"properties", QJsonObject {
        {"name", QJsonObject {{"type", "string"}}},
        {"age",  QJsonObject {{"type", "integer"}}},
    }},
    {"required", QJsonArray {"name", "age"}},
    {"additionalProperties", false},
};

ChatCompletionRequest request("gpt-4o", { Message::user("Extract the person.") });
request.setResponseFormat(ResponseFormat::jsonSchema("person", schema));  // strict by default
```

The same value type feeds the Responses API, where the schema fields are inlined
under `text.format` rather than nested — use `ResponseRequest::setTextFormat()`:

```cpp
ResponseRequest response("gpt-4o", "Extract the person.");
response.setTextFormat(ResponseFormat::jsonSchema("person", schema));
```

`ResponseFormat::text()` and `ResponseFormat::jsonObject()` cover the simpler
modes.

### Typed structured outputs

The schema above describes a shape the C++ code has to reproduce by hand at
both ends — once to ask for it, once to read it back. A `Q_GADGET` can be both:

```cpp
class Person
{
    Q_GADGET
    QTOPENAI_DOC("A person mentioned in the text")
    QTOPENAI_DOC_PROPERTY(age, "Age in whole years")
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int age MEMBER age)
public:
    QString name;
    int age = 0;
};

request.setResponseFormat(ResponseFormat::forType<Person>());
// ... and when the reply arrives:
const Person person = MetaJson::parse<Person>(response.firstMessage().content());
```

`forType<T>()` derives the schema from the properties (via `MetaSchema`), takes
the name from the class and the description from `QTOPENAI_DOC`, and asks
for strict mode — which `MetaSchema` already satisfies, since it marks every
property required and closes the object.

`Core::MetaJson` is the other direction: `parse<T>()` / `fromJson<T>()` populate
the properties from the model's JSON, and `write()` / `toJson()` go back out.
Nested gadgets, `QStringList`s and `Q_ENUM`s (matched against their keys, as the
schema advertised them) all round-trip. A value that does not fit its property
is not written and the read reports `false`, so one malformed field does not
cost the whole object.

Both work on a QObject too — `MetaJson::readInto(object, json)` — with
`objectName` left out, since it is Qt's property and not the model's business.

See [`examples/typed_output.cpp`](examples/typed_output.cpp) (`--schema` prints
the generated `response_format` without calling the API).

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

Two further calls round out the surface. `listResponseInputItems` returns the
items a stored response was built from — the same list shape the Conversations
API returns, so it shares `ConversationItemsReply`:

```cpp
auto *items = client.listResponseInputItems(responseId, params);
```

`compactResponse` compacts a stored response's history and
`countResponseInputTokens` prices a request's input without running it. Both
endpoints are newer than the OpenAPI revision this library was written against,
so only their paths are confirmed: `compactResponse` sends
`{"response_id": ...}` and takes an `extra` object merged in verbatim, and
`InputTokensReply` exposes both the parsed count and the raw payload — so an
unexpected response shape stays reachable rather than being dropped.

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
                        for (const auto &item : page.data)
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

## Conversation history (`QtOpenAi::Chat`)

Every endpoint above takes a request and returns a reply. Keeping the running
conversation — appending turns, building the next request from them, staying
inside the window, letting the user edit a past question — is the application's
job, and `QtOpenAi::Chat` is the part of it that is the same in every
application.

Not to be confused with the server-side [Conversations API](#conversations-api-conversations),
which persists state at the provider. This is local, and works against any
OpenAI-compatible endpoint.

```cpp
using namespace QtOpenAi;

Chat::Transcript transcript;
transcript.setSystemPrompt("You are terse.");
transcript.setTrimPolicy(Chat::TrimPolicy::forModel("gpt-4o-mini"));

transcript.addUserMessage("Why is the sky blue?");
auto *reply = client.createChatCompletion(transcript.buildRequest("gpt-4o-mini"));
connect(reply, &Client::ChatCompletionReply::finished, this,
    [&](const Core::ChatCompletionResponse &response) {
        transcript.addMessage(response.firstMessage());   // and the next request
    });                                                   // is a conversation
```

### It is a tree, and linear use is the tree that never branched

Editing a past message does not overwrite it. `fork()` gives it a sibling and
makes the new one active, so both answers stay reachable — the behaviour behind
every chat UI's `‹ 2/3 ›` control:

```cpp
auto question = transcript.addUserMessage("Why is the sky blue?");
transcript.addMessage(answer);

transcript.fork(question, Core::Message::user("Why are sunsets red?"));
transcript.siblings(question);        // both versions
transcript.setActiveLeaf(previous);   // the first answer is still there
```

`messages()` is the path root → active leaf, with the system prompt in front and
the trim policy applied; that path is the conversation as far as the model is
concerned, and the rest of the tree is history to navigate back to. A transcript
that is only ever appended to has one child per node and reads as a plain list —
which is why there is one type here and not two.

`toJson()`/`fromJson()` round-trip the whole tree, node ids included, so
anything stored alongside an id still points at the right message.

### Trimming

A conversation grows; a context window does not. `Chat::TrimPolicy` takes a
message limit, a token budget or both, and drops from the oldest end — with
three invariants it will not break:

* **The system prompt survives.** Dropping it changes how the model behaves
  rather than merely shortening what it remembers.
* **A tool result never leads.** Dropping the assistant turn that requested the
  tools would leave its results answering nothing, which some providers reject.
* **The newest turn survives.** A single message over budget is sent anyway:
  being told it is too long beats silently sending nothing.

`TrimPolicy::forModel()` takes the budget from `ModelCatalog` — the window less
the room the reply needs — and counts with that model's tokenizer.
`setSummariser()` replaces what was dropped with one message of your making,
which is where a running summary belongs.

See [`examples/conversation.cpp`](examples/conversation.cpp) — interactive with
a key, and an offline walk-through of branching and trimming without one.

### The tool loop, driven for you

`Chat::Agent` owns the chat → `tool_calls` → tool results → chat loop that
[`examples/tool_loop.cpp`](examples/tool_loop.cpp) writes out by hand:

```cpp
Chat::Agent agent(&client, &registry);
agent.setModel("gpt-4o-mini");
agent.setStreaming(true);

connect(&agent, &Chat::Agent::contentDelta, this, &Ui::append);
connect(&agent, &Chat::Agent::finished, this, &Ui::showAnswer);

agent.run("What is the weather in Berlin and Hamburg?");
```

The conversation accumulates in the agent's `Transcript`, so a second `run()`
continues where the first ended, and the trim policy applies throughout.

**The guards are not optional extras.** A loop that talks to a model needs all
three:

```cpp
agent.setMaxIterations(5);   // a model that keeps calling tools instead of answering
agent.setTimeoutMs(60000);   // a run that stops making progress at all
agent.setApprovalCallback([](const Core::ToolCall &call) {
    return confirmWithUser(call);       // refusing is reported to the model as
});                                     // the tool's result, so it can say so
```

`cancel()` abandons a run. Every turn is announced — `assistantMessage`,
`toolInvoked`, `toolRejected`, `finished`, `failed` — so a caller can report the loop
rather than wait for it. See [`examples/agent.cpp`](examples/agent.cpp)
(`--ask` confirms each tool call on the terminal).

## Model capabilities, pricing & token counting

`GET /models` says which models exist. `Core::ModelCatalog` says what they can
do and what they cost — knowledge the API does not return, held locally so it is
available before a request goes out:

```cpp
const Core::ModelInfo info = Core::ModelCatalog::shared().model("gpt-4o-mini-2024-07-18");

info.isKnown();                                  // true — resolved by prefix
info.contextWindow();                            // 128000
info.encoding();                                 // "o200k_base"
info.supports(Core::ModelCapability::Vision);    // true
info.inputPrice();                               // 0.15, USD per 1M tokens
```

Two properties matter more than the table itself:

* **Lookup never fails.** An unknown id first resolves to the longest known id
  that is a prefix of it — which is what turns `gpt-4o-mini-2024-07-18` into the
  entry for `gpt-4o-mini` — and otherwise returns a conservative fallback whose
  `isKnown()` is `false`. Callers do not have to guard the lookup, and one that
  cares can still tell a fact from a guess.
* **The table ages.** Prices change and models appear; the bundled defaults are
  a snapshot. `merge()` takes a JSON table of `{ "<id>": { … } }` and overwrites
  only what it mentions, so a corrected price is a data file rather than a
  release:

```cpp
Core::ModelCatalog::shared().merge(QJsonDocument::fromJson(file.readAll()).object());
```

`Core::TokenCounter` answers the other half — how much of that window a prompt
uses:

```cpp
Core::TokenCounter counter = Core::TokenCounter::forModel("gpt-4o-mini");
counter.count(messages);   // framing overhead included
```

**Vocabulary is not bundled.** The OpenAI encodings are megabytes of table each
and belong to a project with its own release cadence, so this ships the
algorithm and takes the data:

```cpp
Core::TokenCounter::loadEncodingFile("o200k_base", "/path/to/o200k_base.tiktoken");
```

Byte-pair merging then follows tiktoken's algorithm over the UTF-8 bytes of each
piece the encoding's pre-tokenizer produces, so counts match the server's.
Without the file, `count()` still answers using the customary
one-token-per-four-characters estimate, and `isExact()` says which of the two
you got — enough for a rough figure, not enough to fill a context window to the
brim. A counter built before its vocabulary is loaded becomes exact the moment
it arrives.

See [`examples/token_budget.cpp`](examples/token_budget.cpp), which runs
entirely offline.

## Speech-to-text (`/audio/transcriptions`, `/audio/translations`)

Transcribe audio in its source language, or translate it into English. Both
endpoints upload the audio as `multipart/form-data`, so the request carries the
raw file bytes plus a filename; the client builds the multipart body (and
rebuilds it on retries) internally:

```cpp
QByteArray audio = /* read a .wav/.mp3/... into memory */;

Core::TranscriptionRequest request(audio, "clip.wav", "whisper-1");
request.setResponseFormat("verbose_json");   // json / text / srt / verbose_json / vtt
request.setTimestampGranularities({ "segment", "word" });

auto *reply = client.createTranscription(request);
connect(reply, &Client::TranscriptionReply::finished, this,
        [](const Core::TranscriptionResponse &r) {
            qInfo() << r.text();
            for (const auto &segment : r.segments())
                qInfo() << segment.start() << segment.end() << segment.text();
        });
```

`createTranslation(Core::TranslationRequest)` works the same way and returns the
same `TranscriptionReply`. For the plain `text` / `srt` / `vtt` response formats
the transcript is surfaced through `response().text()`. The internal multipart
builder is reused by the other file-upload endpoints (image edits, ...).

## Custom voices (`/audio/voices`, `/audio/voice_consents`)

A cloned voice needs a recorded consent first; both uploads are multipart:

```cpp
Core::CreateVoiceConsentRequest consent("Jane Doe", "en", consentBytes, "consent.wav");
auto *recorded = client.createVoiceConsent(consent);

// ... then, citing the consent's id:
Core::CreateVoiceRequest voice("Narrator", consentId, sampleBytes, "sample.wav");
auto *created = client.createVoice(voice);
```

`listVoiceConsents`, `getVoiceConsent`, `updateVoiceConsent` and
`deleteVoiceConsent` complete the consent CRUD; consents are cursor-paginated
like every other list endpoint, so `PageWalker` iterates them. `voiceStatus()`
and `consentStatus()` stay strings — the spec does not pin their value sets
down, so provider values survive a round-trip instead of collapsing into a
guessed enum. Note the API defines no list endpoint for voices, only creation.

## Images (`/images/generations`, `/edits`, `/variations`)

Generate images from a prompt, or edit/vary an existing one. Generation is a
plain JSON request; edits and variations upload the source image(s) — and, for
edits, an optional mask — as `multipart/form-data`, reusing the same upload
builder as the audio endpoints. All three return an `ImageReply`:

```cpp
Core::ImageGenerationRequest gen("a red cube on a white table", "gpt-image-1");
gen.setSize("1024x1024");
gen.setQuality("high");

auto *reply = client.createImage(gen);
connect(reply, &Client::ImageReply::finished, this,
        [](const Core::ImageResponse &r) {
            const Core::Image image = r.firstImage();
            // image.url() or image.b64Json(), plus image.revisedPrompt()
        });

// Edit with an optional mask (transparent areas mark where to paint):
Core::ImageEditRequest edit(pngBytes, "in.png", "give the cube a hat");
edit.setMask("mask.png", maskBytes);
client.createImageEdit(edit);

// Variations of a source image (dall-e-2):
client.createImageVariation(Core::ImageVariationRequest(pngBytes, "in.png", "dall-e-2"));
```

## Text-to-speech (`/audio/speech`)

Synthesise spoken audio from text. Unlike the JSON endpoints, `/audio/speech`
returns a binary audio blob, so `SpeechReply` surfaces the raw bytes verbatim
(with the response `Content-Type`) rather than a parsed value type:

```cpp
Core::SpeechRequest request("gpt-4o-mini-tts", "Hello from Qt!", "alloy");
request.setResponseFormat("mp3");   // opus / aac / flac / wav / pcm
request.setSpeed(1.0);

auto *speech = client.createSpeech(request);
connect(speech, &Client::SpeechReply::finished, this,
        [](const QByteArray &audio) {
            QFile out("hello.mp3");
            out.open(QIODevice::WriteOnly);
            out.write(audio);   // speech->contentType() == "audio/mpeg"
        });
connect(speech, &Client::SpeechReply::failed, this,
        [](const Client::ClientError &e) { qWarning() << e.message(); });
```

## Video / Sora (`/videos`)

Sora renders video asynchronously: creating a job returns it in the `queued`
state, and you poll until it becomes `completed` (or `failed`) before
downloading the rendered bytes. `createVideo` sends a JSON body, or uploads an
optional reference image as `multipart/form-data` when one is attached:

```cpp
Core::CreateVideoRequest request("a cat surfing a wave at sunset", "sora-2");
request.setSize("720x1280");
request.setSeconds("8");

auto *job = client.createVideo(request);
connect(job, &Client::VideoReply::finished, this,
        [&client](const Core::VideoJob &v) {
            // v.id(), v.status() == Core::VideoStatus::Queued, v.progress()
        });
```

Rather than driving `getVideo` by hand, `pollVideo` returns a `VideoPoller` that
issues `GET /videos/{id}` on a timer and reports every state until the job is
terminal:

```cpp
auto *poller = client.pollVideo(videoId, /*pollIntervalMs=*/2000);
connect(poller, &Client::VideoPoller::progressed, this,
        [](const Core::VideoJob &v) { qInfo() << v.progress() << "%"; });
connect(poller, &Client::VideoPoller::completed, this,
        [&client](const Core::VideoJob &v) {
            if (v.status() != Core::VideoStatus::Completed) {
                qWarning() << v.errorMessage();
                return;
            }
            // Download the rendered bytes (binary, like createSpeech):
            auto *content = client.downloadVideoContent(v.id());
            connect(content, &Client::VideoContentReply::finished, [](const QByteArray &mp4) {
                QFile out("out.mp4");
                out.open(QIODevice::WriteOnly);
                out.write(mp4);   // content->contentType() == "video/mp4"
            });
        });
poller->start();
```

`listVideos`, `remixVideo`, and `deleteVideo` round out the surface. Characters,
edits, and extensions are not yet implemented (limited availability).

## Files (`/files`)

Files are the currency of every endpoint that consumes uploaded data:
fine-tuning, batch, vector stores, and file inputs to a model. Uploads go out as
`multipart/form-data`; everything else is a plain REST call:

```cpp
Core::FileUploadRequest request(bytes, "training.jsonl", "fine-tune");
request.setExpiresAfter("created_at", 3600);   // optional retention window

auto *upload = client.uploadFile(request);
connect(upload, &Client::FileReply::finished, this,
        [](const Core::FileObject &file) {
            // file.id(), file.bytes(), file.purpose(), file.status()
        });
```

`listFiles` pages through the stored files and takes an optional `purpose`
filter on top of the shared `ListParams` cursor parameters:

```cpp
Client::ListParams params;
params.limit = 20;
auto *listing = client.listFiles(params, "assistants");
connect(listing, &Client::FileListReply::finished, this,
        [](const Core::FileList &list) {
            for (const Core::FileObject &file : list.data)
                qInfo() << file.id() << file.filename();
            // list.hasMore / list.lastId drive the next page
        });
```

The contents come back as raw bytes through the shared `BinaryReply` (the same
type `createSpeech` and the video download build on), so binary payloads survive
verbatim:

```cpp
auto *content = client.downloadFileContent(fileId);
connect(content, &Client::BinaryReply::finished, this, [](const QByteArray &bytes) {
    QFile out("downloaded.bin");
    out.open(QIODevice::WriteOnly);
    out.write(bytes);
});
```

`getFile` retrieves a single file's metadata and `deleteFile` removes it; the
deletion acknowledgement arrives as a `FileObject` whose `object()` is
`"file.deleted"`.

## Uploads (`/uploads`)

Files beyond the single-request limit of `POST /files` go up as a sequence of
parts: create the upload, post each chunk, then complete it — the server
assembles the parts into one regular `FileObject`. `uploadInChunks` runs that
whole flow and streams straight from a `QIODevice`, so only one chunk is in
memory at a time:

```cpp
QFile source("training.jsonl");
source.open(QIODevice::ReadOnly);

Core::CreateUploadRequest request("training.jsonl", "fine-tune", source.size(), "text/jsonl");

// The device is not adopted and must outlive the run.
auto *uploader = client.uploadInChunks(request, &source);   // 64 MB parts by default
connect(uploader, &Client::ChunkedUploader::progressed, this,
        [](qint64 sent, qint64 total) { qInfo() << sent << "/" << total; });
connect(uploader, &Client::ChunkedUploader::completed, this,
        [](const Core::Upload &upload) {
            // upload.file()->id() is the assembled file
        });
uploader->start();
```

An in-memory overload takes a `QByteArray` instead and fills in the total size
itself when the request leaves `bytes()` at 0.

The individual steps are available too, for callers that need to drive the flow
themselves (resuming an interrupted upload, computing part ids elsewhere):

```cpp
auto *created  = client.createUpload(request);                 // POST /uploads
auto *part     = client.addUploadPart(uploadId, chunk);        // POST .../parts
auto *finished = client.completeUpload(uploadId, partIds,      // POST .../complete
                                       /*md5=*/QString());
auto *aborted  = client.cancelUpload(uploadId);                // POST .../cancel
```

`Upload::isTerminal()` reports whether an upload can still take parts —
`completed`, `cancelled` and `expired` are final.

## Vector stores (`/vector_stores`)

A vector store is the server-side index behind file search — both for the
Responses `file_search` tool and for Assistants. Files are added by id (upload
them through the Files API first) and are chunked and embedded asynchronously,
so a fresh store starts `in_progress` and becomes searchable as its
`fileCounts()` fill up:

```cpp
Core::CreateVectorStoreRequest request("Support FAQ", {"file-1", "file-2"});
request.setExpiresAfter("last_active_at", 7);          // optional retention

auto *created = client.createVectorStore(request);
connect(created, &Client::VectorStoreReply::finished, this,
        [](const Core::VectorStore &store) {
            // store.status(), store.fileCounts().completed / .total
        });
```

Searching returns ranked chunks together with the file they came from; `text()`
joins a hit's chunks, which is usually what you feed back to a model:

```cpp
Core::VectorStoreSearchRequest search("How do I reset my password?");
search.setMaxNumResults(5);
search.setFilters(QJsonObject {{"type", "eq"}, {"key", "region"}, {"value", "eu"}});

auto *hits = client.searchVectorStore(storeId, search);
connect(hits, &Client::VectorStoreSearchReply::finished, this,
        [](const Core::VectorStoreSearchPage &page) {
            for (const Core::VectorStoreSearchResult &hit : page.data)
                qInfo() << hit.score() << hit.filename() << hit.text();
            // page.hasMore / page.nextPage page through the rest
        });
```

`filters` and `rankingOptions` — and a file's `chunkingStrategy` — stay raw
`QJsonObject`s on purpose: the API keeps extending those grammars, so passing
them through means a new option never needs a library release.

The file sub-resource and batches round out the surface:

```cpp
client.createVectorStoreFile(storeId, fileId);            // attach one file
client.listVectorStoreFiles(storeId, {}, "completed");    // filter by status
client.updateVectorStoreFileAttributes(storeId, fileId, attributes);
client.deleteVectorStoreFile(storeId, fileId);            // detach, file is kept
client.getVectorStoreFileContent(storeId, fileId);        // the parsed chunks

// Bulk ingest, so a large import needs one poll rather than one per file:
client.createVectorStoreFileBatch(storeId, {"file-1", "file-2", "file-3"});
client.getVectorStoreFileBatch(storeId, batchId);
client.cancelVectorStoreFileBatch(storeId, batchId);
client.listVectorStoreFileBatchFiles(storeId, batchId);
```

## Containers (`/containers`)

A container is the sandbox behind the code-interpreter tool: a short-lived
execution environment with its own filesystem under `/mnt/data`. Files get in
either by uploading bytes directly or by referencing a file that is already in
the Files API:

```cpp
Core::CreateContainerRequest request("analysis");
request.setExpiresAfter("last_active_at", 20);   // reclaimed when idle

auto *created = client.createContainer(request);
connect(created, &Client::ContainerReply::finished, this,
        [&client](const Core::Container &container) {
            // Bytes straight into the sandbox (multipart)...
            client.uploadContainerFile(container.id(), "cities.csv", csvBytes);
            // ...or reuse something already uploaded through /files:
            client.attachContainerFile(container.id(), "file-abc");
        });
```

Reading a file back out uses the same shared `BinaryReply` as the other download
endpoints, so binary artefacts (a generated plot, say) survive verbatim:

```cpp
auto *content = client.downloadContainerFileContent(containerId, fileId);
connect(content, &Client::BinaryReply::finished, this, [](const QByteArray &bytes) {
    QFile out("chart.png");
    out.open(QIODevice::WriteOnly);
    out.write(bytes);
});
```

`listContainers`, `getContainer`, `deleteContainer`, `listContainerFiles`,
`getContainerFile` and `deleteContainerFile` complete the surface.

## Batch (`/batches`)

Batches trade latency for cost: a whole JSONL file of requests is processed
asynchronously within a completion window, at a discount. The input file goes
through the Files API with purpose `batch`, and the results come back the same
way:

```cpp
Core::CreateBatchRequest request(inputFileId, "/v1/chat/completions");
request.setMetadata(QJsonObject {{"job", "nightly"}});

auto *created = client.createBatch(request);
connect(created, &Client::BatchReply::finished, this,
        [&client](const Core::Batch &batch) {
            // batch.status() == Core::BatchStatus::Validating
        });
```

`BatchPoller` drives the wait, reporting the request counts as they fill up and
stopping on the first terminal state (`completed`, `failed`, `expired` or
`cancelled` — note that `cancelling` is *not* terminal):

```cpp
auto *poller = client.pollBatch(batchId, 10000);   // every 10 s
connect(poller, &Client::BatchPoller::progressed, this, [](const Core::Batch &b) {
    const auto counts = b.requestCounts();
    qDebug() << counts.completed << "of" << counts.total << "done";
});
connect(poller, &Client::BatchPoller::completed, this, [&client](const Core::Batch &b) {
    client.downloadFileContent(b.outputFileId());  // JSONL, one line per request
    // b.errorFileId() holds the requests that failed, b.errors() the input
    // problems that stopped the batch from being accepted at all.
});
poller->start();
```

`listBatches`, `getBatch` and `cancelBatch` complete the surface. `BatchPoller`
and `VideoPoller` share their timer, lifecycle and auto-delete behaviour through
`JobPoller`, so both behave identically apart from the job type they carry.

## Fine-tuning (`/fine_tuning`)

Training a model on your own data is a long-running job: upload a JSONL
training file with purpose `fine-tune`, start the job, and wait.

```cpp
Core::CreateFineTuningJobRequest request("gpt-4o-mini-2024-07-18", trainingFileId);
request.setSuffix("my-run");
// Optional — leaving the hyperparameters unset lets the service pick them:
Core::FineTuningHyperparameters hyper;
hyper.nEpochs = 3;
request.setMethodType("supervised");
request.setHyperparameters(hyper);

auto *created = client.createFineTuningJob(request);
```

The wire format spells an auto-chosen hyperparameter as the string `"auto"`;
those decode to an unset `std::optional` rather than an invented number, and
unset values are left out of a request body.

Waiting reuses the same `JobPoller` engine as batches and videos — a paused job
is *not* terminal, so a poller keeps waiting across a `pause`/`resume`:

```cpp
auto *poller = client.pollFineTuningJob(jobId, 30000);
connect(poller, &Client::FineTuningJobPoller::completed, this,
        [](const Core::FineTuningJob &job) {
            if (job.status() == Core::FineTuningJobStatus::Succeeded)
                qDebug() << "new model:" << job.fineTunedModel();
        });
poller->start();
```

`listFineTuningEvents` returns the progress log (status messages and periodic
metrics samples), `listFineTuningCheckpoints` the mid-training snapshots — each
a usable model of its own — and the
`{list,create,delete}FineTuningCheckpointPermission(s)` trio controls which
projects may use a checkpoint. `cancelFineTuningJob`, `pauseFineTuningJob` and
`resumeFineTuningJob` complete the lifecycle.

## Evals (`/evals`)

An eval is a reusable definition — the shape of the test items plus the graders
scoring them — and a *run* executes it against a data source. The config and the
grader list are large open unions in the API, so they are carried as raw JSON
rather than half-modelled; everything a client acts on (ids, name, status,
counts) is typed:

```cpp
Core::CreateEvalRequest request(dataSourceConfig, testingCriteria);
request.setName("Capital cities");
auto *created = client.createEval(request);

// ... then, per run:
Core::CreateEvalRunRequest runRequest(dataSource);
auto *started = client.createEvalRun(evalId, runRequest);
```

`EvalRunPoller` waits for the graders. It is the only poller that carries two
ids — the run id as `JobPoller::jobId()`, plus the owning `evalId()`:

```cpp
auto *poller = client.pollEvalRun(evalId, runId, 5000);
connect(poller, &Client::EvalRunPoller::progressed, this, [](const Core::EvalRun &r) {
    const auto counts = r.resultCounts();
    qDebug() << counts.passed << "passed," << counts.failed << "failed";
});
poller->start();
```

`listEvalRunOutputItems` and `getEvalRunOutputItem` return the per-item verdicts.
Two API quirks are handled for you: cancelling a run is a bare `POST` to the run
itself (not a `/cancel` sub-path), and the delete acknowledgements name their id
`eval_id`/`run_id`, which the value types accept as alternative spellings of
`id`.

## Assistants (`/assistants`) — beta

An assistant is a *stored* configuration: a model plus the instructions, tools
and resources it should always run with. It is created once and then run against
threads (below), so a client only has to keep an id.

```cpp
Core::CreateAssistantRequest request("gpt-4o-mini");
request.setName("Weather assistant");
request.setInstructions("Answer weather questions in one sentence.");
request.addTool(Core::Tool::function("get_weather", "Current weather", schema));
request.addFileSearchTool();          // the hosted tools carry no schema
auto *created = client.createAssistant(request);
```

The same body type modifies an assistant — `updateAssistant(id, request)` sends
only the fields that were set, so a rename cannot reset the instructions.

Because `tools`, `tool_resources` and `response_format` are open unions in the
API (a function tool carries a schema, `file_search` carries its own config),
`Assistant` keeps them as raw JSON; `addTool()` builds a function entry from the
typed `Core::Tool` the rest of the library uses.

Assistants are still a beta surface, and the API rejects a call that does not say
which version it speaks. The client sends `OpenAI-Beta: assistants=v2` on every
Assistants and threads request — nothing to configure. A provider that wants a
different value can override it with `setDefaultHeader("OpenAI-Beta", ...)`.

## Threads, messages & runs (`/threads`) — beta

A thread is the conversation, kept server-side: append messages, run an assistant
against it, and the transcript grows without the client resending it.

```cpp
Core::CreateThreadRequest thread;
thread.addUserMessage("What is the weather in Oslo?");
auto *opened = client.createThread(thread);           // or createThreadAndRun(...)

Core::CreateRunRequest run(assistantId);
auto *started = client.createRun(threadId, run);
```

A run is asynchronous, and it stops for **two** different reasons — `RunPoller`
reports them separately, because only one of them means the run is over:

```cpp
auto *poller = client.pollRun(threadId, runId, 1000);
connect(poller, &Client::RunPoller::completed, this, [](const Core::Run &run) {
    qDebug() << "finished:" << Core::runStatusToString(run.status());
});
connect(poller, &Client::RunPoller::requiresAction, this, [&](const Core::Run &run) {
    // The model called a tool this program owns. The calls arrive as ordinary
    // ToolCalls, so the ToolRegistry answers them unchanged.
    QList<Core::ToolOutput> outputs;
    for (const Core::ToolCall &call : run.requiredToolCalls())
        outputs.append({call.id(), registry.invoke(call).content()});
    client.submitToolOutputs(threadId, run.id(), outputs);   // the run continues
});
poller->start();
```

`Run::isTerminal()` covers `completed`/`failed`/`cancelled`/`incomplete`/
`expired`; `requires_action` is deliberately *not* terminal — the run is parked,
not done — so the poller stops for it without pretending the run finished.

Streaming works the same way as elsewhere: `createRunStream()` emits
`messageDelta()` for incremental assistant text and `runChanged()` for each new
run state. A parked run emits `requiresAction()` and then `finished()` — the
request is over either way — and `submitToolOutputsStream()` resumes it as a new
stream, so a streamed tool loop never has to fall back to polling.

`listThreadMessages()` returns the transcript (most-recent-first). The Assistants
content parts nest their text differently from the chat ones, so
`ThreadMessage::text()` pulls the readable part out of whatever the message
carries. `listRunSteps()` and `getRunStep()` are the audit trail: which message
the assistant wrote, which tools it called.

## Realtime (`/realtime`)

Every other endpoint in this library is a request that ends in a reply. The
Realtime API is a **channel**: a WebSocket that stays open while both sides send
events, with audio flowing in both directions as the model speaks. That is why
it lives in its own module — `QtOpenAi::Realtime` is the only part of the library
that needs `Qt6::WebSockets`, and it is built only when that component is
present (`QTOPENAI_BUILD_REALTIME`).

The REST half stays on `Client`, because minting a credential is an ordinary
request. Doing that first is the point: the API key never leaves your process,
and the channel is opened with a short-lived secret — exactly what you would
hand a browser or a mobile client.

```cpp
Core::RealtimeSessionConfig config;
config.setModel("gpt-realtime");
config.setInstructions("You are concise.");
config.setOutputModalities({"audio"});          // or {"text"}; not both
auto *minted = client.createRealtimeClientSecret(config, 600);   // seconds

// ... then, with minted->clientSecret().value():
Realtime::RealtimeConnection connection;
connection.setApiKey(secret.value());           // or the API key, server-side
connection.setModel("gpt-realtime");
connection.open();
connection.sendText("Explain a WebSocket in one sentence.");
```

An API key and an ephemeral secret are presented identically, so moving a
program from server-side to browser-side changes only that one string. Events
sent before the handshake completes are queued and flushed on connect, so
`sendText()` on the line after `open()` is not a race.

Output arrives as signals, with audio already base64-decoded so a player can be
fed straight from it:

```cpp
connect(&connection, &Realtime::RealtimeConnection::textDelta, this, &Ui::append);
connect(&connection, &Realtime::RealtimeConnection::transcriptDelta, this, &Ui::append);
connect(&connection, &Realtime::RealtimeConnection::audioDelta, this, &Player::play);
connect(&connection, &Realtime::RealtimeConnection::responseFinished, this, &Ui::done);
```

The protocol is two discriminated unions — some 45 server events and 11 client
ones. Modelling 56 classes would mean 56 places to update whenever the API grows
one, and a client that silently drops whatever it does not know. So
`Core::RealtimeEvent` types the envelope (`type`, `event_id`), keeps the rest of
every event verbatim in `payload()`, and names the fields that recur across the
union — `delta()`, `itemId()`, `responseId()`, `session()`, `errorMessage()`.
The named signals above are the handful callers almost always want;
`eventReceived()` carries everything, including events this library has never
heard of:

```cpp
connect(&connection, &Realtime::RealtimeConnection::eventReceived, this,
        [](const Core::RealtimeEvent &event) {
            qDebug() << event.type() << event.payload();
        });
connection.sendEvent(Core::RealtimeEvent(u"response.cancel"_s));   // or any other
```

Audio in is the mirror image: `sendAudio(pcm)` appends to the input buffer
(base64 is handled for you) and `commitAudio()` closes a turn — needed only when
you have turned server-side turn detection off, since otherwise the server
commits for you.

An `error` event is a message on the channel, not a transport failure: it
arrives as `errorReceived()` and the connection stays open.
`socketError()` is the other one — a refused handshake or a dropped socket.

`RealtimeSessionConfig` is one type rather than the usual request/response pair,
because the API genuinely uses one shape in four places: two REST bodies, the
`session.update` client event, and the `session.created` server event. Splitting
it would mean converting between two identical types every time a session is
reconfigured mid-call. Its `audio` tree, `tools`, `tool_choice`, `tracing` and
`prompt` are carried verbatim, and `max_output_tokens` is a `QJsonValue` because
the API answers with the string `"inf"` as readily as with a number.

SIP calls bridged into a session are controlled over REST, answering a
`realtime.call.incoming` webhook:

```cpp
client.acceptRealtimeCall(callId, config);          // ... or
client.rejectRealtimeCall(callId, 486);             // 0 → the API's 603 Decline
client.referRealtimeCall(callId, "tel:+14155550123");
client.hangupRealtimeCall(callId);
```

These are the one family with nothing to decode — the API acknowledges and
returns no object — so `RealtimeCallReply::finished()` carries no payload.

> **Not covered:** `POST /realtime/calls`, the WebRTC SDP handshake. It takes an
> SDP offer produced by a peer-connection stack, which Qt does not ship; the SIP
> control endpoints above cover the calls the library can actually complete.

## ChatKit (`/chatkit`) — beta

ChatKit is OpenAI's hosted chat UI; this is the backend half of it. The point of
a session is `clientSecret()`: a short-lived credential, scoped to one workflow
and one end user, that the browser uses **instead of** the API key — which never
leaves your server:

```cpp
Core::CreateChatKitSessionRequest request(workflowId, "user_789");
request.setExpiresAfter(600);            // seconds; the API defaults to 10 min
request.setMaxRequestsPerMinute(30);
auto *session = client.createChatKitSession(request);
// ... hand session->session().clientSecret() to the frontend
client.cancelChatKitSession(sessionId);  // stops that secret working
```

Threads are created by the ChatKit frontend as the user talks, not through this
API, so the REST surface only reads them — there is no create call and no
request type for one:

```cpp
auto *threads = client.listChatKitThreads(params, "user_789");   // filter by user
auto *items = client.listChatKitThreadItems(threadId);
```

A thread's `status` is the one status in the library that arrives as a tagged
object rather than a bare string, so it is split into `status()` and
`statusReason()` and reassembled on the way out.

A thread item is a six-way union — user and assistant messages, widgets, client
tool calls, tasks and task groups. They agree on an envelope and nothing else, so
only the envelope and the message content are typed; everything else survives
verbatim in `raw()` rather than being dropped:

```cpp
for (const Core::ChatKitThreadItem &item : items->list().data) {
    if (item.isUserMessage() || item.isAssistantMessage())
        qDebug() << item.text();
    else
        qDebug() << item.type() << item.raw();   // widget, tool call, task, ...
}
```

ChatKit is a beta surface of its own: the client sends
`OpenAI-Beta: chatkit_beta=v1` on every call, and as with Assistants a provider
wanting another value can override it with `setDefaultHeader("OpenAI-Beta", ...)`.

## Skills (`/skills`)

A skill is a named, reusable bundle of files a model can be pointed at. The
skill object holds no content of its own — it points at *versions*, which are
immutable once published:

```cpp
// A directory bundle: the file names are paths relative to the skill root.
Core::CreateSkillRequest bundle;
bundle.addFile("SKILL.md", markdown);
bundle.addFile("scripts/build.py", script);
auto *created = client.createSkill(bundle);           // becomes version 1

// A zip works just as well, and is the single-file constructor.
auto *fromZip = client.createSkill({"pdf-report.zip", zipBytes});
```

Both forms go up as `multipart/form-data` under the same `files` field, so
switching between them changes nothing but what you put in the request.

Publishing a version does **not** promote it. `latest_version` moves on every
upload; `default_version` — what callers get when they name no version — only
moves when you say so, which makes rolling forward or back a single call that
touches no content:

```cpp
Core::CreateSkillRequest revision;
revision.addFile("SKILL.md", updatedMarkdown);
auto *published = client.createSkillVersion(skillId, revision);

client.setDefaultSkillVersion(skillId, "2");          // or: revision.setMakeDefault(true)
```

`revision.setMakeDefault(true)` does both in one request; it is the multipart
`default` flag, spelled `makeDefault()` here because `default` is a C++ keyword.
`POST /skills` has no such field, so an untouched request never sends one.

Bundles come back as zips, which are bytes rather than JSON and therefore use the
shared `BinaryReply`:

```cpp
auto *zip = client.downloadSkillContent(skillId);                 // default version
auto *pinned = client.downloadSkillVersionContent(skillId, "2");  // one version
```

`version` is a string everywhere — it is a path segment, not an object id, so it
is kept exactly as the API spells it. Deleting a skill or a version answers with
the same value types, reporting `object()` as `skill.deleted` /
`skill.version.deleted`.

## Provider profiles

The endpoints are the same across OpenAI-compatible providers; the way in is
not. `Client::ProviderProfile` bundles the four things that differ — base URL,
auth scheme, Azure's `api-version`, any headers — under the provider's name:

```cpp
client.setProfile(Client::ProviderProfile::groq());
client.setApiKey(key);
```

Built in: `openAi()`, `azure(resource, apiVersion = {})`, `ollama()`,
`lmStudio()`, `vllm()`, `groq()`, `openRouter()`. `builtIn()` returns the
argument-free ones for offering a choice, and `fromName()` looks one up
case-insensitively for a config file.

A profile is a value, so a built-in is a starting point rather than a fixed
menu — and a provider this library has never heard of is the same type with its
fields set:

```cpp
ProviderProfile profile = ProviderProfile::openRouter();
profile.setHeader("HTTP-Referer", "https://example.test");

ProviderProfile house;                                    // nothing privileged
house.setBaseUrl(QUrl("https://llm.internal/v1"));        // about the built-ins
house.setAuthScheme(Client::AuthScheme::AzureApiKey);
```

Two deliberate limits:

* **The API key is not part of a profile.** A profile says which provider; a key
  says who you are. `setProfile()` leaves the key untouched — putting a secret
  in a value type that gets copied, compared and logged is how secrets escape.
* **`requiresApiKey()`** is `false` for the local servers, so a caller knows not to
  prompt for something the user does not have.

`azure()` configures the key header and the `api-version` parameter, which is
what an Azure endpoint speaking the OpenAI-compatible path shape needs. It does
**not** rewrite paths into the older `/openai/deployments/<deployment>/…` form —
that is a different path grammar, not a different profile, and faking it with a
base URL would produce requests that quietly 404.

## Metrics & observability

`Client::MetricsCollector` records what the client is costing and how it is
behaving. Attach one and every request is timed and counted — duration,
outcome, HTTP status, retries, and the rate-limit headroom the provider
reported — without any of the calling code knowing it is there:

```cpp
Client::MetricsCollector metrics;
metrics.attach(&client);

connect(&metrics, &Client::MetricsCollector::requestRecorded, this,
    [](const Client::RequestMetrics &r) {
        qInfo() << r.durationMs << "ms" << (r.ok ? "ok" : "failed")
                << r.rateLimit.remainingRequests << "requests left";
    });

const auto snapshot = metrics.snapshot();
snapshot.averageDurationMs();
snapshot.failuresByStatus.value(429);   // status 0 is "no response at all"
snapshot.cost();                        // USD, from the catalog's prices
```

Tokens and cost need one thing more. A reply is generic; only the *typed*
response knows which model answered and what it spent, so `observe()` wraps the
call:

```cpp
metrics.observe(client.createChatCompletion(request));
```

It compiles for any reply whose response reports `model()` and `usage()`, and
fails to compile for one that does not — which is the right answer for a file
upload. Cost comes from [`ModelCatalog`](#model-capabilities-pricing--token-counting)
at the moment each request is recorded; a model with no price contributes zero,
an honest "unknown" rather than "free". `setCatalog()` takes a corrected table.

**Time to first token** — what a user perceives as latency — is measured for
streams. The collector finds the streaming reply's `contentDelta` signal through
the meta-object rather than naming each streaming type, so an endpoint added
later is measured on the day it is added. Ask for `stream_options:
{include_usage: true}` if you want a streamed response to report tokens too.

Nothing is paid for when nothing is attached: `Client` announces each reply
through an ordinary signal, and an unconnected signal costs a comparison.

See [`examples/metrics.cpp`](examples/metrics.cpp).

## Interceptors

`Client::Interceptor` is a hook around every request the client makes. Subclass
it, override one or both halves, and install it:

```cpp
Client::LoggingInterceptor logger;
client.addInterceptor(&logger);
```

```cpp
std::optional<Client::InterceptedResponse>
beforeRequest(Client::InterceptedRequest &request) override;   // on the way out
void afterResponse(const Client::InterceptedResponse &response) override;
```

Interceptors exist for what has to happen on *every* call and cannot be written
at the call sites: structured logging with the credentials taken out, a header
whose value differs per request, serving a repeat from a cache. A header with a
**constant** value is not one of them — that is `setDefaultHeader()`, and
routing it through an interceptor would only make it harder to find.

Ordering nests the way middleware conventionally does: `beforeRequest()` runs in
installation order, `afterResponse()` in reverse, so the first installed is the
outermost. The exchange carries the request that caused it
(`response.request`), so the two halves correlate without an interceptor
keeping state across requests that overlap.

The client does not take ownership, but it does keep track: a destroyed
interceptor removes itself, so it cannot be called after it is gone. Nothing is
paid for when nothing is installed — the chain is one empty-list check.

Two scope limits, both deliberate:

* `afterResponse()` does not fire for the **streaming** endpoints. A stream's
  body arrives as a sequence of events and never exists as one object.
  `beforeRequest()` does fire for them, so header injection and logging still
  cover streams.
* The **multipart uploads** do not offer their body to the chain. It is rebuilt
  per attempt and can be a whole file; handing it over would mean holding an
  upload in memory for the benefit of a logger.

### Logging, redacted

`LoggingInterceptor` writes one line per request and one per response to the
`qtopenai.http` category:

```
--> POST https://api.openai.com/v1/chat/completions
    Authorization: <redacted>
<-- 200 POST https://api.openai.com/v1/chat/completions (412 ms)
```

The redaction is the point. An API key is a bearer credential: once it is in a
log file it is in every backup, bug report and pasted terminal buffer that file
reaches. So the header values that can carry one are replaced before anything is
written, as are query parameters whose name looks like a secret — and the
*default* is to redact, so forgetting to configure it is the safe outcome rather
than the leak. `setRedactedHeaders()` replaces the list;
`defaultRedactedHeaders()` is what it starts as.

Bodies are off by default for a related reason: they hold the user's prompts.
`setLogBodies(true)` turns them on, truncated to `maxBodyLength()`. The category
itself defaults to off; turn it on without a rebuild with

```sh
QT_LOGGING_RULES="qtopenai.http.debug=true"
```

or connect to `logged()` to route the same lines somewhere of your own.

### Response caching

`CachingInterceptor` answers an identical request from a store instead of the
network — worth having for deterministic calls (`temperature: 0`), for a prompt
an application re-issues as the user moves back and forth, and for tests. Each hit is
a round trip and a bill that did not happen:

```cpp
Client::CachingInterceptor cache;
client.addInterceptor(&cache);                     // installing it is the opt-in
```

The store is pluggable. `MemoryResponseCache` is the default — an LRU with a
size ceiling and a time limit, both because both failure modes are real: without
a size limit a long-running process grows without bound, and without a time
limit a cached answer outlives the question.

```cpp
Client::MemoryResponseCache store(512);
store.setTtlSeconds(60);                           // 0 disables expiry
cache.setCache(&store);                            // not owned; nullptr restores the default
```

**What is cached is an allow-list, and that is the whole safety story.** A POST
is not idempotent in general: replaying `POST /files` from a cache would hand
back the id of a file the caller believes it just created, and replaying `POST
/fine_tuning/jobs` would hide a job that was never started. So only the
endpoints that compute an answer from their input and change nothing are
cacheable by default — `/chat/completions`, `/completions`, `/embeddings`,
`/moderations` — and `setCacheablePaths()` is the caller's to extend if their
provider has others. GET and DELETE are never cached: a listing that cannot
change is not a listing, and a cached DELETE is a lie.

Three more rules that follow from the same reasoning:

* The key hashes the verb, the URL, the body **and the credential**, so a cache
  shared between two accounts cannot serve one account's answer to the other.
  The credential is hashed, never stored.
* Only 2xx responses are stored. An error describes the provider at a moment,
  not the request; caching one turns a blip into a sticky failure.
* Streams are bypassed on their own — there is no single body to store.

`hit()`, `missed()` and `stored()` report what happened, which is enough to
measure a hit rate without the cache keeping counters nobody reads.

See [`examples/interceptors.cpp`](examples/interceptors.cpp).

## Rate limiting

`Client::RateLimiter` throttles a client so the provider does not have to. A 429
costs a round trip, a retry and sometimes a longer penalty than the wait would
have been, so staying under the limit is cheaper than recovering from crossing
it:

```cpp
Client::RateLimiter limiter;
limiter.setMaxConcurrent(4);
limiter.setRequestsPerMinute(60);
limiter.setTokensPerMinute(90000);
client.setRateLimiter(&limiter);
```

The three budgets are independent and any of them may be left at `0`, meaning no
limit:

* **`maxConcurrent`** — requests in flight at once. Worth setting even when the
  provider imposes no limit, because it also bounds how much of your own memory
  and how many sockets a burst can take.
* **`requestsPerMinute`** — a rolling window, not a per-minute bucket. A bucket
  lets a caller spend the whole budget in the last second of one minute and the
  whole of the next in the first second of the next, which is exactly the burst
  the limit exists to prevent.
* **`tokensPerMinute`** — the same window over *estimated* prompt tokens. The
  estimate runs over the serialised body and is deliberately generous: a token
  budget that undercounts is a budget that does not work.

Requests over budget queue and are released in order. Calling code does not
change: a call still returns its reply immediately, the reply simply has not
started yet. `queued()`, `inFlight()` and `queueChanged()` are enough to report
how far along a burst is.

A 429 carrying `Retry-After` pauses the **whole client**, not only the reply
that received it — the provider is saying that *you* are going too fast. A
response reporting an exhausted window (`x-ratelimit-remaining-requests: 0`)
does the same. `pauseFor()` is the same lever by hand, and it never shortens a
pause already in force.

Three things are deliberately outside its scope:

* **Streams are not gated.** A stream is held open for as long as the model is
  talking, and counting one against a concurrency budget would let a single long
  answer starve everything behind it.
* **Cache hits are not gated.** A hit makes no request, so there is no budget to
  spend.
* **Retries are not re-queued.** A retry is the tail of a request that already
  got through; making it queue again would pin the slot it occupies behind
  whatever is now ahead of it.

Destroying a limiter, or replacing it with `setRateLimiter(nullptr)`, releases
whatever is waiting rather than abandoning it — a caller holding a reply that
would never start would wait forever, which is worse than one burst over budget.
Nothing is queued when no limiter is installed.

See [`examples/rate_limiting.cpp`](examples/rate_limiting.cpp).

## Guardrails

`Client::Guardrail` screens text against the Moderations API and applies a
policy to the answer:

```cpp
Client::Guardrail guardrail(&client);
guardrail.setAction("violence", Client::GuardrailAction::Warn);
guardrail.setAction("self-harm", Client::GuardrailAction::Block);

auto *reply = guardrail.createChatCompletion(request);
connect(reply, &Client::GuardedChatReply::blocked, this, &Ui::showRefusal);
connect(reply, &Client::GuardedChatReply::flagged, this, &Ui::showNotice);
connect(reply, &Client::GuardedChatReply::finished, this, &Ui::showAnswer);
```

Both sides are screened, for different reasons: the **input**, so the
application does not spend a request relaying something it would refuse to
show; the **output**, because a model can produce what its prompt did not ask
for. Either check can be turned off with `setScreenInput()` /
`setScreenOutput()`. A fully screened exchange is three round trips, not one —
that is the honest cost, and it is why the checks are separable.

The policy is **per category**, because the categories are not comparable. An
app for adults may reasonably allow what a children's app must block, and
neither is a "level" of the other. Categories are the provider's own strings;
anything the policy does not name gets `defaultAction()`, which is `Block` — a
category nobody thought about is more likely to matter than not, and an
application that wants everything through can say so in one line. Where several
categories match, the strictest decides; letting a warned category downgrade a
blocked one would make the outcome depend on map ordering.

`setThreshold()` makes the policy stricter than the provider: `1.0` (the
default) trusts the provider's own `flagged` and nothing else, while a lower
value also matches on the category score. A verdict carries *every* category the
provider scored, not only the ones that crossed the line, so a caller can explain
a refusal rather than only announcing it.

Two things worth being explicit about:

* **A failed screening fails the exchange.** A guardrail that treats "I could
  not check" as "it is fine" is not a guardrail.
* **This is deliberately not an `Interceptor`.** The interceptor chain is
  synchronous — a request either goes out now or is answered now — and
  screening needs a round trip of its own. Wiring an awaited call into a chain
  that cannot wait would mean either blocking the event loop or letting the
  unscreened request go out first, and both defeat the purpose. So the guardrail
  composes calls instead.

`judge()` applies the same policy to a `ModerationResult` obtained some other
way, so there is one decision procedure rather than two that can drift.

See [`examples/guardrails.cpp`](examples/guardrails.cpp).

## Mapping many prompts

`Client::ChatMap` runs many prompts and collects the answers in order — the
shape of classification over a dataset, fan-out summarisation and offline
evals: N requests that have nothing to do with each other, which should go out
together but not all at once.

```cpp
Client::ChatMap map(&client);
map.setConcurrency(4);

auto *run = map.map(QStringLiteral("gpt-4o-mini"), prompts);
connect(run, &Client::ChatMapReply::progress, this, &Ui::setProgress);
connect(run, &Client::ChatMapReply::allFinished, this, [run]() { use(run->contents()); });
```

Results are **index-aligned with the input from the first moment**, before
anything has answered. Classifying a thousand rows is only useful if row 837's
answer can still be found at 837 after two of its neighbours failed, so
`results()` keeps successes and failures side by side in input order and
`contents()` leaves a hole rather than closing it.

**A failed item does not fail the run.** One row hitting a content filter is not
a reason to throw away the rest; the error is recorded against its index and the
run carries on. `setMaxFailures()` is for the case where it *is* a reason — a
wrong API key fails every item, and burning a thousand requests to discover that
is a waste worth stopping.

`concurrency()` is what this run keeps in flight, and it **composes with rather
than replaces** [`RateLimiter`](#rate-limiting): a limiter governs everything
the client does, a `ChatMap` governs one run, and with both, whichever is
stricter decides. It is a cap and not a batch size — a slow item does not hold
back the ones behind it.

This is the client-side counterpart to the server-side Batch API, and the trade
is latency against cost: a batch job is cheaper and answers in hours, this
answers in seconds at full price.

`abort()` stops issuing and abandons what is in flight; `allFinished()` still
fires, because a caller waiting on it must not be left waiting because it was
the one who gave up.

See [`examples/parallel_map.cpp`](examples/parallel_map.cpp).

## Local vector search

`Core::VectorIndex` is a small in-memory vector index — the local,
dependency-free half of retrieval-augmented generation — and
`Client::SemanticIndex` attaches the embedding step to it:

```cpp
Client::SemanticIndex index(&client);
index.add(paragraphs);                       // one request for the whole batch

auto *hits = index.query("how do I cancel?", 3);
connect(hits, &Client::SemanticQueryReply::finished, this, &Ui::showPassages);
```

Everything `SemanticIndex` does is two steps a caller could have written: embed
the text, then search the vectors. It exists because those two steps have to
agree about the model — vectors from two different embedding models rank
against each other as convincing nonsense — and keeping the model next to the
index is how they cannot drift apart. `VectorIndex::add()` refuses a vector
whose length differs from the rest for the same reason; a model change
mid-corpus is exactly how that happens, and refusing is the only way a caller
finds out.

Ranking is **brute force, deliberately**. An approximate-nearest-neighbour
structure earns its complexity somewhere past a hundred thousand vectors; below
that a scan over a few thousand embeddings is a few milliseconds, and it is
exact, has no index to rebuild and no parameters to tune wrongly. Past that
point the answer is a real vector database — or OpenAI's own server-side vector
stores — not a worse one here.

Three metrics, all reported so that **higher is better**: `Cosine` (the
default — embedding models encode meaning in direction, and magnitude mostly
encodes how long the text was), `DotProduct`, and `Euclidean`, whose score is
the negated distance so ranking code never has to ask which metric produced it.
`search(query, k, minScore)`'s floor is the difference between "the five
closest documents" and "the five closest documents that are actually about
this", which for a retrieval prompt is the difference between context and
noise.

The index is a plain serialisable value, so a corpus survives a restart:

```cpp
saveJson(index.index().toJson());
index.setIndex(Core::VectorIndex::fromJson(loadJson()));  // no re-embedding
```

The raw arithmetic is available on its own in the `Core::Vector` namespace —
`dot()`, `norm()`, `cosineSimilarity()`, `euclideanDistance()`,
`normalized()` — where mismatched lengths return 0 rather than reading past the
shorter vector.

See [`examples/semantic_search.cpp`](examples/semantic_search.cpp).

## Administration (`QtOpenAi::Admin`)

The `/organization` surface — usage and costs, users and invites, projects,
roles, certificates, admin keys, audit logs — uses an **admin** API key, and
that is why it is a separate object rather than more methods on `Client`:

```cpp
Admin::Organization organization(baseUrl, adminKey);
Admin::ProjectListReply *reply = organization.listProjects();
```

An admin key can archive a project or revoke a colleague's access; a standard
key can only spend money answering questions. Putting these endpoints on
`Client::Client` would have meant one object type carrying either kind of
credential, with nothing to stop the admin key from being sent to
`/chat/completions`. Two types, two keys, and the compiler keeps them apart.
`Organization` deliberately does not hand out the client it uses inside, because
that would give the ability back.

**It is not a second networking stack.** Requests go through `Client`'s
documented request path, so the administration surface gets the same retry
policy, the same interceptor chain — including the credential redaction in
`LoggingInterceptor`, which matters more here than anywhere else — and the same
rate limiter, from one implementation rather than a copy:

```cpp
Client::LoggingInterceptor logger;
organization.addInterceptor(&logger);            // covers /organization too
```

### Projects and what lives inside them

```cpp
organization.listProjects();
organization.createProject(QStringLiteral("Staging"));
organization.archiveProject(projectId);              // a POST, not a DELETE

organization.listProjectUsers(projectId);            // and service accounts,
organization.listProjectApiKeys(projectId);          // API keys, rate limits
```

**Archiving is not deleting, and there is no `deleteProject()`.** A project is
what usage and cost records point at, so the API has no way to remove one:
archiving sets `status` and stamps `archivedAt`, and the billing history keeps
explaining itself. It reads like a delete at the call site, which is why the
method is named for what it does.

The nested collections all repeat one shape under a project id, so their paths
are composed from the collection constants rather than spelled out per call
site — and the data-driven test asserts every composed path, because a wrong one
is a 404 rather than a wrong answer.

**Secrets appear exactly once.** `createProjectServiceAccount()` is the only
reply that carries a usable key (`Core::ServiceAccountApiKey::value`); every
later read is a `Core::ProjectApiKey`, which has a `redactedValue()` and no
`value` at all. There is no endpoint that creates a project API key — a listing
that handed out live credentials would make the admin key a master key.

`Core::ProjectApiKey::owner()` keeps the API's tagged union rather than
flattening it to one name: which kind of principal holds a key — a person or a
service account — is the question an audit asks first.

A rate limit update is **partial**, and `Core::ProjectRateLimit` makes that
structural: every limit is a `std::optional`, so only what the caller set goes on
the wire.

```cpp
Core::ProjectRateLimit limits;
limits.setMaxRequestsPerMinute(600);        // the only field sent
organization.modifyProjectRateLimit(projectId, rateLimitId, limits);
```

A plain `int` would have made "leave this alone" and "set this to zero" the same
value — and zero here means the model is unusable in that project.

### Roles and groups

```cpp
organization.listRoles();                                  // the organization's
organization.listRoles(Admin::RoleScope::project(id));     // one project's

organization.createGroup(QStringLiteral("Support Team"));
organization.addGroupUser(groupId, userId);
organization.assignGroupRole(groupId, roleId);             // scope defaults again
```

**Scope is an argument, not a second set of methods.** Every role path exists
twice, once for the organization and once for a project, and the OpenAPI document
does not merely repeat the schemas across the pair — it points both at the same
ones. Writing them out twice would have doubled thirteen methods and every test
covering them, so that the second copy could differ from the first by a comment.
`Core::OrganizationRole` is one class for both; which scope a role belongs to is
its `resourceType()`, a value rather than a type.

**The scope prefix is not the one you would guess.** A project's *groups* live
under `/organization/projects/{id}/groups`, but a project's *roles* live under
`/projects/{id}/roles` — no `/organization` in front. That is what the API serves,
in its path table and its own curl examples alike. `Admin::RoleScope` is the one
place that knows it, and the data-driven test pins down all thirteen composed
paths on both sides of the inconsistency.

**A role can be inherited, and that changes what revoking it does.** A role a
group gave a user is listed for the user with the group named in
`assignmentSources()`; taking it away from the user achieves nothing, because the
user never had it directly:

```cpp
if (role.isInherited())                     // came through a group
    disableTheRevokeButton();
```

`Core::OrganizationRole` is also one class for the catalogue and for an
assignment. Listing the roles a principal holds returns the same fields plus
provenance — when the role was made, by whom, and which group it comes through —
and reading one out of the catalogue simply leaves those empty.

These lists paginate differently from the rest of the library: `Core::CursorPage`
instead of `Core::ListPage`, because the server sends a single opaque `next`
cursor rather than the `first_id`/`last_id` item ids that walk a `ListPage` in
either direction. Decoding one envelope into the other would have meant a
`lastId` that quietly means something else here than everywhere else.

Two smaller frictions are handled rather than passed on. A group's SCIM flag
arrives as `is_scim_managed` from the groups endpoints and as `scim_managed` when
the same group is embedded in a role assignment; `Core::Group` reads both and
writes one, so a synchronised group is never reported as editable in one of the
two places. And `Core::RoleRequest` writes `role_name` where `Core::OrganizationRole`
reads `name` — the setter is named for the wire, so the mismatch is visible at
the single place it exists.

### Certificates

```cpp
organization.uploadCertificate(pem, QStringLiteral("Production"));
organization.listCertificates();                              // uploaded to the org
organization.listProjectCertificates(projectId);              // active in a project
organization.activateCertificates({certificateId});           // a batch, always
```

**Activating is a batch operation, not a verb on a certificate.** `activate` and
`deactivate` are POSTs to a path ending in the verb, carrying `certificate_ids` —
there is no `POST /organization/certificates/{id}/activate`, which is exactly the
reading the names invite. The API takes one to ten ids per call and answers with
the certificates it changed, so the reply is a list even when you passed one id.
All four paths are pinned in a data-driven test for that reason.

One `Core::Certificate` covers the API's three certificate schemas. They differ
only in which value `object` carries and whether the PEM body and the `active`
flag are present — values, not types, the same call the roles surface makes for
its two scopes. Unlike roles, though, the *methods* are not scope-parameterised:
only listing and the two toggles exist at both scopes, so a scope argument on
upload or delete would be a parameter with one legal value.

**`active` is a `std::optional<bool>`, and the third state is the point.**
Activation belongs to a scope; a certificate read by id belongs to none, so the
API sends no `active` at all there. A plain `bool` would report every
certificate fetched that way as switched off:

```cpp
if (!certificate.active())            // the question does not apply here
    ...
else if (*certificate.active())       // genuinely on, at the scope it came from
```

**The PEM body is asked for, never assumed.** Neither listing returns it, and a
single read returns it only when `getCertificate(id, /*includeContent=*/true)`
puts `include[]=content` on the query — a page of certificates should not drag a
page of PEM bodies with it. The validity window, on the other hand, is flattened
out of the API's `certificate_details` wrapper into `validAt()`, `expiresAt()`
and `pemContent()`: that nesting groups nothing meaning anything on its own, and
`toJson()` puts it back where the server expects it.

These lists paginate by `first_id`/`last_id` — `Core::ListPage`, not the
`CursorPage` the roles and groups endpoints needed.

### Usage and costs

What did last week cost, and what spent it? Eleven endpoints answer that — ten
usage reports and `/organization/costs` — and they all take the same query and
answer in the same shape:

```cpp
Admin::UsageQuery query;
query.startTime = QDateTime::currentSecsSinceEpoch() - 7 * 24 * 3600;   // required
query.bucketWidth = QStringLiteral("1d");
query.groupBy = {QStringLiteral("model")};

organization.usage(Admin::Organization::UsageKind::Completions, query);
organization.costs(query);
```

So they are **one method with an enumerator, not ten methods**, and **one query
type, not eleven signatures spelling out the same six parameters**. That second
one is the part worth getting right: a query the server does not understand does
not fail, it comes back as a perfectly valid report of the wrong thing — which is
why the tests assert the query string that goes on the wire rather than the
reply.

A report is a page of time buckets, and a bucket is a time range plus rows:

```cpp
for (const Core::UsageBucket &bucket : usage.data)        // one per day here
    for (const Core::UsageResult &row : bucket.results)   // one per model
        chart.add(bucket.startTime, row.model(), row.totalTokens());
```

An **empty bucket is a bucket**. The server sends the quiet days too, and keeping
them is what lets a caller plot the series without inventing the gaps.

`Core::UsageResult` is likewise one type for all ten reports rather than ten that
differ by an integer. The grouping keys — project, user, key, model — are the
same everywhere and are typed members; the counters differ per endpoint
(`input_tokens` for completions, `characters` for speech, `usage_bytes` for
vector stores) and live in a map behind named accessors:

```cpp
row.inputTokens();                                  // the documented ones, typed
row.metric(QStringLiteral("reasoning_tokens"));     // and one added after this build
```

Ten near-identical classes would have been ten places to keep in step, and any
counter guessed wrong would have decoded as a silent zero instead of reaching the
caller. Costs are the exception that earns its own type: `Core::CostResult`
carries money — a value *and its currency*, never one without the other — which
would not have survived that map.

### Members and invitations

Membership has two halves, and the API keeps them apart on purpose:

```cpp
organization.listUsers();                            // who is in
organization.listInvites();                          // who has been asked
organization.createInvite({email, QStringLiteral("reader")});
organization.modifyUserRole(userId, QStringLiteral("owner"));
organization.deleteUser(userId);
```

**An invitation is the pending half.** It exists until it is accepted, expires,
or is withdrawn, and only then does a `Core::OrganizationUser` appear — there is
no endpoint that creates a member directly. `deleteInvite()` withdraws an offer;
`deleteUser()` removes someone who already accepted. They are not the same
operation and this library does not pretend they are.

Two roles are in play and they are *not* the same field. `Invite::role` and
`OrganizationUser::role` are the organization role — "owner" or "reader" —
while each `Core::InviteProject` carries a project role of its own. A reader in
the organization can still own a project, which is exactly why flattening the two
would have been wrong.

Both are kept as the string the server sent, like `Project::status`: a role this
build has never heard of has to survive a round trip rather than decay to the
first enumerator — and on *this* field the wrong guess reports a reader as an
owner.

### The request path, from another module

That reuse is what `Client::planRequest()`, `adoptReply()` and the
`issueRequest<Reply>()` template that composes them exist for. They are the
extension point for a module adding an endpoint family — not the way to call an
endpoint, which is what the ~165 named methods on `Client` are for:

```cpp
return client.issueRequest<ProjectListReply>(Client::Verb::Get, "/organization/projects", query);
```

The alternative was a second request path in the new module, kept in step with
the first by hand. `ClientPrivate::issue()` now runs through the same two halves,
so there is still exactly one implementation of *how a request is made*.

Coverage so far is the ten usage reports and `/organization/costs`,
`/organization/users` and `/organization/invites` in full,
`/organization/projects` with its members, service accounts, API keys and rate
limits, `/organization/roles` and `/organization/groups` with their members
and role assignments at both scopes, and `/organization/certificates` with its
activation toggles at both scopes; the rest of the surface — model and
hosted-tool permissions, audit logs, spend alerts, data retention — is tracked in
[#28](https://github.com/prsfr/QtOpenAi/issues/28) and its sub-issues. See
[`examples/organization.cpp`](examples/organization.cpp),
[`examples/organization_usage.cpp`](examples/organization_usage.cpp),
[`examples/organization_members.cpp`](examples/organization_members.cpp),
[`examples/organization_projects.cpp`](examples/organization_projects.cpp),
[`examples/organization_roles.cpp`](examples/organization_roles.cpp) and
[`examples/organization_certificates.cpp`](examples/organization_certificates.cpp).

## Persistence (`QtOpenAi::Storage`)

A desktop application closed at the end of the day should open tomorrow with
yesterday's conversations in it. `Store` is where a conversation tree, the
response cache and a metrics snapshot go to survive that:

```cpp
Storage::JsonFileStore store(directory);
if (!store.open())
    qWarning() << store.lastError();

store.saveConversation(QStringLiteral("chat-1"), transcript, tr("Sky colours"));
// ... next launch:
if (const auto saved = store.loadConversation(QStringLiteral("chat-1")))
    transcript = *saved;
```

**A conversation is stored as the tree it is**, not as the messages currently on
screen. The branch a user made by editing an earlier question is still there
after a restart, and `siblings()` still finds it — a store that kept only the
active path would look correct in every test but the one that matters.

Listing is separate from loading, because listing is what an application does
on every start for every conversation it has:

```cpp
for (const Storage::ConversationRecord &record : store.conversations())
    ui->addRow(record.title, record.updatedAt, record.messageCount);   // no trees read
```

### Two backends, one interface

| Backend | Module | When it is the right one |
|---|---|---|
| `Storage::JsonFileStore` | always built | The data should stay legible: readable in an editor, diffable, `cp -r`-able, recoverable one conversation at a time. |
| `Sql::SqliteStore` | optional (`Qt6::Sql`) | There are thousands of conversations: listing is an index scan, pruning the cache is one statement, and the history is one file to back up. |

Swapping them is the one line that constructs the store; everything else is
written against `Store`. Both keep a **versioned schema**: a store written by a
newer build is refused rather than read on a guess — that is how the newer
version's data gets lost — and the migration hook for older ones is in place
with nothing yet below version 1 to run.

The SQLite backend is SQLite specifically, not "a database". The schema uses
`INSERT OR REPLACE` and a `LIMIT` inside a `DELETE ... NOT IN` subquery, the
file needs no server, and it is the driver Qt ships built in. Taking a
`QSqlDatabase` from the caller would have made the class nominally portable and
actually tested against exactly one engine.

### A cache that outlives the process

`PersistentResponseCache` is a `ResponseCache` whose bodies live in a store, so
`CachingInterceptor` keeps working across a restart:

```cpp
Storage::PersistentResponseCache cache(&store);
Client::CachingInterceptor caching;
caching.setCache(&cache);                          // not owned
client.addInterceptor(&caching);
```

Everything `CachingInterceptor` decides stays where it was — what may be cached
at all, what the key hashes, that errors are never stored. This adds only what a
store cannot decide for itself: a time limit (300 s by default, because an
answer from the last session is by definition from a while ago) and a count
ceiling (1024, higher than the in-memory 128 because the cost here is disk, not
resident memory). Both are applied on insert, in one call into the store.

### Metrics that are the user's, not the process's

```cpp
if (const auto saved = store.loadMetrics(QStringLiteral("all-time")))
    metrics.restore(*saved);                       // carry on from there
...
store.saveMetrics(QStringLiteral("all-time"), metrics.snapshot());
```

`restore()` *replaces* what has been counted rather than adding to it: in the
"restore at startup, save at exit" shape the snapshot already contains those
requests, and adding would double every one of them. Snapshots are keyed, so
"this month" and "all time" can sit side by side.

### Autosave

Saving on every change is a file write per streamed fragment; saving on exit is
the save that is missing after the crash. `Autosave` sits between:

```cpp
Storage::Autosave autosave(&store);
autosave.setIntervalMs(2000);
autosave.setConversation(id, [&] { return transcript; });
autosave.setMetrics(QStringLiteral("all-time"), &metrics);   // hooks the collector itself
connect(&agent, &Chat::Agent::finished, &autosave, &Storage::Autosave::touch);
```

`touch()` says something changed; the store is written at most once per
interval. The conversation is fetched through a callback at save time rather
than copied in, so an application that keeps editing its transcript does not
hand over a new copy on every change — which is the per-change work the class
exists to avoid.

**The destructor does not save.** Tempting, but the source is a callback into
the application, and calling it while that application is being torn down reads
objects that may already be gone. Call `flush()` at shutdown, where the caller
still knows what is alive. A failed save leaves the state dirty and emits
`failed()` — a silent autosave failure is data loss nobody hears about.

See [`examples/persistence.cpp`](examples/persistence.cpp); run it twice.

## Ready-made tools

`QtOpenAi::Tools` is a set of tools a model can be given — the filesystem, one
HTTP GET, a clock and a calculator — each behind a policy that has to be filled
in on purpose.

```cpp
Tools::ToolPolicy policy;                    // everything off
policy.utilities = true;
policy.fileRead  = true;
policy.sandbox   = Tools::FileSandbox({docsPath});

Tools::DefaultTools tools;
tools.setApprovalHandler([this](const QString &name, const QJsonObject &args) {
    return askTheUser(name, args);
});
const QStringList installed = tools.install(&registry, policy);
```

**It is a separate module, and that is not tidiness.** Linking
`QtOpenAi::Client` must never be what gives a model access to a filesystem.
Reaching these tools takes a line in a `CMakeLists.txt`, then a policy object,
then an explicit `install()` — and a reviewer can find every application that
took those steps by grepping for the module name.

Everything is off by default, at every level: an empty `ToolPolicy` installs
nothing, a `FileSandbox` with no roots allows nothing, and an `HttpTools` with
no allowed hosts fetches nothing. Forgetting to configure this cannot be the
thing that grants a power.

### The filesystem sandbox

A model that can name a file can name any file, and it is steered by whatever
text is in its context — including text an attacker wrote into a document it was
asked to summarise. So `FileSandbox` never asks whether a path looks suspicious;
it asks whether the path, **after every symlink has been followed**, is inside a
directory the application named:

* `docs/../../etc/passwd` and a symlink from inside the jail to `/etc/shadow`
  fail the same check for the same reason — the check is on the resolved path,
  not on the spelling.
* `/srv/docs-secret` is not inside `/srv/docs`, even though one string starts
  with the other. Containment is by path component.
* Writing to a file that does not exist yet resolves the **parent**, which is
  what catches creating a file through a symlinked directory.
* Read-only is separate from access, and on by default: reading a corpus and
  rewriting it are different powers, and only one of them cannot be undone.
  `write_file` needs `fileWrite` in the policy **and** a non-read-only sandbox.
* A size cap, because a tool result is pasted straight into a context window.

A refusal is a *result*, not an exception: the model has to be able to read it
and try something else, and a thrown error would end the turn instead of
correcting it. `FileTools::refused()` reports every attempt, which is how an
application notices it is being probed.

### HTTP

Letting a model fetch URLs is the most dangerous ordinary tool there is, and not
because of what it downloads: a model asked to summarise a page will happily
fetch `http://169.254.169.254/` or `http://localhost:8080/admin`, from inside
the network the application is running in. The allow-list is the whole defence,
so it is required rather than recommended, matched **exactly** — `example.com`
does not allow `evil.example.com` — and https is required unless waived.
Redirects are not followed, because a redirect names a host the allow-list never
approved. The body cap is enforced as the response arrives rather than after.

### Utilities

`current_time`, `calculate` and `uuid`: worth shipping precisely because they
are dull. A model that cannot read a clock will confidently state the wrong
date, and one that does arithmetic by predicting the next token gets it wrong in
ways that look right.

`calculate` uses a small arithmetic parser written for the purpose. Handing
model-supplied text to a scripting engine would be `eval` on a string the user
never saw; this grammar has no names to resolve and nothing to call, so there is
nothing to escape into.

### Approval

`setApprovalHandler()` is asked before every side-effecting call — writing a
file, fetching a URL — and returning false refuses it with a sentence the model
can work around. Reads are not gated by default, since a prompt per read makes a
UI unusable and the sandbox already bounds what is readable;
`setApproveReads(true)` gates those too.

Schemas come from the methods themselves via
[`MetaSchema`](#tool-schemas-from-the-meta-object-system), so renaming an
argument cannot leave a stale schema behind for the model to call with.

See [`examples/sandboxed_tools.cpp`](examples/sandboxed_tools.cpp).

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

// Every POST carries a generated Idempotency-Key so a retried create call
// cannot be charged twice. On by default; every attempt shares one key.
client.setIdempotencyKeysEnabled(false); // opt out if a provider dislikes it

// Bring your own QNetworkAccessManager (proxies, custom SSL, a test double).
// The client does not take ownership -- it stays yours to delete. Left unset,
// the client creates one on first use, parented to itself.
client.setNetworkAccessManager(myManager);

// Rate-limit headroom from the last response's headers:
connect(reply, &Client::ChatCompletionReply::finished, this, [reply] {
    const auto rl = reply->rateLimit();  // remainingRequests / remainingTokens / ...
});
```

### Iterating a paginated endpoint

List endpoints return one page at a time (`has_more` plus a `last_id` cursor).
`PageWalker` turns any of them into an iterate-all — it feeds each page's last
id back as the next `after` and stops when the server clears `has_more`:

```cpp
auto *walker = new Client::PageWalker<Client::FileListReply, Core::FileList>(
        [&client](const Client::ListParams &p) { return client.listFiles(p); });

walker->setPageHandler([](const Core::FileList &page) {
    for (const Core::FileObject &file : page.data)
        qDebug() << file.id();
});
connect(walker, &Client::PageWalkerBase::finished, this, [] { /* all pages seen */ });
connect(walker, &Client::PageWalkerBase::failed, this, [](const Client::ClientError &e) { ... });
walker->start();   // deletes itself when it stops, unless setAutoDelete(false)
```

Only the two template arguments and the fetch lambda change per endpoint; a
handler that has seen enough can call `stop()` mid-walk. Every list endpoint
returns a `ListPage`, so every one of them can be walked this way.

**Azure OpenAI** (and other `api-key`-style providers):

```cpp
client.setAuthScheme(Client::Client::AuthScheme::AzureApiKey);  // api-key: <key>
client.setApiVersion("2024-06-01");                             // ?api-version=...
```

## Examples

Runnable programs live in [`examples/`](examples) and are built with
`QTOPENAI_BUILD_EXAMPLES` (on by default for a top-level build). Each reads
`OPENAI_API_KEY`, optional `OPENAI_BASE_URL`, and optional `OPENAI_MODEL` from
the environment, so they work against any OpenAI-compatible endpoint — including
local model servers (Ollama, vLLM, LM Studio, ...):

```sh
export OPENAI_API_KEY=sk-...        # any non-empty value for a keyless local server
export OPENAI_BASE_URL=http://localhost:11434/v1
export OPENAI_MODEL=llama3.1        # overrides each example's default model
./build/bin/streaming "Write a haiku about Qt."
```

| Program             | Endpoint / feature                                   |
|---------------------|------------------------------------------------------|
| `pagination`        | Iterate every page of a list endpoint (`PageWalker`) |
| `token_budget`      | Model capabilities, pricing and token counting, offline |
| `conversation`      | Conversation history, branching and trimming (`Chat`) |
| `agent`             | The tool loop driven by `Chat::Agent`, with guards    |
| `metrics`           | Token usage, cost, latency and time-to-first-token   |
| `interceptors`      | Redacting log, per-request trace header, response cache |
| `rate_limiting`     | Concurrency cap, RPM/TPM budgets and a request queue |
| `parallel_map`      | Map many prompts to answers, N at a time, in order   |
| `chat_tool_loop`    | Chat completion with tool calling via `ToolRegistry` |
| `meta_tools`        | Tool calling with a schema derived from a Q_INVOKABLE |
| `sandboxed_tools`   | Filesystem tools inside a jail, with an approval prompt |
| `streaming`         | Streamed chat completion (SSE), token by token       |
| `responses`         | Responses API (`/responses`)                         |
| `structured_output` | Structured Outputs (`response_format` json_schema)   |
| `typed_output`      | Structured Outputs bound to a Q_GADGET, both ways    |
| `vision`            | Multimodal input (text + image content parts)        |
| `embeddings`        | Embeddings (`/embeddings`)                            |
| `semantic_search`   | Index a corpus locally and answer from what it finds |
| `moderations`       | Moderation (`/moderations`)                           |
| `guardrails`        | Screen input and output against a per-category policy |
| `tts`               | Text-to-speech (`/audio/speech`)                      |
| `voice_cloning`     | Custom voice: consent → voice (`/audio/voices`)      |
| `transcribe`        | Speech-to-text (`/audio/transcriptions`)             |
| `image`             | Image generation (`/images/generations`)             |
| `video`             | Video / Sora: create → poll → download (`/videos`)   |
| `files`             | Files: upload → list → download → delete (`/files`)  |
| `chunked_upload`    | Large-file multipart upload (`/uploads`)             |
| `vector_search`     | Index a document and search it (`/vector_stores`)    |
| `containers`        | Code-interpreter sandbox + files (`/containers`)     |
| `batch`             | Batch: upload → create → poll → results (`/batches`) |
| `fine_tuning`       | Fine-tuning: train → poll → events (`/fine_tuning`)  |
| `evals`             | Evals: define → run → poll → items (`/evals`)        |
| `assistant`         | Assistant + thread + tool-calling run (`/threads`)   |
| `skills`            | Skill: publish → version → promote → zip (`/skills`) |
| `chatkit`           | ChatKit session secret + thread transcript (`/chatkit`)|
| `realtime`          | Live Realtime session over a WebSocket (`/realtime`)  |
| `persistence`       | Conversation, cache and metrics kept across runs     |
| `organization`      | List the organization's projects (`/organization`)   |

`realtime` is the one example that needs the optional `QtOpenAi::Realtime`
module, so it is built only when `Qt6::WebSockets` is available. `persistence`
uses the optional `QtOpenAi::Sql` module when it is there and falls back to the
JSON-files store when it is not. The rest need nothing beyond
`QtOpenAi::Client`.

## Building

Requirements: CMake ≥ 3.21, a C++17 compiler, and Qt 6 (`Core`, `Network`,
plus `Test` for the test suite, `WebSockets` for the optional
`QtOpenAi::Realtime` module and `Sql` for the optional `QtOpenAi::Sql` module).

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
| `QTOPENAI_BUILD_REALTIME` | Qt WebSockets  | Build `QtOpenAi::Realtime`; on when `Qt6::WebSockets` is found. |
| `QTOPENAI_BUILD_SQL`      | Qt Sql         | Build `QtOpenAi::Sql`; on when `Qt6::Sql` is found. |

### Using it from another CMake project

```cmake
add_subdirectory(QtOpenAi)
target_link_libraries(myapp PRIVATE QtOpenAi::Client)     # pulls in Core
target_link_libraries(myapp PRIVATE QtOpenAi::Storage)    # pulls in Chat + Client
target_link_libraries(myapp PRIVATE QtOpenAi::Admin)      # the /organization surface
target_link_libraries(myapp PRIVATE QtOpenAi::Realtime)   # optional; adds WebSockets
target_link_libraries(myapp PRIVATE QtOpenAi::Sql)        # optional; adds Sql
```

## Testing

The suite is written with **QtTest** and runs entirely offline — the networking
tests spin up a local stub HTTP server, and the Realtime ones a local stub
WebSocket server, so no API key, internet access or audio device is required.

CI ([`.github/workflows/ci.yml`](.github/workflows/ci.yml)) builds and tests on
**Linux only until 1.0**. The library has no platform-specific code, so the
macOS and Windows jobs were re-proving the same result at roughly twice the
wall-clock of the Linux one. Both are still one switch away, without editing the
workflow:

| To build and test all three platforms | |
|---|---|
| for every run | set the repository variable `CI_ALL_PLATFORMS` to `true` |
| for a single run | *Actions → CI → Run workflow*, tick **all_platforms** |

Restoring all platforms unconditionally is tracked in
[#92](https://github.com/prsfr/QtOpenAi/issues/92) for the 1.0 release.

## License

[MIT](LICENSE).
