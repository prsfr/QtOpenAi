// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class EvalData;

// An eval definition (POST /evals, GET /evals/{id}, ...): the shape of the test
// data plus the graders to score model output against.
//
// `data_source_config` and `testing_criteria` are large open unions in the
// OpenAPI spec — a dozen config kinds and a growing set of grader types — so
// they are carried as raw JSON rather than half-modelled here. Everything the
// client actually needs to act on (ids, name, timestamps) is typed.
//
// The deletion acknowledgement of DELETE /evals/{id} also decodes into this
// type; it names the id `eval_id`, which fromJson() accepts as an alternative
// spelling of `id`.
class QTOPENAI_CORE_EXPORT Eval
{
public:
    Eval();
    Eval(const Eval &other);
    Eval(Eval &&other) noexcept;
    Eval &operator=(const Eval &other);
    Eval &operator=(Eval &&other) noexcept;
    ~Eval();

    void swap(Eval &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "eval" (or "eval.deleted").
    QString object() const;
    void setObject(const QString &object);

    QString name() const;
    void setName(const QString &name);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // How the eval's items are shaped (`data_source_config`), verbatim.
    QJsonObject dataSourceConfig() const;
    void setDataSourceConfig(const QJsonObject &dataSourceConfig);

    // The graders scoring each item (`testing_criteria`), verbatim.
    QJsonArray testingCriteria() const;
    void setTestingCriteria(const QJsonArray &testingCriteria);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QJsonObject toJson() const;
    static Eval fromJson(const QJsonObject &json);

    bool operator==(const Eval &other) const;
    bool operator!=(const Eval &other) const { return !(*this == other); }

private:
    QSharedDataPointer<EvalData> d;
};

// A `list` of evals (GET /evals). Cursor-paginated; reuses the shared list-page
// type.
using EvalList = ListPage<Eval>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Eval)
Q_DECLARE_METATYPE(QtOpenAi::Core::Eval)
Q_DECLARE_METATYPE(QtOpenAi::Core::EvalList)
