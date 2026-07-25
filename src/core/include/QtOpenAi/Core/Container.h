// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ContainerData;

// A sandboxed container (Containers API) — the execution environment behind the
// code-interpreter tool, with its own small filesystem.
//
// Containers are short-lived: `expires_after` sets how long after the anchor
// event an idle container is reclaimed. The deletion acknowledgement of
// DELETE /containers/{id} shares this shape, with object() "container.deleted".
class QTOPENAI_CORE_EXPORT Container
{
public:
    Container();
    Container(const Container &other);
    Container(Container &&other) noexcept;
    Container &operator=(const Container &other);
    Container &operator=(Container &&other) noexcept;
    ~Container();

    void swap(Container &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "container" (or "container.deleted").
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString name() const;
    void setName(const QString &name);

    // Lifecycle state, e.g. "running" or "expired". Kept as a string so
    // provider-specific values survive a round-trip.
    QString status() const;
    void setStatus(const QString &status);

    // Optional idle-expiry policy (`expires_after`): an anchor — currently only
    // "last_active_at" — plus a lifetime in minutes. Minutes is 0 when unset.
    QString expiresAfterAnchor() const;
    int expiresAfterMinutes() const;
    void setExpiresAfter(const QString &anchor, int minutes);

    QJsonObject toJson() const;
    static Container fromJson(const QJsonObject &json);

    bool operator==(const Container &other) const;
    bool operator!=(const Container &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ContainerData> d;
};

// A `list` of containers (GET /containers). Cursor-paginated; reuses the shared
// list-page type.
using ContainerList = ListPage<Container>;

class ContainerFileData;

// One file inside a container's filesystem. It is either uploaded directly or
// copied in from the Files API, which `source` records.
class QTOPENAI_CORE_EXPORT ContainerFile
{
public:
    ContainerFile();
    ContainerFile(const ContainerFile &other);
    ContainerFile(ContainerFile &&other) noexcept;
    ContainerFile &operator=(const ContainerFile &other);
    ContainerFile &operator=(ContainerFile &&other) noexcept;
    ~ContainerFile();

    void swap(ContainerFile &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "container.file" (or "container.file.deleted").
    QString object() const;
    void setObject(const QString &object);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // File size in bytes; 0 when absent.
    qint64 bytes() const;
    void setBytes(qint64 bytes);

    QString containerId() const;
    void setContainerId(const QString &containerId);

    // Absolute path inside the container, e.g. "/mnt/data/report.csv".
    QString path() const;
    void setPath(const QString &path);

    // Where the file came from, e.g. "user" or "assistant".
    QString source() const;
    void setSource(const QString &source);

    QJsonObject toJson() const;
    static ContainerFile fromJson(const QJsonObject &json);

    bool operator==(const ContainerFile &other) const;
    bool operator!=(const ContainerFile &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ContainerFileData> d;
};

// A `list` of container files (GET /containers/{id}/files).
using ContainerFileList = ListPage<ContainerFile>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Container)
Q_DECLARE_SHARED(QtOpenAi::Core::ContainerFile)
Q_DECLARE_METATYPE(QtOpenAi::Core::Container)
Q_DECLARE_METATYPE(QtOpenAi::Core::ContainerList)
Q_DECLARE_METATYPE(QtOpenAi::Core::ContainerFile)
Q_DECLARE_METATYPE(QtOpenAi::Core::ContainerFileList)
