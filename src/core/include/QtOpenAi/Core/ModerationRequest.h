// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ModerationRequestData;

// The body of a POST /moderations request. `input` may be a plain string, an
// array of strings, or an array of multimodal content parts (via the QJsonValue
// overload). `model` is optional.
class QTOPENAI_CORE_EXPORT ModerationRequest
{
public:
    ModerationRequest();
    explicit ModerationRequest(QString input);
    ModerationRequest(const ModerationRequest &other);
    ModerationRequest(ModerationRequest &&other) noexcept;
    ModerationRequest &operator=(const ModerationRequest &other);
    ModerationRequest &operator=(ModerationRequest &&other) noexcept;
    ~ModerationRequest();

    void swap(ModerationRequest &other) noexcept { d.swap(other.d); }

    QJsonValue input() const;
    void setInput(const QJsonValue &input);
    void setInput(const QString &input);

    QString model() const;
    void setModel(const QString &model);

    QJsonObject toJson() const;
    static ModerationRequest fromJson(const QJsonObject &json);

    bool operator==(const ModerationRequest &other) const;
    bool operator!=(const ModerationRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ModerationRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ModerationRequest)
