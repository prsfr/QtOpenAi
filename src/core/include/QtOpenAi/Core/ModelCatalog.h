// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ModelInfo.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

class ModelCatalogData;

// What the library knows about the models it can be pointed at: context window,
// output limit, tokenizer encoding, capabilities and price.
//
//     const ModelInfo info = ModelCatalog::shared().model("gpt-4o-mini");
//     info.contextWindow();                        // 128000
//     info.supports(ModelCapability::Vision);      // true
//
// Two things follow from a bundled table of facts that live outside this
// library:
//
//  * **It ages.** The defaults are a snapshot -- prices change, models appear.
//    That is why `merge()` exists and why it is not an afterthought: point it
//    at a JSON file and the catalog is current again without a release.
//  * **It cannot be complete.** `model()` never fails. An unknown id first
//    tries the longest known id that is a prefix of it, which is what turns
//    "gpt-4o-mini-2024-07-18" into the entry for "gpt-4o-mini"; failing that it
//    returns a conservative fallback whose `isKnown()` is false, so a caller
//    who cares can tell a fact from a guess.
class QTOPENAI_CORE_EXPORT ModelCatalog
{
public:
    ModelCatalog(); // empty
    ModelCatalog(const ModelCatalog &other);
    ModelCatalog(ModelCatalog &&other) noexcept;
    ModelCatalog &operator=(const ModelCatalog &other);
    ModelCatalog &operator=(ModelCatalog &&other) noexcept;
    ~ModelCatalog();

    void swap(ModelCatalog &other) noexcept { d.swap(other.d); }

    // The bundled table, as a value -- a fresh copy each call.
    static ModelCatalog defaults();

    // The catalog the library consults, initialised from defaults(). Mutable on
    // purpose: this is where an application installs its own table.
    static ModelCatalog &shared();

    // Never fails; see the class comment for what an unknown id yields.
    ModelInfo model(const QString &id) const;

    // Exactly this id, with no prefix match and no fallback.
    bool contains(const QString &id) const;
    ModelInfo entry(const QString &id) const;

    QStringList ids() const;
    int count() const;
    bool isEmpty() const;

    void insert(const ModelInfo &info);
    bool remove(const QString &id);
    void clear();

    // The entry returned for an id nothing matches. Setting it is how an
    // application chooses what "unknown model" should mean for it.
    ModelInfo fallback() const;
    void setFallback(const ModelInfo &fallback);

    // Add or replace entries from a JSON table of { "<id>": { ... } }.
    // Existing entries not mentioned are left alone, so a file can carry only
    // what changed.
    void merge(const QJsonObject &json);
    QJsonObject toJson() const;

    bool operator==(const ModelCatalog &other) const;
    bool operator!=(const ModelCatalog &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ModelCatalogData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ModelCatalog)
