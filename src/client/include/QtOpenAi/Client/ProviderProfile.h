// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/GlobalClient.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QUrl>

namespace QtOpenAi {
namespace Client {

class ProviderProfileData;

// Everything that differs between one OpenAI-compatible provider and the next.
//
// The endpoints are the same; the way in is not. A base URL, how the key is
// presented, a query parameter Azure needs and nobody else does, a header one
// provider wants. Configuring that by hand is four calls a caller has to know
// about, and a profile is those four calls named after the provider:
//
//     client.setProfile(ProviderProfile::groq());
//     client.setApiKey(key);
//
// The profile is a value: take a built-in one, change what you need, apply it.
// Nothing here is exclusive to the built-ins — a provider this library has
// never heard of is a default-constructed profile with its URL set.
//
// **Azure needs a word.** This configures the key header and the `api-version`
// parameter, which is what an Azure endpoint speaking the OpenAI-compatible
// path shape needs. It does *not* rewrite paths into the
// `/openai/deployments/<deployment>/…` form of the older Azure API; that is a
// different path grammar, not a different profile, and pretending otherwise
// with a base URL would produce requests that quietly 404.
class QTOPENAI_CLIENT_EXPORT ProviderProfile
{
public:
    ProviderProfile();
    ProviderProfile(const ProviderProfile &other);
    ProviderProfile(ProviderProfile &&other) noexcept;
    ProviderProfile &operator=(const ProviderProfile &other);
    ProviderProfile &operator=(ProviderProfile &&other) noexcept;
    ~ProviderProfile();

    void swap(ProviderProfile &other) noexcept { d.swap(other.d); }

    // --- The built-ins ----------------------------------------------------
    static ProviderProfile openAi();
    // `resource` is the Azure resource name: <resource>.openai.azure.com.
    static ProviderProfile azure(const QString &resource, const QString &apiVersion = QString());
    static ProviderProfile ollama();
    static ProviderProfile lmStudio();
    static ProviderProfile vllm();
    static ProviderProfile groq();
    static ProviderProfile openRouter();

    // Every built-in that needs no argument, so a UI can offer a list.
    static QList<ProviderProfile> builtIn();
    // One of them by name, case-insensitively; a null profile when there is no
    // such provider, which `isNull()` reports.
    static ProviderProfile fromName(const QString &name);

    // --- What a profile is ------------------------------------------------
    QString name() const;
    void setName(const QString &name);

    QUrl baseUrl() const;
    void setBaseUrl(const QUrl &baseUrl);

    Client::AuthScheme authScheme() const;
    void setAuthScheme(Client::AuthScheme scheme);

    // Azure's `api-version` query parameter; empty everywhere else.
    QString apiVersion() const;
    void setApiVersion(const QString &apiVersion);

    // Headers this provider wants on every request.
    QHash<QByteArray, QByteArray> headers() const;
    void setHeader(const QByteArray &name, const QByteArray &value);
    void setHeaders(const QHash<QByteArray, QByteArray> &headers);

    // Whether a key is needed at all. The local servers accept requests
    // without one, which is worth knowing before prompting for it.
    bool requiresApiKey() const;
    void setRequiresApiKey(bool required);

    // The model to reach for when the caller names none. Only filled in where
    // this library can be sure of it: a wrong model id fails the request, so an
    // empty default -- meaning "you name it" -- is the better answer for a
    // provider whose catalogue is its own business.
    QString defaultModel() const;
    void setDefaultModel(const QString &model);

    bool isNull() const;

    // Apply to a client. Equivalent to Client::setProfile(), from the other
    // side, for code that holds the profile rather than the client.
    void applyTo(Client *client) const;

    bool operator==(const ProviderProfile &other) const;
    bool operator!=(const ProviderProfile &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ProviderProfileData> d;
};

} // namespace Client
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Client::ProviderProfile)
