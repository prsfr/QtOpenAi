// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>

#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Client {

class ClientErrorData;

// Describes a failed API interaction: a category, an HTTP status (when known),
// the provider's error `type`/`code`, and a human-readable message. An
// implicitly-shared value type; its data lives behind a d-pointer so the layout
// stays ABI-stable.
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

    ClientError();
    ClientError(Kind kind, QString message, int httpStatus = 0);
    ClientError(const ClientError &other);
    ClientError(ClientError &&other) noexcept;
    ClientError &operator=(const ClientError &other);
    ClientError &operator=(ClientError &&other) noexcept;
    ~ClientError();

    Kind kind() const;
    int httpStatus() const;
    QString message() const;

    QString type() const;
    void setType(const QString &type);

    QString code() const;
    void setCode(const QString &code);

    bool isError() const;
    explicit operator bool() const { return isError(); }

private:
    QSharedDataPointer<ClientErrorData> d;
};

} // namespace Client
} // namespace QtOpenAi

Q_DECLARE_METATYPE(QtOpenAi::Client::ClientError)
