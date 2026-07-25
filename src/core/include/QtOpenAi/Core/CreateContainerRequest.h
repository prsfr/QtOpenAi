// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

class CreateContainerRequestData;

// The body of a POST /containers request.
class QTOPENAI_CORE_EXPORT CreateContainerRequest
{
public:
    CreateContainerRequest();
    explicit CreateContainerRequest(QString name, QStringList fileIds = {});
    CreateContainerRequest(const CreateContainerRequest &other);
    CreateContainerRequest(CreateContainerRequest &&other) noexcept;
    CreateContainerRequest &operator=(const CreateContainerRequest &other);
    CreateContainerRequest &operator=(CreateContainerRequest &&other) noexcept;
    ~CreateContainerRequest();

    void swap(CreateContainerRequest &other) noexcept { d.swap(other.d); }

    QString name() const;
    void setName(const QString &name);

    // Ids of already-uploaded files (Files API ids) to copy into the container's
    // filesystem when it starts.
    QStringList fileIds() const;
    void setFileIds(const QStringList &fileIds);

    // Optional idle-expiry policy (`expires_after`): an anchor — currently only
    // "last_active_at" — plus a lifetime in minutes. Omitted while minutes is 0.
    QString expiresAfterAnchor() const;
    int expiresAfterMinutes() const;
    void setExpiresAfter(const QString &anchor, int minutes);

    QJsonObject toJson() const;

    bool operator==(const CreateContainerRequest &other) const;
    bool operator!=(const CreateContainerRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateContainerRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateContainerRequest)
