// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/MetaSchema.h>
#include <QtOpenAi/Tools/GlobalTools.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

class QNetworkAccessManager;

namespace QtOpenAi {
namespace Tools {

// A single, deliberately dull HTTP GET, behind a host allow-list.
//
//     HttpTools http;
//     http.setAllowedHosts({QStringLiteral("docs.example.com")});
//     registry.registerMethod(&http, QStringLiteral("http_get"));
//
// Letting a model fetch URLs is the most dangerous ordinary tool there is, and
// not because of what it downloads. A model asked to summarise a page will
// happily fetch `http://169.254.169.254/` or `http://localhost:8080/admin`, and
// the request goes out from inside the network the application is running in.
// The allow-list is the whole defence, so it is required rather than
// recommended: with no hosts allowed, nothing is fetched.
//
// The rest of the limits exist because a tool result goes straight into a
// context window and a request holds a slot until it finishes:
//
//   * **https only**, by default. A plaintext fetch can be rewritten in transit
//     by anyone on the path, and the result is then instructions the model will
//     read.
//   * **A size cap**, enforced as the body arrives rather than after, so a
//     hostile server cannot make the cap irrelevant by sending more.
//   * **A timeout**, so a server that never answers does not hold the turn open.
//
// Redirects are not followed. A redirect is a host the allow-list never
// approved, and following one would hand the decision to the server.
class QTOPENAI_TOOLS_EXPORT HttpTools : public QObject
{
    Q_OBJECT
    QTOPENAI_DOC("Fetch pages over HTTP from an allowed set of hosts.")
public:
    explicit HttpTools(QObject *parent = nullptr);
    ~HttpTools() override;

    // Hosts that may be fetched, compared case-insensitively and exactly --
    // "example.com" does not allow "evil.example.com", because a wildcard is
    // the kind of thing that looks harmless until someone registers the
    // subdomain. Empty (the default) allows nothing.
    QStringList allowedHosts() const;
    void setAllowedHosts(const QStringList &hosts);
    void addAllowedHost(const QString &host);

    // Refuse anything that is not https. True by default.
    bool requiresHttps() const;
    void setRequiresHttps(bool required);

    // Largest body accepted, in bytes. Default 256 KiB. 0 means no limit.
    qint64 maxBytes() const;
    void setMaxBytes(qint64 bytes);

    // How long to wait, in milliseconds. Default 10000.
    int timeoutMs() const;
    void setTimeoutMs(int timeoutMs);

    // Inject a manager, for a proxy or for a test. Not owned.
    void setNetworkAccessManager(QNetworkAccessManager *manager);
    QNetworkAccessManager *networkAccessManager() const;

    QTOPENAI_DOC_METHOD(http_get, "Fetch a URL with GET and return the response body as text.", url,
                        "The absolute URL to fetch.")
    // Blocks on a local event loop until the fetch settles. A tool result has
    // to be a value by the time the registry returns it, and the alternative --
    // an asynchronous tool protocol -- would be a change to every tool in the
    // library for the benefit of this one.
    Q_INVOKABLE QString http_get(const QString &url);

Q_SIGNALS:
    void refused(const QString &url, const QString &reason);
    void fetched(const QString &url, int httpStatus, qint64 bytes);

private:
    QStringList m_allowedHosts;
    QNetworkAccessManager *m_manager = nullptr;
    qint64 m_maxBytes = 256 * 1024;
    int m_timeoutMs = 10000;
    bool m_requiresHttps = true;
};

} // namespace Tools
} // namespace QtOpenAi
