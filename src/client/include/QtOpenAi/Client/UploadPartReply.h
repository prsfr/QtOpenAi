// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Upload.h>

namespace QtOpenAi {
namespace Client {

class UploadPartReplyPrivate;

// An upload-part request (POST /uploads/{id}/parts).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT UploadPartReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::UploadPart part() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::UploadPart &part);

private:
    friend class Client;
    UploadPartReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                    QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(UploadPartReply)
};

} // namespace Client
} // namespace QtOpenAi
