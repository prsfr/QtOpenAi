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

class ModelData;

// A model description (GET /models, GET /models/{id}).
class QTOPENAI_CORE_EXPORT Model
{
public:
    Model();
    Model(const Model &other);
    Model(Model &&other) noexcept;
    Model &operator=(const Model &other);
    Model &operator=(Model &&other) noexcept;
    ~Model();

    void swap(Model &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    QString object() const;
    void setObject(const QString &object);

    qint64 created() const;
    void setCreated(qint64 created);

    QString ownedBy() const;
    void setOwnedBy(const QString &ownedBy);

    QJsonObject toJson() const;
    static Model fromJson(const QJsonObject &json);

    bool operator==(const Model &other) const;
    bool operator!=(const Model &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ModelData> d;
};

// A `list` of models (GET /models). Not cursor-paginated, but reuses the shared
// list-page type.
using ModelList = ListPage<Model>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Model)
Q_DECLARE_METATYPE(QtOpenAi::Core::ModelList)
