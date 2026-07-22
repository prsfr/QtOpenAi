// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>

#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Client {

// Describes a failed API interaction: a category, an HTTP status (when known),
// the provider's error `type`/`code`, and a human-readable message.
class QTOPENAI_CLIENT_EXPORT ClientError
{
    Q_GADGET
    Q_PROPERTY(Kind kind READ kind)
    Q_PROPERTY(int httpStatus READ httpStatus)
    Q_PROPERTY(QString message READ message)
public:
    enum class Kind {
        NoError,
        Network,        // transport-level failure (timeout, DNS, TLS, ...)
        Http,           // non-2xx response with a parsable error body
        Parse,          // response body was not valid/expected JSON
        InvalidRequest, // request rejected locally before sending
    };
    Q_ENUM(Kind)

    ClientError() = default;
    ClientError(Kind kind, QString message, int httpStatus = 0)
        : m_kind(kind), m_httpStatus(httpStatus), m_message(std::move(message))
    {
    }

    Kind kind() const { return m_kind; }
    int httpStatus() const { return m_httpStatus; }
    QString message() const { return m_message; }

    QString type() const { return m_type; }
    void setType(const QString &type) { m_type = type; }

    QString code() const { return m_code; }
    void setCode(const QString &code) { m_code = code; }

    bool isError() const { return m_kind != Kind::NoError; }
    explicit operator bool() const { return isError(); }

private:
    Kind m_kind = Kind::NoError;
    int m_httpStatus = 0;
    QString m_message;
    QString m_type;
    QString m_code;
};

} // namespace Client
} // namespace QtOpenAi

Q_DECLARE_METATYPE(QtOpenAi::Client::ClientError)
