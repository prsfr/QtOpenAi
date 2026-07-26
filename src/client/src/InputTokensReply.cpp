// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/InputTokensReply.h"

#include "RestReplyBase_p.h"

#include <QtCore/QJsonValue>
#include <QtCore/QVariant>

namespace QtOpenAi {
namespace Client {

class InputTokensReplyPrivate : public RestReplyBasePrivate
{
public:
    qint64 inputTokens = 0;
    QJsonObject object;
};

InputTokensReply::InputTokensReply(std::function<QNetworkReply *()> requestFactory,
                                   RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new InputTokensReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

qint64 InputTokensReply::inputTokens() const
{
    Q_D(const InputTokensReply);
    return d->inputTokens;
}

QJsonObject InputTokensReply::object() const
{
    Q_D(const InputTokensReply);
    return d->object;
}

bool InputTokensReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(InputTokensReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->object = object;
    const QJsonValue value = object.value(QStringLiteral("input_tokens"));
    d->inputTokens = value.isDouble() ? value.toVariant().toLongLong() : 0;
    Q_EMIT finished(d->inputTokens);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
