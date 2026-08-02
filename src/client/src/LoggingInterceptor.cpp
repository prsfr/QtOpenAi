// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/LoggingInterceptor.h"

#include <QtCore/QStringList>
#include <QtCore/QUrlQuery>

namespace QtOpenAi {
namespace Client {

// Default QtInfoMsg, so the debug lines this interceptor writes are off until
// someone asks for them. An HTTP log carries the user's prompts; making that
// opt-in is the same decision as logBodies() defaulting to false.
Q_LOGGING_CATEGORY(lcHttp, "qtopenai.http", QtInfoMsg)

namespace {

constexpr QLatin1String kRedacted("<redacted>");

// Query parameter names whose value is redacted. Matched as substrings and
// case-insensitively, because the spelling varies by provider ("api_key",
// "apiKey", "access_token", "X-Sig"). Over-redacting a log costs nothing;
// under-redacting one costs a credential.
bool isSecretParameter(const QString &name)
{
    static const QStringList needles
            = {QStringLiteral("key"),      QStringLiteral("token"), QStringLiteral("secret"),
               QStringLiteral("password"), QStringLiteral("sig"),   QStringLiteral("auth")};
    for (const QString &needle : needles) {
        if (name.contains(needle, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

// The URL as it is safe to write down: same URL, secrets in the query replaced.
QString safeUrl(const QUrl &url)
{
    QUrlQuery query(url);
    if (query.isEmpty())
        return url.toString(QUrl::FullyEncoded);

    QList<QPair<QString, QString>> items = query.queryItems();
    for (auto &item : items) {
        if (isSecretParameter(item.first))
            item.second = kRedacted;
    }
    QUrl safe = url;
    QUrlQuery redacted;
    redacted.setQueryItems(items);
    safe.setQuery(redacted);
    return safe.toString(QUrl::FullyEncoded);
}

QString excerpt(const QByteArray &body, int limit)
{
    if (body.isEmpty())
        return {};
    if (limit <= 0 || body.size() <= limit)
        return QString::fromUtf8(body);
    return QString::fromUtf8(body.left(limit))
           + QStringLiteral(" ... (%1 bytes total)").arg(body.size());
}

} // namespace

class LoggingInterceptorPrivate
{
public:
    bool logHeaders = true;
    bool logBodies = false;
    int maxBodyLength = 512;
    QList<QByteArray> redactedHeaders = LoggingInterceptor::defaultRedactedHeaders();
};

LoggingInterceptor::LoggingInterceptor(QObject *parent)
    : Interceptor(parent)
    , d_ptr(new LoggingInterceptorPrivate)
{ }

LoggingInterceptor::~LoggingInterceptor() = default;

QList<QByteArray> LoggingInterceptor::defaultRedactedHeaders()
{
    // Every header this library or a provider behind it can put a credential
    // in. `api-key` is Azure's, `x-api-key` and `x-goog-api-key` belong to
    // OpenAI-compatible providers, and `cookie` is here because a proxy in
    // front of any of them can turn a session into one.
    return {"authorization",       "api-key", "x-api-key",  "x-goog-api-key",
            "proxy-authorization", "cookie",  "set-cookie", "openai-organization",
            "openai-project"};
}

void LoggingInterceptor::setLogHeaders(bool enabled)
{
    Q_D(LoggingInterceptor);
    d->logHeaders = enabled;
}

bool LoggingInterceptor::logHeaders() const
{
    Q_D(const LoggingInterceptor);
    return d->logHeaders;
}

void LoggingInterceptor::setLogBodies(bool enabled)
{
    Q_D(LoggingInterceptor);
    d->logBodies = enabled;
}

bool LoggingInterceptor::logBodies() const
{
    Q_D(const LoggingInterceptor);
    return d->logBodies;
}

void LoggingInterceptor::setMaxBodyLength(int bytes)
{
    Q_D(LoggingInterceptor);
    d->maxBodyLength = bytes;
}

int LoggingInterceptor::maxBodyLength() const
{
    Q_D(const LoggingInterceptor);
    return d->maxBodyLength;
}

void LoggingInterceptor::setRedactedHeaders(const QList<QByteArray> &names)
{
    Q_D(LoggingInterceptor);
    d->redactedHeaders.clear();
    d->redactedHeaders.reserve(names.size());
    // Normalised once here so the per-header check is a plain comparison.
    for (const QByteArray &name : names)
        d->redactedHeaders.append(name.toLower());
}

QList<QByteArray> LoggingInterceptor::redactedHeaders() const
{
    Q_D(const LoggingInterceptor);
    return d->redactedHeaders;
}

std::optional<InterceptedResponse> LoggingInterceptor::beforeRequest(InterceptedRequest &request)
{
    Q_D(LoggingInterceptor);

    QStringList lines;
    lines << QStringLiteral("--> %1 %2")
                     .arg(QString::fromUtf8(request.method), safeUrl(request.url()));

    if (d->logHeaders) {
        const QList<QByteArray> names = request.request.rawHeaderList();
        for (const QByteArray &name : names) {
            const bool secret = d->redactedHeaders.contains(name.toLower());
            lines << QStringLiteral("    %1: %2")
                             .arg(QString::fromUtf8(name),
                                  secret ? QString(kRedacted)
                                         : QString::fromUtf8(request.request.rawHeader(name)));
        }
    }
    if (d->logBodies) {
        const QString body = excerpt(request.body, d->maxBodyLength);
        if (!body.isEmpty())
            lines << QStringLiteral("    %1").arg(body);
    }

    for (const QString &line : std::as_const(lines)) {
        qCDebug(lcHttp).noquote() << line;
        Q_EMIT logged(line);
    }

    // A logger observes; it never answers.
    return std::nullopt;
}

void LoggingInterceptor::afterResponse(const InterceptedResponse &response)
{
    Q_D(LoggingInterceptor);

    QStringList lines;
    // A transport failure has no status to report, so it reports what it has.
    const QString outcome = response.httpStatus > 0
                                    ? QString::number(response.httpStatus)
                                    : QStringLiteral("failed: %1").arg(response.error.message());
    lines << QStringLiteral("<-- %1 %2 %3 (%4 ms%5)")
                     .arg(outcome, QString::fromUtf8(response.request.method),
                          safeUrl(response.url()), QString::number(response.elapsedMs),
                          response.fromCache ? QStringLiteral(", cached") : QString());

    if (d->logBodies) {
        const QString body = excerpt(response.body, d->maxBodyLength);
        if (!body.isEmpty())
            lines << QStringLiteral("    %1").arg(body);
    }

    for (const QString &line : std::as_const(lines)) {
        qCDebug(lcHttp).noquote() << line;
        Q_EMIT logged(line);
    }
}

} // namespace Client
} // namespace QtOpenAi
