// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/Interceptor.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Client {

// Everything this library logs goes through here. It is off by default -- an
// HTTP log carries the user's prompts -- and turned on without a rebuild by
//
//     QT_LOGGING_RULES="qtopenai.http.debug=true"
QTOPENAI_CLIENT_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcHttp)

class LoggingInterceptorPrivate;

// Writes one line per request and one per response to the `qtopenai.http`
// logging category, with the credentials taken out.
//
//     LoggingInterceptor logger;
//     client.addInterceptor(&logger);
//
//     --> POST https://api.openai.com/v1/chat/completions
//         authorization: <redacted>
//     <-- 200 https://api.openai.com/v1/chat/completions (412 ms)
//
// The redaction is the point. An API key is a bearer credential: once it is in
// a log file it is in every backup, every bug report and every pasted terminal
// buffer that log file ever reaches. So the header values that can carry one
// are replaced with `<redacted>` before anything is written, as are query
// parameters whose name looks like a secret -- and the *default* is to redact,
// so forgetting to configure it is the safe outcome rather than the leak.
//
// Bodies are off by default for a related reason: they hold the user's prompts,
// which is data the user did not agree to have written to disk. Turning them on
// truncates to maxBodyLength() so one large upload cannot fill a disk -- and
// redacts the JSON fields that carry a credential, because a handful of
// responses hand back a live secret exactly once and a log is the last place it
// should survive. See redactedBodyFields().
class QTOPENAI_CLIENT_EXPORT LoggingInterceptor : public Interceptor
{
    Q_OBJECT
    Q_PROPERTY(bool logHeaders READ logHeaders WRITE setLogHeaders)
    Q_PROPERTY(bool logBodies READ logBodies WRITE setLogBodies)
    Q_PROPERTY(int maxBodyLength READ maxBodyLength WRITE setMaxBodyLength)
public:
    explicit LoggingInterceptor(QObject *parent = nullptr);
    ~LoggingInterceptor() override;

    // Log each request's headers, redacted. On by default: which headers went
    // out is most of what one debugs, and none of it is secret once redacted.
    void setLogHeaders(bool enabled);
    bool logHeaders() const;

    // Log the request and response bodies. Off by default -- they contain the
    // prompts.
    void setLogBodies(bool enabled);
    bool logBodies() const;

    // Longest body excerpt written, in bytes. 0 means no limit. Default 512.
    void setMaxBodyLength(int bytes);
    int maxBodyLength() const;

    // Header names whose value is replaced with `<redacted>`, compared
    // case-insensitively. Defaults to every header that is known to carry a
    // credential; setting this replaces the list rather than adding to it, so
    // pass the defaults along if you mean to extend them.
    void setRedactedHeaders(const QList<QByteArray> &names);
    QList<QByteArray> redactedHeaders() const;
    static QList<QByteArray> defaultRedactedHeaders();

    // JSON field names whose value is replaced with `<redacted>` when a body is
    // logged, matched on the key case-insensitively and at any depth. Header
    // redaction does not cover this: a created key's secret arrives in the
    // *response body*, which was written out verbatim.
    //
    // Applies to request and response bodies alike -- a secret goes up as well
    // as down -- and only to bodies that parse as JSON; anything else (a stream,
    // an upload) is logged as before. Setting this replaces the list rather than
    // adding to it, so pass the defaults along if you mean to extend them.
    void setRedactedBodyFields(const QStringList &names);
    QStringList redactedBodyFields() const;
    static QStringList defaultRedactedBodyFields();

    std::optional<InterceptedResponse> beforeRequest(InterceptedRequest &request) override;
    void afterResponse(const InterceptedResponse &response) override;

Q_SIGNALS:
    // Every line written, verbatim. Lets a caller route the log somewhere of
    // their own -- and a test assert on it -- without hijacking the global
    // message handler.
    void logged(const QString &line);

private:
    Q_DECLARE_PRIVATE(LoggingInterceptor)
    QScopedPointer<LoggingInterceptorPrivate> d_ptr;
};

} // namespace Client
} // namespace QtOpenAi
