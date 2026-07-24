// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/ClientError.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Client {

class ClientErrorData : public QSharedData
{
public:
    ClientError::Kind kind = ClientError::Kind::NoError;
    int httpStatus = 0;
    QString message;
    QString type;
    QString code;
};

ClientError::ClientError()
    : d(new ClientErrorData)
{ }

ClientError::ClientError(Kind kind, QString message, int httpStatus)
    : d(new ClientErrorData)
{
    d->kind = kind;
    d->httpStatus = httpStatus;
    d->message = std::move(message);
}

ClientError::ClientError(const ClientError &other) = default;
ClientError::ClientError(ClientError &&other) noexcept = default;
ClientError &ClientError::operator=(const ClientError &other) = default;
ClientError &ClientError::operator=(ClientError &&other) noexcept = default;
ClientError::~ClientError() = default;

ClientError::Kind ClientError::kind() const { return d->kind; }

int ClientError::httpStatus() const { return d->httpStatus; }

QString ClientError::message() const { return d->message; }

QString ClientError::type() const { return d->type; }

void ClientError::setType(const QString &type) { d->type = type; }

QString ClientError::code() const { return d->code; }

void ClientError::setCode(const QString &code) { d->code = code; }

bool ClientError::isError() const { return d->kind != Kind::NoError; }

} // namespace Client
} // namespace QtOpenAi
