// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/LoggingInterceptor.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaMethod>
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

// Replace every secret-named field, at any depth, and report whether anything
// was. Walks rather than string-matches so it is the *key* that decides: a
// prompt that happens to mention "value" is still readable, and a field called
// `value` nested three objects down is still redacted.
//
// Only what changes is written back, and that is the whole point. Qt 6's JSON
// containers are CBOR-backed, so QJsonObject::iterator::operator= and
// QJsonArray::replace() are not cheap slot assignments; assigning every element
// -- including the scalars a walk leaves alone -- made this quadratic in the
// container's size. A 1 MiB body cost 244 ms, on the thread that is trying to
// send a request. Returning a bool rather than a value is what lets an untouched
// subtree stay untouched, which is the overwhelmingly common case: most bodies
// carry no secret at all.
bool redactObject(QJsonObject &object, const QStringList &fields);
bool redactArray(QJsonArray &array, const QStringList &fields);

bool redactObject(QJsonObject &object, const QStringList &fields)
{
    bool changed = false;
    for (auto it = object.begin(); it != object.end(); ++it) {
        // Compared case-insensitively, as the header list is: a provider that
        // spells it `Value` is carrying the same secret.
        if (fields.contains(it.key(), Qt::CaseInsensitive)) {
            it.value() = QString(kRedacted);
            changed = true;
            continue;
        }
        const QJsonValue value = it.value();
        if (value.isObject()) {
            QJsonObject nested = value.toObject();
            if (redactObject(nested, fields)) {
                it.value() = nested;
                changed = true;
            }
        } else if (value.isArray()) {
            QJsonArray nested = value.toArray();
            if (redactArray(nested, fields)) {
                it.value() = nested;
                changed = true;
            }
        }
    }
    return changed;
}

bool redactArray(QJsonArray &array, const QStringList &fields)
{
    bool changed = false;
    for (qsizetype i = 0; i < array.size(); ++i) {
        const QJsonValue element = array.at(i);
        if (element.isObject()) {
            QJsonObject nested = element.toObject();
            if (redactObject(nested, fields)) {
                array.replace(i, nested);
                changed = true;
            }
        } else if (element.isArray()) {
            QJsonArray nested = element.toArray();
            if (redactArray(nested, fields)) {
                array.replace(i, nested);
                changed = true;
            }
        }
    }
    return changed;
}

// The body as it is safe to write down. A body that is not JSON -- a stream, a
// multipart upload -- is returned unchanged, because there is no field
// structure to find a secret in and truncation already bounds it.
QByteArray safeBody(const QByteArray &body, const QStringList &fields)
{
    if (fields.isEmpty() || body.isEmpty())
        return body;

    // Parsed before truncation: excerpting first would leave a half-object that
    // no longer parses, and the secret in it unredacted.
    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (document.isObject()) {
        QJsonObject object = document.object();
        redactObject(object, fields);
        return Core::detail::compactJson(object);
    }
    if (document.isArray()) {
        QJsonArray array = document.array();
        redactArray(array, fields);
        return Core::detail::compactJson(array);
    }
    return body;
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
    QStringList redactedBodyFields = LoggingInterceptor::defaultRedactedBodyFields();
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

QStringList LoggingInterceptor::defaultRedactedBodyFields()
{
    // `value` is the one that matters and the one that reads like a false
    // positive: it is how the API hands back a newly created admin key, a
    // realtime client secret and an injected container secret. It is also rare
    // -- eight of the API's ~1400 schemas have a field by that name -- so
    // redacting it costs a debugger almost nothing and saves a live credential.
    // The rest are the names a provider or a proxy in front of one uses for the
    // same thing.
    return {QStringLiteral("value"),         QStringLiteral("secret"),
            QStringLiteral("client_secret"), QStringLiteral("api_key"),
            QStringLiteral("apikey"),        QStringLiteral("access_token"),
            QStringLiteral("refresh_token"), QStringLiteral("password"),
            QStringLiteral("authorization")};
}

void LoggingInterceptor::setRedactedBodyFields(const QStringList &names)
{
    Q_D(LoggingInterceptor);
    d->redactedBodyFields = names;
}

QStringList LoggingInterceptor::redactedBodyFields() const
{
    Q_D(const LoggingInterceptor);
    return d->redactedBodyFields;
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

    // Nothing reads what follows unless the category is enabled or someone is
    // connected to logged(), and building it anyway was the steady state: the
    // category defaults to QtInfoMsg and the header's own workflow is "install
    // it, switch it on with QT_LOGGING_RULES". So a body was redacted, excerpted
    // and formatted on the thread trying to send a request, then discarded. The
    // signal has to be checked too -- connecting to logged() without touching
    // the category is a legitimate way to use this.
    if (!lcHttp().isDebugEnabled()
        && !isSignalConnected(QMetaMethod::fromSignal(&LoggingInterceptor::logged)))
        return std::nullopt;

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
        const QString body
                = excerpt(safeBody(request.body, d->redactedBodyFields), d->maxBodyLength);
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

    // Nothing reads what follows unless the category is enabled or someone is
    // connected to logged(), and building it anyway was the steady state: the
    // category defaults to QtInfoMsg and the header's own workflow is "install
    // it, switch it on with QT_LOGGING_RULES". So a body was redacted, excerpted
    // and formatted on the thread trying to send a request, then discarded. The
    // signal has to be checked too -- connecting to logged() without touching
    // the category is a legitimate way to use this.
    if (!lcHttp().isDebugEnabled()
        && !isSignalConnected(QMetaMethod::fromSignal(&LoggingInterceptor::logged)))
        return;

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
        const QString body
                = excerpt(safeBody(response.body, d->redactedBodyFields), d->maxBodyLength);
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
