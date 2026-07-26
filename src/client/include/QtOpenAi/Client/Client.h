// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/BatchListReply.h>
#include <QtOpenAi/Client/BatchPoller.h>
#include <QtOpenAi/Client/BatchReply.h>
#include <QtOpenAi/Client/BinaryReply.h>
#include <QtOpenAi/Client/ChatCompletionListReply.h>
#include <QtOpenAi/Client/ChatCompletionMessageListReply.h>
#include <QtOpenAi/Client/ChatCompletionReply.h>
#include <QtOpenAi/Client/ChatCompletionStreamReply.h>
#include <QtOpenAi/Client/ChunkedUploader.h>
#include <QtOpenAi/Client/CompletionReply.h>
#include <QtOpenAi/Client/CompletionStreamReply.h>
#include <QtOpenAi/Client/ContainerFileListReply.h>
#include <QtOpenAi/Client/ContainerFileReply.h>
#include <QtOpenAi/Client/ContainerListReply.h>
#include <QtOpenAi/Client/ContainerReply.h>
#include <QtOpenAi/Client/ConversationItemsReply.h>
#include <QtOpenAi/Client/ConversationReply.h>
#include <QtOpenAi/Client/EmbeddingReply.h>
#include <QtOpenAi/Client/EvalListReply.h>
#include <QtOpenAi/Client/EvalReply.h>
#include <QtOpenAi/Client/EvalRunListReply.h>
#include <QtOpenAi/Client/EvalRunOutputItemListReply.h>
#include <QtOpenAi/Client/EvalRunOutputItemReply.h>
#include <QtOpenAi/Client/EvalRunPoller.h>
#include <QtOpenAi/Client/EvalRunReply.h>
#include <QtOpenAi/Client/FileListReply.h>
#include <QtOpenAi/Client/FileReply.h>
#include <QtOpenAi/Client/FineTuningCheckpointListReply.h>
#include <QtOpenAi/Client/FineTuningEventListReply.h>
#include <QtOpenAi/Client/FineTuningJobListReply.h>
#include <QtOpenAi/Client/FineTuningJobPoller.h>
#include <QtOpenAi/Client/FineTuningJobReply.h>
#include <QtOpenAi/Client/FineTuningPermissionListReply.h>
#include <QtOpenAi/Client/FineTuningPermissionReply.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/ImageReply.h>
#include <QtOpenAi/Client/InputTokensReply.h>
#include <QtOpenAi/Client/ListParams.h>
#include <QtOpenAi/Client/ModelListReply.h>
#include <QtOpenAi/Client/ModelReply.h>
#include <QtOpenAi/Client/ModerationReply.h>
#include <QtOpenAi/Client/ResponseReply.h>
#include <QtOpenAi/Client/ResponseStreamReply.h>
#include <QtOpenAi/Client/RetryPolicy.h>
#include <QtOpenAi/Client/SpeechReply.h>
#include <QtOpenAi/Client/TranscriptionReply.h>
#include <QtOpenAi/Client/UploadPartReply.h>
#include <QtOpenAi/Client/UploadReply.h>
#include <QtOpenAi/Client/VectorStoreFileBatchReply.h>
#include <QtOpenAi/Client/VectorStoreFileContentReply.h>
#include <QtOpenAi/Client/VectorStoreFileListReply.h>
#include <QtOpenAi/Client/VectorStoreFileReply.h>
#include <QtOpenAi/Client/VectorStoreListReply.h>
#include <QtOpenAi/Client/VectorStoreReply.h>
#include <QtOpenAi/Client/VectorStoreSearchReply.h>
#include <QtOpenAi/Client/VideoContentReply.h>
#include <QtOpenAi/Client/VideoListReply.h>
#include <QtOpenAi/Client/VideoPoller.h>
#include <QtOpenAi/Client/VideoReply.h>
#include <QtOpenAi/Client/VoiceConsentListReply.h>
#include <QtOpenAi/Client/VoiceConsentReply.h>
#include <QtOpenAi/Client/VoiceReply.h>
#include <QtOpenAi/Core/ChatCompletionRequest.h>
#include <QtOpenAi/Core/CompletionRequest.h>
#include <QtOpenAi/Core/CreateBatchRequest.h>
#include <QtOpenAi/Core/CreateContainerRequest.h>
#include <QtOpenAi/Core/CreateEvalRequest.h>
#include <QtOpenAi/Core/CreateFineTuningJobRequest.h>
#include <QtOpenAi/Core/CreateUploadRequest.h>
#include <QtOpenAi/Core/CreateVectorStoreRequest.h>
#include <QtOpenAi/Core/CreateVideoRequest.h>
#include <QtOpenAi/Core/CreateVoiceRequest.h>
#include <QtOpenAi/Core/EmbeddingRequest.h>
#include <QtOpenAi/Core/FileUploadRequest.h>
#include <QtOpenAi/Core/ImageEditRequest.h>
#include <QtOpenAi/Core/ImageGenerationRequest.h>
#include <QtOpenAi/Core/ImageVariationRequest.h>
#include <QtOpenAi/Core/ModerationRequest.h>
#include <QtOpenAi/Core/ResponseOutputItem.h>
#include <QtOpenAi/Core/ResponseRequest.h>
#include <QtOpenAi/Core/SpeechRequest.h>
#include <QtOpenAi/Core/TranscriptionRequest.h>
#include <QtOpenAi/Core/TranslationRequest.h>
#include <QtOpenAi/Core/VectorStoreSearch.h>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

class QIODevice;
class QNetworkAccessManager;

namespace QtOpenAi {
namespace Client {

class ClientPrivate;

// The entry point for talking to an OpenAI-compatible chat API.
//
// Configure a base URL and API key, then call createChatCompletion() to obtain
// an asynchronous ChatCompletionReply. The client works with any endpoint that
// speaks the OpenAI /chat/completions protocol (OpenAI, Azure OpenAI, Ollama,
// vLLM, LM Studio, ...).
class QTOPENAI_CLIENT_EXPORT Client : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(
            QString organization READ organization WRITE setOrganization NOTIFY organizationChanged)
public:
    explicit Client(QObject *parent = nullptr);
    // Construct with a base URL and key in one step.
    Client(QUrl baseUrl, QString apiKey, QObject *parent = nullptr);
    ~Client() override;

    // The API root, e.g. https://api.openai.com/v1. Endpoint paths such as
    // "/chat/completions" are appended to it. Defaults to the OpenAI root.
    QUrl baseUrl() const;
    void setBaseUrl(const QUrl &baseUrl);

    QString apiKey() const;
    void setApiKey(const QString &apiKey);

    // Optional OpenAI organization / project header value.
    QString organization() const;
    void setOrganization(const QString &organization);

    // How the API key is presented. BearerToken (default) sends
    // `Authorization: Bearer <key>`; AzureApiKey sends `api-key: <key>`.
    enum class AuthScheme {
        BearerToken,
        AzureApiKey
    };
    Q_ENUM(AuthScheme)
    AuthScheme authScheme() const;
    void setAuthScheme(AuthScheme scheme);

    // Azure OpenAI `api-version` query parameter (appended to every request when
    // non-empty). Ignored by standard OpenAI endpoints.
    QString apiVersion() const;
    void setApiVersion(const QString &apiVersion);

    // Automatic-retry policy for transient failures (429/5xx/network).
    RetryPolicy retryPolicy() const;
    void setRetryPolicy(const RetryPolicy &policy);

    // Send a generated `Idempotency-Key` header with every POST, so the
    // automatic retries of a create call cannot be charged twice. On by
    // default, because retrying POSTs is on by default; every attempt of one
    // logical call shares the same key. Providers that do not know the header
    // ignore it.
    bool idempotencyKeysEnabled() const;
    void setIdempotencyKeysEnabled(bool enabled);

    // Per-request transfer timeout in milliseconds (0 disables; default 0).
    int requestTimeoutMs() const;
    void setRequestTimeoutMs(int timeoutMs);

    // Custom User-Agent (empty leaves Qt's default).
    QString userAgent() const;
    void setUserAgent(const QString &userAgent);

    // Extra headers sent with every request (e.g. provider-specific).
    void setDefaultHeader(const QByteArray &name, const QByteArray &value);
    void removeDefaultHeader(const QByteArray &name);
    QHash<QByteArray, QByteArray> defaultHeaders() const;

    // Inject a custom QNetworkAccessManager (e.g. for proxies or test doubles).
    // The client does not take ownership.
    void setNetworkAccessManager(QNetworkAccessManager *manager);
    QNetworkAccessManager *networkAccessManager() const;

    // Start a chat completion. Ownership of the returned reply follows the
    // reply's auto-delete policy (enabled by default); pass a parent to tie its
    // lifetime elsewhere.
    ChatCompletionReply *createChatCompletion(const Core::ChatCompletionRequest &request);

    // Start a streamed chat completion (Server-Sent Events). Forces the
    // request's `stream` flag to true and returns a ChatCompletionStreamReply
    // that emits incremental deltas. Ownership follows the reply's auto-delete
    // policy (enabled by default).
    ChatCompletionStreamReply *
    createChatCompletionStream(const Core::ChatCompletionRequest &request);

    // Classify text/image input against the moderation policy (POST /moderations).
    ModerationReply *createModeration(const Core::ModerationRequest &request);

    // Legacy text completion (POST /completions). Mainly for OpenAI-compatible
    // servers that only expose the deprecated endpoint. Ownership follows the
    // reply's auto-delete policy (enabled by default).
    CompletionReply *createCompletion(const Core::CompletionRequest &request);

    // Streamed legacy text completion (Server-Sent Events). Forces the request's
    // `stream` flag to true and returns a CompletionStreamReply emitting text
    // deltas. Ownership follows the reply's auto-delete policy.
    CompletionStreamReply *createCompletionStream(const Core::CompletionRequest &request);

    // --- Responses API (POST/GET/DELETE /responses) ------------------------
    // Create a response. Ownership follows the reply's auto-delete policy
    // (enabled by default); pass a parent to tie its lifetime elsewhere.
    ResponseReply *createResponse(const Core::ResponseRequest &request);

    // Streamed response (Server-Sent Events). Forces the request's `stream` flag
    // to true and returns a ResponseStreamReply emitting typed events. Ownership
    // follows the reply's auto-delete policy.
    ResponseStreamReply *createResponseStream(const Core::ResponseRequest &request);

    // Retrieve a previously created (stored) response by id.
    ResponseReply *getResponse(const QString &responseId);

    // Cancel an in-progress background response.
    ResponseReply *cancelResponse(const QString &responseId);

    // Delete a stored response. On success the reply's response() carries the
    // deletion acknowledgement (object "response.deleted").
    ResponseReply *deleteResponse(const QString &responseId);

    // The input items a stored response was produced from
    // (GET /responses/{id}/input_items). The payload is a cursor-paginated list
    // of the same item model the Conversations API returns, so it shares
    // ConversationItemsReply rather than duplicating it.
    ConversationItemsReply *listResponseInputItems(const QString &responseId,
                                                   const ListParams &params = {});

    // Compact a stored response's accumulated history server-side, returning
    // the compacted response (POST /responses/compact).
    //
    // NOTE: this endpoint is newer than the OpenAPI revision this library was
    // written against, so only its path is confirmed. The body is sent as
    // {"response_id": ...}; `extra` is merged in verbatim for fields the
    // provider expects that are not modelled here.
    ResponseReply *compactResponse(const QString &responseId, const QJsonObject &extra = {});

    // Price a request's input without running it (POST /responses/input_tokens).
    //
    // NOTE: same caveat as compactResponse() — the request is sent as a normal
    // Responses body and the reply exposes both the parsed `input_tokens` count
    // and the raw payload, so an unexpected shape stays reachable.
    InputTokensReply *countResponseInputTokens(const Core::ResponseRequest &request);

    // --- Conversations API (/conversations) --------------------------------
    // Create a conversation, optionally seeded with items and metadata.
    ConversationReply *createConversation(const QJsonObject &metadata = {},
                                          const QList<Core::ResponseOutputItem> &items = {});

    ConversationReply *getConversation(const QString &conversationId);

    // Replace the conversation's metadata.
    ConversationReply *updateConversation(const QString &conversationId,
                                          const QJsonObject &metadata);

    ConversationReply *deleteConversation(const QString &conversationId);

    // List the items in a conversation (most-recent-first by default).
    ConversationItemsReply *listConversationItems(const QString &conversationId);

    // Append items to a conversation.
    ConversationItemsReply *createConversationItems(const QString &conversationId,
                                                    const QList<Core::ResponseOutputItem> &items);

    // Fetch a single conversation item (surfaced as a one-item list).
    ConversationItemsReply *getConversationItem(const QString &conversationId,
                                                const QString &itemId);

    // Delete an item; on success the reply carries the updated conversation.
    ConversationReply *deleteConversationItem(const QString &conversationId, const QString &itemId);

    // --- Stored Chat Completions management (/chat/completions/{id}) --------
    // List stored chat completions (created with store: true).
    ChatCompletionListReply *listChatCompletions(const ListParams &params = {});

    // Retrieve a stored chat completion by id.
    ChatCompletionReply *getChatCompletion(const QString &completionId);

    // Replace a stored completion's metadata.
    ChatCompletionReply *updateChatCompletion(const QString &completionId,
                                              const QJsonObject &metadata);

    // Delete a stored chat completion.
    ChatCompletionReply *deleteChatCompletion(const QString &completionId);

    // List the input messages of a stored chat completion.
    ChatCompletionMessageListReply *listChatCompletionMessages(const QString &completionId,
                                                               const ListParams &params = {});

    // --- Embeddings (/embeddings) ------------------------------------------
    // Create embeddings for the request's input.
    EmbeddingReply *createEmbeddings(const Core::EmbeddingRequest &request);

    // --- Audio: speech-to-text ---------------------------------------------
    // Transcribe audio in its source language (POST /audio/transcriptions).
    // Uploads the request's audio bytes as multipart/form-data. Ownership
    // follows the reply's auto-delete policy (enabled by default).
    TranscriptionReply *createTranscription(const Core::TranscriptionRequest &request);

    // Translate audio into English (POST /audio/translations). Same multipart
    // upload; the reply carries the translated transcript.
    TranscriptionReply *createTranslation(const Core::TranslationRequest &request);
    // --- Audio: text-to-speech (/audio/speech) -----------------------------
    // Synthesise speech from text. The reply exposes the raw audio bytes and
    // the response Content-Type. Ownership follows the reply's auto-delete
    // policy (enabled by default).
    SpeechReply *createSpeech(const Core::SpeechRequest &request);

    // --- Audio: custom voices (/audio/voices, /audio/voice_consents) -------
    // Build a custom voice from an audio sample, authorised by an accepted
    // consent. Multipart upload; the resulting voice becomes usable as the
    // `voice` of a text-to-speech request once its voiceStatus() says so.
    //
    // The spec defines no list endpoint for voices, so there is no listVoices()
    // here — only creation.
    VoiceReply *createVoice(const Core::CreateVoiceRequest &request);

    // Record the spoken consent that authorises cloning a voice (multipart).
    VoiceConsentReply *createVoiceConsent(const Core::CreateVoiceConsentRequest &request);

    VoiceConsentListReply *listVoiceConsents(const ListParams &params = {});

    VoiceConsentReply *getVoiceConsent(const QString &consentId);

    // Rename a consent (POST /audio/voice_consents/{id}).
    VoiceConsentReply *updateVoiceConsent(const QString &consentId, const QString &name);

    // Delete a consent. On success the reply's consent() carries the deletion
    // acknowledgement.
    VoiceConsentReply *deleteVoiceConsent(const QString &consentId);

    // --- Images (/images) --------------------------------------------------
    // Generate images from a text prompt (POST /images/generations).
    ImageReply *createImage(const Core::ImageGenerationRequest &request);

    // Edit an image given a prompt (and optional mask) (POST /images/edits).
    // Uploads the source image(s)/mask as multipart/form-data.
    ImageReply *createImageEdit(const Core::ImageEditRequest &request);

    // Produce variations of a source image (POST /images/variations, dall-e-2).
    ImageReply *createImageVariation(const Core::ImageVariationRequest &request);

    // --- Video / Sora (/videos) --------------------------------------------
    // Start an asynchronous video-generation job (POST /videos). The returned
    // job starts in the `queued` state; poll getVideo() (or use pollVideo()) to
    // follow its progress. When the request carries an input reference the body
    // is uploaded as multipart/form-data, otherwise as JSON.
    VideoReply *createVideo(const Core::CreateVideoRequest &request);

    // Retrieve a single video job's current state (GET /videos/{id}).
    VideoReply *getVideo(const QString &videoId);

    // List video jobs (GET /videos), most-recent-first by default.
    VideoListReply *listVideos(const ListParams &params = {});

    // Delete a video job (DELETE /videos/{id}). On success the reply's job()
    // carries the deletion acknowledgement.
    VideoReply *deleteVideo(const QString &videoId);

    // Create a new job that remixes an existing completed video with a new
    // prompt (POST /videos/{id}/remix).
    VideoReply *remixVideo(const QString &videoId, const QString &prompt);

    // Download the rendered video bytes of a completed job
    // (GET /videos/{id}/content). The reply exposes the raw bytes and the
    // response Content-Type, mirroring createSpeech().
    VideoContentReply *downloadVideoContent(const QString &videoId);

    // Poll a video job until it reaches a terminal state. Returns a VideoPoller
    // that emits progressed()/completed()/failed(); call start() to begin.
    // Ownership follows the poller's auto-delete policy (enabled by default).
    VideoPoller *pollVideo(const QString &videoId, int pollIntervalMs = 2000);

    // --- Files (/files) ----------------------------------------------------
    // Upload a file for use by fine-tuning, batch, assistants, vector stores or
    // the Responses file inputs (POST /files). The bytes go out as
    // multipart/form-data. Ownership follows the reply's auto-delete policy.
    FileReply *uploadFile(const Core::FileUploadRequest &request);

    // List the uploaded files (GET /files), optionally restricted to a single
    // `purpose` (empty lists every purpose).
    FileListReply *listFiles(const ListParams &params = {}, const QString &purpose = {});

    // Retrieve a single file's metadata (GET /files/{id}).
    FileReply *getFile(const QString &fileId);

    // Delete a file (DELETE /files/{id}). On success the reply's file() carries
    // the deletion acknowledgement (object "file.deleted").
    FileReply *deleteFile(const QString &fileId);

    // Download a file's contents (GET /files/{id}/content). The reply exposes
    // the raw bytes and the response Content-Type.
    BinaryReply *downloadFileContent(const QString &fileId);

    // --- Uploads (/uploads) ------------------------------------------------
    // Open a multipart upload for a file larger than the POST /files limit. The
    // total size is declared up front; parts follow, then completeUpload().
    UploadReply *createUpload(const Core::CreateUploadRequest &request);

    // Add one chunk to an open upload (POST /uploads/{id}/parts), sent as
    // multipart/form-data. The reply carries the part id to replay to
    // completeUpload().
    UploadPartReply *addUploadPart(const QString &uploadId, const QByteArray &data);

    // Assemble an upload from its parts, in the given order
    // (POST /uploads/{id}/complete). Pass the payload's `md5` to have the server
    // verify the result; empty omits the check. On success the reply's upload()
    // carries the finished file in file().
    UploadReply *completeUpload(const QString &uploadId, const QStringList &partIds,
                                const QString &md5 = {});

    // Abort an open upload; its parts are discarded
    // (POST /uploads/{id}/cancel).
    UploadReply *cancelUpload(const QString &uploadId);

    // Run the whole start -> parts -> complete flow over `source`, reading one
    // chunk at a time. Returns a ChunkedUploader emitting
    // progressed()/completed()/failed(); call start() to begin. The device is
    // not adopted and must outlive the run. Ownership of the uploader follows
    // its auto-delete policy (enabled by default).
    ChunkedUploader *uploadInChunks(const Core::CreateUploadRequest &request, QIODevice *source,
                                    qint64 chunkSize = ChunkedUploader::defaultChunkSize);

    // The same flow over an in-memory payload; the request's bytes() may be left
    // at 0 and is then derived from the data.
    ChunkedUploader *uploadInChunks(const Core::CreateUploadRequest &request,
                                    const QByteArray &data,
                                    qint64 chunkSize = ChunkedUploader::defaultChunkSize);

    // --- Vector stores (/vector_stores) ------------------------------------
    // Create a vector store, optionally seeded with already-uploaded file ids.
    VectorStoreReply *createVectorStore(const Core::CreateVectorStoreRequest &request);

    VectorStoreListReply *listVectorStores(const ListParams &params = {});

    VectorStoreReply *getVectorStore(const QString &vectorStoreId);

    // Modify a store; the request carries only the fields to change.
    VectorStoreReply *updateVectorStore(const QString &vectorStoreId,
                                        const Core::CreateVectorStoreRequest &request);

    // Delete a store. On success the reply's store() carries the deletion
    // acknowledgement (object "vector_store.deleted").
    VectorStoreReply *deleteVectorStore(const QString &vectorStoreId);

    // Attach an uploaded file to a store; ingestion then runs asynchronously.
    VectorStoreFileReply *createVectorStoreFile(const QString &vectorStoreId, const QString &fileId,
                                                const QJsonObject &chunkingStrategy = {},
                                                const QJsonObject &attributes = {});

    // List a store's files, optionally restricted to one ingestion status
    // ("in_progress", "completed", "cancelled" or "failed").
    VectorStoreFileListReply *listVectorStoreFiles(const QString &vectorStoreId,
                                                   const ListParams &params = {},
                                                   const QString &filter = {});

    VectorStoreFileReply *getVectorStoreFile(const QString &vectorStoreId, const QString &fileId);

    // Replace a file's search attributes.
    VectorStoreFileReply *updateVectorStoreFileAttributes(const QString &vectorStoreId,
                                                          const QString &fileId,
                                                          const QJsonObject &attributes);

    // Detach a file from the store (the underlying file itself is kept).
    VectorStoreFileReply *deleteVectorStoreFile(const QString &vectorStoreId,
                                                const QString &fileId);

    // Fetch the parsed text chunks a file contributed to the index.
    VectorStoreFileContentReply *getVectorStoreFileContent(const QString &vectorStoreId,
                                                           const QString &fileId);

    // Attach many files in one call, so a bulk ingest needs a single poll.
    VectorStoreFileBatchReply *createVectorStoreFileBatch(const QString &vectorStoreId,
                                                          const QStringList &fileIds,
                                                          const QJsonObject &chunkingStrategy = {},
                                                          const QJsonObject &attributes = {});

    VectorStoreFileBatchReply *getVectorStoreFileBatch(const QString &vectorStoreId,
                                                       const QString &batchId);

    VectorStoreFileBatchReply *cancelVectorStoreFileBatch(const QString &vectorStoreId,
                                                          const QString &batchId);

    // List the files of one batch, with the same status filter as listVectorStoreFiles().
    VectorStoreFileListReply *listVectorStoreFileBatchFiles(const QString &vectorStoreId,
                                                            const QString &batchId,
                                                            const ListParams &params = {},
                                                            const QString &filter = {});

    // Run a semantic search over a store and return a page of ranked hits.
    VectorStoreSearchReply *searchVectorStore(const QString &vectorStoreId,
                                              const Core::VectorStoreSearchRequest &request);

    // --- Containers (/containers) ------------------------------------------
    // Create a sandboxed container, optionally pre-loaded with uploaded files.
    ContainerReply *createContainer(const Core::CreateContainerRequest &request);

    ContainerListReply *listContainers(const ListParams &params = {});

    ContainerReply *getContainer(const QString &containerId);

    // Delete a container. On success the reply's container() carries the
    // deletion acknowledgement (object "container.deleted").
    ContainerReply *deleteContainer(const QString &containerId);

    // Upload bytes straight into a container's filesystem
    // (POST /containers/{id}/files as multipart/form-data).
    ContainerFileReply *uploadContainerFile(const QString &containerId, const QString &fileName,
                                            const QByteArray &data);

    // Copy an existing Files-API file into a container instead of uploading it
    // again (the JSON `file_id` form of the same endpoint).
    ContainerFileReply *attachContainerFile(const QString &containerId, const QString &fileId);

    ContainerFileListReply *listContainerFiles(const QString &containerId,
                                               const ListParams &params = {});

    ContainerFileReply *getContainerFile(const QString &containerId, const QString &fileId);

    ContainerFileReply *deleteContainerFile(const QString &containerId, const QString &fileId);

    // Download a container file's contents
    // (GET /containers/{id}/files/{file_id}/content). The reply exposes the raw
    // bytes and the response Content-Type.
    BinaryReply *downloadContainerFileContent(const QString &containerId, const QString &fileId);

    // --- Batch (/batches) --------------------------------------------------
    // Queue a JSONL file of requests for asynchronous, discounted processing.
    // The batch starts in the `validating` state; poll getBatch() (or use
    // pollBatch()) until isTerminal(), then fetch the results from the Files API
    // with the batch's outputFileId()/errorFileId().
    BatchReply *createBatch(const Core::CreateBatchRequest &request);

    BatchListReply *listBatches(const ListParams &params = {});

    BatchReply *getBatch(const QString &batchId);

    // Request cancellation. The batch moves to `cancelling` first and reaches
    // `cancelled` once the in-flight requests have drained.
    BatchReply *cancelBatch(const QString &batchId);

    // Poll a batch until it reaches a terminal state. Returns a BatchPoller that
    // emits progressed()/completed()/failed(); call start() on it. The poller
    // deletes itself once it stops unless setAutoDelete(false) is used.
    BatchPoller *pollBatch(const QString &batchId, int pollIntervalMs = 2000);

    // --- Fine-tuning (/fine_tuning) ----------------------------------------
    // Start a training run over an uploaded JSONL training file. The job starts
    // in `validating_files`; poll getFineTuningJob() (or use
    // pollFineTuningJob()) until isTerminal(), then use the job's
    // fineTunedModel() as the model id in later requests.
    FineTuningJobReply *createFineTuningJob(const Core::CreateFineTuningJobRequest &request);

    FineTuningJobListReply *listFineTuningJobs(const ListParams &params = {});

    FineTuningJobReply *getFineTuningJob(const QString &jobId);

    // Stop a job for good.
    FineTuningJobReply *cancelFineTuningJob(const QString &jobId);

    // Suspend a running job and pick it up again later. A paused job is not
    // terminal, so a poller keeps waiting across the pause.
    FineTuningJobReply *pauseFineTuningJob(const QString &jobId);
    FineTuningJobReply *resumeFineTuningJob(const QString &jobId);

    // The job's progress log — status messages and periodic metrics samples.
    FineTuningEventListReply *listFineTuningEvents(const QString &jobId,
                                                   const ListParams &params = {});

    // Mid-training snapshots, each usable as a model of its own.
    FineTuningCheckpointListReply *listFineTuningCheckpoints(const QString &jobId,
                                                             const ListParams &params = {});

    // Poll a fine-tuning job until it reaches a terminal state. Returns a
    // FineTuningJobPoller that emits progressed()/completed()/failed(); call
    // start() on it. It deletes itself once it stops unless setAutoDelete(false)
    // is used.
    FineTuningJobPoller *pollFineTuningJob(const QString &jobId, int pollIntervalMs = 2000);

    // Which projects may use a checkpoint. Creating grants answers with the
    // whole list, so both calls share a reply type.
    FineTuningPermissionListReply *
    listFineTuningCheckpointPermissions(const QString &checkpointId, const ListParams &params = {});

    FineTuningPermissionListReply *
    createFineTuningCheckpointPermissions(const QString &checkpointId,
                                          const QStringList &projectIds);

    FineTuningPermissionReply *deleteFineTuningCheckpointPermission(const QString &checkpointId,
                                                                    const QString &permissionId);

    // --- Evals (/evals) ----------------------------------------------------
    // Define an eval: how its items are shaped plus the graders scoring them.
    EvalReply *createEval(const Core::CreateEvalRequest &request);

    EvalListReply *listEvals(const ListParams &params = {});

    EvalReply *getEval(const QString &evalId);

    // Rename an eval and/or replace its metadata (POST /evals/{id}); empty
    // arguments are left out of the body.
    EvalReply *updateEval(const QString &evalId, const QString &name,
                          const QJsonObject &metadata = {});

    // Delete an eval. On success the reply's eval() carries the deletion
    // acknowledgement (object "eval.deleted").
    EvalReply *deleteEval(const QString &evalId);

    // Execute an eval against a data source. The run starts `queued`; poll
    // getEvalRun() (or use pollEvalRun()) until isTerminal(), then read the
    // result counts and the per-item output.
    EvalRunReply *createEvalRun(const QString &evalId, const Core::CreateEvalRunRequest &request);

    EvalRunListReply *listEvalRuns(const QString &evalId, const ListParams &params = {});

    EvalRunReply *getEvalRun(const QString &evalId, const QString &runId);

    // Cancel a run. The API models this as a bare POST to the run itself rather
    // than a /cancel sub-path.
    EvalRunReply *cancelEvalRun(const QString &evalId, const QString &runId);

    EvalRunReply *deleteEvalRun(const QString &evalId, const QString &runId);

    // Per-item results of a finished run.
    EvalRunOutputItemListReply *listEvalRunOutputItems(const QString &evalId, const QString &runId,
                                                       const ListParams &params = {});

    EvalRunOutputItemReply *getEvalRunOutputItem(const QString &evalId, const QString &runId,
                                                 const QString &outputItemId);

    // Poll a run until it reaches a terminal state. Returns an EvalRunPoller
    // that emits progressed()/completed()/failed(); call start() on it. It
    // deletes itself once it stops unless setAutoDelete(false) is used.
    EvalRunPoller *pollEvalRun(const QString &evalId, const QString &runId,
                               int pollIntervalMs = 2000);

    // --- Models (/models) --------------------------------------------------
    // List the available models.
    ModelListReply *listModels();

    // Retrieve a single model by id.
    ModelReply *getModel(const QString &modelId);

Q_SIGNALS:
    void baseUrlChanged();
    void apiKeyChanged();
    void organizationChanged();

private:
    // cancel/pause/resume differ only in the path segment they POST to.
    FineTuningJobReply *postFineTuningJobAction(const QString &jobId, const QString &action);

    Q_DECLARE_PRIVATE(Client)
    QScopedPointer<ClientPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
