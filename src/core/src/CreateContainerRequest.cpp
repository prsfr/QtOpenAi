// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateContainerRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CreateContainerRequestData : public QSharedData
{
public:
    QString name;
    QStringList fileIds;
    QString expiresAfterAnchor;
    int expiresAfterMinutes = 0;
};

CreateContainerRequest::CreateContainerRequest()
    : d(new CreateContainerRequestData)
{ }

CreateContainerRequest::CreateContainerRequest(QString name, QStringList fileIds)
    : d(new CreateContainerRequestData)
{
    d->name = std::move(name);
    d->fileIds = std::move(fileIds);
}

CreateContainerRequest::CreateContainerRequest(const CreateContainerRequest &other) = default;
CreateContainerRequest::CreateContainerRequest(CreateContainerRequest &&other) noexcept = default;
CreateContainerRequest &CreateContainerRequest::operator=(const CreateContainerRequest &other)
        = default;
CreateContainerRequest &CreateContainerRequest::operator=(CreateContainerRequest &&other) noexcept
        = default;
CreateContainerRequest::~CreateContainerRequest() = default;

QString CreateContainerRequest::name() const { return d->name; }
void CreateContainerRequest::setName(const QString &name) { d->name = name; }

QStringList CreateContainerRequest::fileIds() const { return d->fileIds; }
void CreateContainerRequest::setFileIds(const QStringList &fileIds) { d->fileIds = fileIds; }

QString CreateContainerRequest::expiresAfterAnchor() const { return d->expiresAfterAnchor; }
int CreateContainerRequest::expiresAfterMinutes() const { return d->expiresAfterMinutes; }

void CreateContainerRequest::setExpiresAfter(const QString &anchor, int minutes)
{
    d->expiresAfterAnchor = anchor;
    d->expiresAfterMinutes = minutes;
}

QJsonObject CreateContainerRequest::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNotEmpty(json, QStringLiteral("file_ids"), d->fileIds);
    if (d->expiresAfterMinutes > 0) {
        QJsonObject expiresAfter;
        detail::insertIfNotEmpty(expiresAfter, QStringLiteral("anchor"), d->expiresAfterAnchor);
        expiresAfter.insert(QStringLiteral("minutes"), d->expiresAfterMinutes);
        json.insert(QStringLiteral("expires_after"), expiresAfter);
    }
    return json;
}

bool CreateContainerRequest::operator==(const CreateContainerRequest &other) const
{
    return d->name == other.d->name && d->fileIds == other.d->fileIds
           && d->expiresAfterAnchor == other.d->expiresAfterAnchor
           && d->expiresAfterMinutes == other.d->expiresAfterMinutes;
}

} // namespace Core
} // namespace QtOpenAi
