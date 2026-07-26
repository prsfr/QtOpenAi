// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>

#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

class InputTokensReplyPrivate;

// An asynchronous handle for POST /responses/input_tokens, which prices a
// request's input without running it.
//
// The endpoint answers with a small counting object rather than one of the
// modelled Core types, so this reply exposes the count directly plus the whole
// payload — fields this library does not name stay reachable through object().
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT InputTokensReply : public RestReplyBase
{
    Q_OBJECT
public:
    // Tokens the request's input would consume; 0 when the field is absent.
    qint64 inputTokens() const;

    // The decoded response body, verbatim.
    QJsonObject object() const;

Q_SIGNALS:
    void finished(qint64 inputTokens);

private:
    friend class Client;
    InputTokensReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                     QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(InputTokensReply)
};

} // namespace Client
} // namespace QtOpenAi
