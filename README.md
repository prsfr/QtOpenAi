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
* **No boilerplate replies** — most endpoints decode into exactly one value
  type, so they derive from `TypedReply<T>`, which owns the parsed value and the
  decode step. A concrete reply is left with only what is genuinely its own: a
  typed `finished(...)` signal, a getter named the way that endpoint names its
  payload, and the one line that fires the signal. The streaming replies, which
  carry real state, still derive from `RestReplyBase` directly.
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
| `chat_tool_loop`    | Chat completion with tool calling via `ToolRegistry` |
| `streaming`         | Streamed chat completion (SSE), token by token       |
| `responses`         | Responses API (`/responses`)                         |
| `structured_output` | Structured Outputs (`response_format` json_schema)   |
| `vision`            | Multimodal input (text + image content parts)        |
| `embeddings`        | Embeddings (`/embeddings`)                            |
| `moderations`       | Moderation (`/moderations`)                           |
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
