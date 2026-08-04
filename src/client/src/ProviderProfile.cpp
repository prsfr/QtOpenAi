// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ProviderProfile.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Client {

namespace {

// The Azure API version this library was written against. It is a date, and
// Azure keeps issuing new ones; the argument to azure() exists because this
// will age.
constexpr QLatin1String kDefaultAzureApiVersion("2024-10-21");

ProviderProfile make(const QString &name, const QString &baseUrl,
                     Client::AuthScheme scheme = Client::AuthScheme::BearerToken)
{
    ProviderProfile profile;
    profile.setName(name);
    profile.setBaseUrl(QUrl(baseUrl));
    profile.setAuthScheme(scheme);
    return profile;
}

// A local server needs no key, and asking a user for one they do not have is
// worse than not asking.
ProviderProfile makeLocal(const QString &name, const QString &baseUrl)
{
    ProviderProfile profile = make(name, baseUrl);
    profile.setRequiresApiKey(false);
    return profile;
}

} // namespace

class ProviderProfileData : public QSharedData
{
public:
    QString name;
    QUrl baseUrl;
    Client::AuthScheme authScheme = Client::AuthScheme::BearerToken;
    QString apiVersion;
    QHash<QByteArray, QByteArray> headers;
    bool requiresApiKey = true;
    QString defaultModel;
};

ProviderProfile::ProviderProfile()
    : d(new ProviderProfileData)
{ }

ProviderProfile::ProviderProfile(const ProviderProfile &other) = default;
ProviderProfile::ProviderProfile(ProviderProfile &&other) noexcept = default;
ProviderProfile &ProviderProfile::operator=(const ProviderProfile &other) = default;
ProviderProfile &ProviderProfile::operator=(ProviderProfile &&other) noexcept = default;
ProviderProfile::~ProviderProfile() = default;

ProviderProfile ProviderProfile::openAi()
{
    ProviderProfile profile
            = make(QStringLiteral("OpenAI"), QStringLiteral("https://api.openai.com/v1"));
    profile.setDefaultModel(QStringLiteral("gpt-4o-mini"));
    return profile;
}

ProviderProfile ProviderProfile::azure(const QString &resource, const QString &apiVersion)
{
    ProviderProfile profile
            = make(QStringLiteral("Azure OpenAI"),
                   QStringLiteral("https://%1.openai.azure.com/openai/v1").arg(resource),
                   Client::AuthScheme::AzureApiKey);
    profile.setApiVersion(apiVersion.isEmpty() ? QString(kDefaultAzureApiVersion) : apiVersion);
    return profile;
}

ProviderProfile ProviderProfile::ollama()
{
    return makeLocal(QStringLiteral("Ollama"), QStringLiteral("http://localhost:11434/v1"));
}

ProviderProfile ProviderProfile::lmStudio()
{
    return makeLocal(QStringLiteral("LM Studio"), QStringLiteral("http://localhost:1234/v1"));
}

ProviderProfile ProviderProfile::vllm()
{
    return makeLocal(QStringLiteral("vLLM"), QStringLiteral("http://localhost:8000/v1"));
}

ProviderProfile ProviderProfile::groq()
{
    return make(QStringLiteral("Groq"), QStringLiteral("https://api.groq.com/openai/v1"));
}

ProviderProfile ProviderProfile::openRouter()
{
    return make(QStringLiteral("OpenRouter"), QStringLiteral("https://openrouter.ai/api/v1"));
}

QList<ProviderProfile> ProviderProfile::builtIn()
{
    // Azure is absent: it cannot be built without a resource name, so there is
    // no argument-free profile of it to offer.
    return {openAi(), ollama(), lmStudio(), vllm(), groq(), openRouter()};
}

ProviderProfile ProviderProfile::fromName(const QString &name)
{
    for (const ProviderProfile &profile : builtIn()) {
        if (profile.name().compare(name, Qt::CaseInsensitive) == 0)
            return profile;
    }
    return {};
}

QString ProviderProfile::name() const { return d->name; }
void ProviderProfile::setName(const QString &name) { d->name = name; }

QUrl ProviderProfile::baseUrl() const { return d->baseUrl; }
void ProviderProfile::setBaseUrl(const QUrl &baseUrl) { d->baseUrl = baseUrl; }

Client::AuthScheme ProviderProfile::authScheme() const { return d->authScheme; }
void ProviderProfile::setAuthScheme(Client::AuthScheme scheme) { d->authScheme = scheme; }

QString ProviderProfile::apiVersion() const { return d->apiVersion; }
void ProviderProfile::setApiVersion(const QString &apiVersion) { d->apiVersion = apiVersion; }

QHash<QByteArray, QByteArray> ProviderProfile::headers() const { return d->headers; }

void ProviderProfile::setHeader(const QByteArray &name, const QByteArray &value)
{
    d->headers.insert(name, value);
}

void ProviderProfile::setHeaders(const QHash<QByteArray, QByteArray> &headers)
{
    d->headers = headers;
}

bool ProviderProfile::requiresApiKey() const { return d->requiresApiKey; }
void ProviderProfile::setRequiresApiKey(bool required) { d->requiresApiKey = required; }

QString ProviderProfile::defaultModel() const { return d->defaultModel; }
void ProviderProfile::setDefaultModel(const QString &model) { d->defaultModel = model; }

bool ProviderProfile::isNull() const { return d->name.isEmpty() && d->baseUrl.isEmpty(); }

void ProviderProfile::applyTo(Client *client) const
{
    if (!client)
        return;

    // The API key is deliberately not part of a profile: a profile says which
    // provider, a key says who you are, and putting a secret in a value type
    // that gets copied and logged is how secrets escape.
    if (!d->baseUrl.isEmpty())
        client->setBaseUrl(d->baseUrl);
    client->setAuthScheme(d->authScheme);
    client->setApiVersion(d->apiVersion);
    for (auto it = d->headers.constBegin(); it != d->headers.constEnd(); ++it)
        client->setDefaultHeader(it.key(), it.value());
}

bool ProviderProfile::operator==(const ProviderProfile &other) const
{
    return d->name == other.d->name && d->baseUrl == other.d->baseUrl
           && d->authScheme == other.d->authScheme && d->apiVersion == other.d->apiVersion
           && d->headers == other.d->headers && d->requiresApiKey == other.d->requiresApiKey
           && d->defaultModel == other.d->defaultModel;
}

} // namespace Client
} // namespace QtOpenAi
