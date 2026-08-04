// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ModelCatalog.h"

#include <QtCore/QHash>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

constexpr QLatin1String kCl100k("cl100k_base");
constexpr QLatin1String kO200k("o200k_base");

// The bundled table. Prices are US dollars per million tokens as published in
// mid-2026; they are a snapshot, not a promise -- ModelCatalog::merge() is the
// supported way to correct them without waiting for a release of this library.
struct Entry
{
    QLatin1String id;
    int contextWindow;
    int maxOutputTokens;
    QLatin1String encoding;
    int capabilities;
    double inputPrice;
    double outputPrice;
    double cachedInputPrice;
};

constexpr int kChat
        = ModelCapability::Tools | ModelCapability::StructuredOutputs | ModelCapability::Streaming;
constexpr int kChatVision = kChat | ModelCapability::Vision;
constexpr int kReasoning = kChat | ModelCapability::Reasoning;

constexpr Entry kDefaults[] = {
        // GPT-4.1 family: the long-context line.
        {QLatin1String("gpt-4.1"), 1047576, 32768, kO200k, kChatVision, 2.00, 8.00, 0.50},
        {QLatin1String("gpt-4.1-mini"), 1047576, 32768, kO200k, kChatVision, 0.40, 1.60, 0.10},
        {QLatin1String("gpt-4.1-nano"), 1047576, 32768, kO200k, kChatVision, 0.10, 0.40, 0.025},

        // GPT-4o family.
        {QLatin1String("gpt-4o"), 128000, 16384, kO200k, kChatVision | ModelCapability::Audio, 2.50,
         10.00, 1.25},
        {QLatin1String("gpt-4o-mini"), 128000, 16384, kO200k, kChatVision | ModelCapability::Audio,
         0.15, 0.60, 0.075},

        // Reasoning models: a large window, most of which they may spend thinking.
        {QLatin1String("o3-mini"), 200000, 100000, kO200k, kReasoning, 1.10, 4.40, 0.55},
        {QLatin1String("o1"), 200000, 100000, kO200k, kReasoning | ModelCapability::Vision, 15.00,
         60.00, 7.50},
        {QLatin1String("o1-mini"), 128000, 65536, kO200k, kReasoning, 1.10, 4.40, 0.55},

        // The cl100k_base generation, still reachable and still worth counting for.
        {QLatin1String("gpt-4-turbo"), 128000, 4096, kCl100k, kChatVision, 10.00, 30.00, 0},
        {QLatin1String("gpt-4"), 8192, 8192, kCl100k, kChat, 30.00, 60.00, 0},
        {QLatin1String("gpt-3.5-turbo"), 16385, 4096, kCl100k, kChat, 0.50, 1.50, 0},

        // Embeddings: an input limit and an input price, and nothing else.
        {QLatin1String("text-embedding-3-small"), 8191, 0, kCl100k, ModelCapability::None, 0.02, 0,
         0},
        {QLatin1String("text-embedding-3-large"), 8191, 0, kCl100k, ModelCapability::None, 0.13, 0,
         0},
};

ModelInfo makeInfo(const Entry &entry)
{
    ModelInfo info(entry.id);
    info.setContextWindow(entry.contextWindow);
    info.setMaxOutputTokens(entry.maxOutputTokens);
    info.setEncoding(entry.encoding);
    info.setCapabilities(ModelCapability::Flags(entry.capabilities));
    info.setInputPrice(entry.inputPrice);
    info.setOutputPrice(entry.outputPrice);
    info.setCachedInputPrice(entry.cachedInputPrice);
    return info;
}

// What an id nothing matches is worth assuming: enough context to be useful,
// the current encoding, and no price, because guessing one would be worse than
// admitting to none.
ModelInfo makeFallback()
{
    ModelInfo info;
    info.setKnown(false);
    info.setContextWindow(8192);
    info.setMaxOutputTokens(4096);
    info.setEncoding(kO200k);
    info.setCapabilities(ModelCapability::Flags(kChat));
    return info;
}

} // namespace

class ModelCatalogData : public QSharedData
{
public:
    QHash<QString, ModelInfo> entries;
    QStringList order;
    ModelInfo fallback = makeFallback();

    void put(const ModelInfo &info)
    {
        if (!entries.contains(info.id()))
            order.append(info.id());
        entries.insert(info.id(), info);
    }
};

ModelCatalog::ModelCatalog()
    : d(new ModelCatalogData)
{ }

ModelCatalog::ModelCatalog(const ModelCatalog &other) = default;
ModelCatalog::ModelCatalog(ModelCatalog &&other) noexcept = default;
ModelCatalog &ModelCatalog::operator=(const ModelCatalog &other) = default;
ModelCatalog &ModelCatalog::operator=(ModelCatalog &&other) noexcept = default;
ModelCatalog::~ModelCatalog() = default;

ModelCatalog ModelCatalog::defaults()
{
    ModelCatalog catalog;
    for (const Entry &entry : kDefaults)
        catalog.d->put(makeInfo(entry));
    return catalog;
}

ModelCatalog &ModelCatalog::shared()
{
    static ModelCatalog catalog = defaults();
    return catalog;
}

ModelInfo ModelCatalog::model(const QString &id) const
{
    auto it = d->entries.constFind(id);
    if (it != d->entries.constEnd())
        return it.value();

    // Model ids are versioned by suffix -- "gpt-4o-mini-2024-07-18" is the
    // entry for "gpt-4o-mini". The longest prefix wins, so "gpt-4.1-mini-x"
    // does not fall back to "gpt-4.1".
    QString best;
    for (const QString &candidate : d->order) {
        if (id.startsWith(candidate) && candidate.size() > best.size())
            best = candidate;
    }
    if (!best.isEmpty()) {
        ModelInfo info = d->entries.value(best);
        // The answer is about the id that was asked for, whichever entry
        // supplied the facts.
        info.setId(id);
        return info;
    }

    ModelInfo fallback = d->fallback;
    fallback.setId(id);
    return fallback;
}

bool ModelCatalog::contains(const QString &id) const { return d->entries.contains(id); }

ModelInfo ModelCatalog::entry(const QString &id) const { return d->entries.value(id); }

QStringList ModelCatalog::ids() const { return d->order; }

int ModelCatalog::count() const { return d->entries.size(); }

bool ModelCatalog::isEmpty() const { return d->entries.isEmpty(); }

void ModelCatalog::insert(const ModelInfo &info)
{
    if (!info.id().isEmpty())
        d->put(info);
}

bool ModelCatalog::remove(const QString &id)
{
    if (!d->entries.remove(id))
        return false;
    d->order.removeAll(id);
    return true;
}

void ModelCatalog::clear()
{
    d->entries.clear();
    d->order.clear();
}

ModelInfo ModelCatalog::fallback() const { return d->fallback; }

void ModelCatalog::setFallback(const ModelInfo &fallback) { d->fallback = fallback; }

void ModelCatalog::merge(const QJsonObject &json)
{
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        if (!it.value().isObject())
            continue;
        ModelInfo info = ModelInfo::fromJson(it.value().toObject());
        // The key is the id, so a file does not have to repeat it inside.
        info.setId(it.key());
        d->put(info);
    }
}

QJsonObject ModelCatalog::toJson() const
{
    QJsonObject json;
    for (const QString &id : d->order) {
        QJsonObject entry = d->entries.value(id).toJson();
        entry.remove(QStringLiteral("id"));
        json.insert(id, entry);
    }
    return json;
}

bool ModelCatalog::operator==(const ModelCatalog &other) const
{
    return d->entries == other.d->entries && d->fallback == other.d->fallback;
}

} // namespace Core
} // namespace QtOpenAi
