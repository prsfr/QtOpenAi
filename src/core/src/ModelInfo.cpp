// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/ModelInfo.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

// Capabilities travel as a list of names, so a catalog file stays readable and
// a name this version does not know is simply skipped rather than shifting
// every other flag.
struct CapabilityName
{
    ModelCapability::Flag flag;
    QLatin1String name;
};

constexpr CapabilityName kCapabilityNames[] = {
        {ModelCapability::Tools, QLatin1String("tools")},
        {ModelCapability::Vision, QLatin1String("vision")},
        {ModelCapability::Audio, QLatin1String("audio")},
        {ModelCapability::StructuredOutputs, QLatin1String("structured_outputs")},
        {ModelCapability::Streaming, QLatin1String("streaming")},
        {ModelCapability::Reasoning, QLatin1String("reasoning")},
};

QJsonArray capabilitiesToJson(ModelCapability::Flags capabilities)
{
    QJsonArray names;
    for (const CapabilityName &entry : kCapabilityNames) {
        if (capabilities.testFlag(entry.flag))
            names.append(QString(entry.name));
    }
    return names;
}

ModelCapability::Flags capabilitiesFromJson(const QJsonArray &names)
{
    ModelCapability::Flags capabilities;
    for (const QJsonValue &value : names) {
        const QString name = value.toString();
        for (const CapabilityName &entry : kCapabilityNames) {
            if (name == entry.name)
                capabilities |= entry.flag;
        }
    }
    return capabilities;
}

} // namespace

class ModelInfoData : public QSharedData
{
public:
    QString id;
    bool known = true;
    int contextWindow = 0;
    int maxOutputTokens = 0;
    QString encoding;
    ModelCapability::Flags capabilities;
    double inputPrice = 0;
    double outputPrice = 0;
    double cachedInputPrice = 0;
};

ModelInfo::ModelInfo()
    : d(new ModelInfoData)
{ }

ModelInfo::ModelInfo(const QString &id)
    : d(new ModelInfoData)
{
    d->id = id;
}

ModelInfo::ModelInfo(const ModelInfo &other) = default;
ModelInfo::ModelInfo(ModelInfo &&other) noexcept = default;
ModelInfo &ModelInfo::operator=(const ModelInfo &other) = default;
ModelInfo &ModelInfo::operator=(ModelInfo &&other) noexcept = default;
ModelInfo::~ModelInfo() = default;

QString ModelInfo::id() const { return d->id; }
void ModelInfo::setId(const QString &id) { d->id = id; }

bool ModelInfo::isKnown() const { return d->known; }
void ModelInfo::setKnown(bool known) { d->known = known; }

int ModelInfo::contextWindow() const { return d->contextWindow; }
void ModelInfo::setContextWindow(int tokens) { d->contextWindow = tokens; }

int ModelInfo::maxOutputTokens() const { return d->maxOutputTokens; }
void ModelInfo::setMaxOutputTokens(int tokens) { d->maxOutputTokens = tokens; }

QString ModelInfo::encoding() const { return d->encoding; }
void ModelInfo::setEncoding(const QString &encoding) { d->encoding = encoding; }

ModelCapability::Flags ModelInfo::capabilities() const { return d->capabilities; }
void ModelInfo::setCapabilities(ModelCapability::Flags capabilities)
{
    d->capabilities = capabilities;
}

bool ModelInfo::supports(ModelCapability::Flags capabilities) const
{
    return (d->capabilities & capabilities) == capabilities;
}

double ModelInfo::inputPrice() const { return d->inputPrice; }
void ModelInfo::setInputPrice(double price) { d->inputPrice = price; }

double ModelInfo::outputPrice() const { return d->outputPrice; }
void ModelInfo::setOutputPrice(double price) { d->outputPrice = price; }

double ModelInfo::cachedInputPrice() const { return d->cachedInputPrice; }
void ModelInfo::setCachedInputPrice(double price) { d->cachedInputPrice = price; }

QJsonObject ModelInfo::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("context_window"), d->contextWindow);
    json.insert(QStringLiteral("max_output_tokens"), d->maxOutputTokens);
    detail::insertIfNotEmpty(json, QStringLiteral("encoding"), d->encoding);
    json.insert(QStringLiteral("capabilities"), capabilitiesToJson(d->capabilities));
    json.insert(QStringLiteral("input_price"), d->inputPrice);
    json.insert(QStringLiteral("output_price"), d->outputPrice);
    if (d->cachedInputPrice > 0)
        json.insert(QStringLiteral("cached_input_price"), d->cachedInputPrice);
    return json;
}

ModelInfo ModelInfo::fromJson(const QJsonObject &json)
{
    ModelInfo info;
    info.d->id = detail::stringOr(json, QStringLiteral("id"));
    info.d->contextWindow = json.value(QStringLiteral("context_window")).toInt();
    info.d->maxOutputTokens = json.value(QStringLiteral("max_output_tokens")).toInt();
    info.d->encoding = detail::stringOr(json, QStringLiteral("encoding"));
    info.d->capabilities
            = capabilitiesFromJson(json.value(QStringLiteral("capabilities")).toArray());
    info.d->inputPrice = json.value(QStringLiteral("input_price")).toDouble();
    info.d->outputPrice = json.value(QStringLiteral("output_price")).toDouble();
    info.d->cachedInputPrice = json.value(QStringLiteral("cached_input_price")).toDouble();
    return info;
}

bool ModelInfo::operator==(const ModelInfo &other) const
{
    return d->id == other.d->id && d->known == other.d->known
           && d->contextWindow == other.d->contextWindow
           && d->maxOutputTokens == other.d->maxOutputTokens && d->encoding == other.d->encoding
           && d->capabilities == other.d->capabilities
           && qFuzzyCompare(d->inputPrice, other.d->inputPrice)
           && qFuzzyCompare(d->outputPrice, other.d->outputPrice)
           && qFuzzyCompare(d->cachedInputPrice, other.d->cachedInputPrice);
}

} // namespace Core
} // namespace QtOpenAi
