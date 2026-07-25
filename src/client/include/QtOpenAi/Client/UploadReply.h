// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/Upload.h>

namespace QtOpenAi {
namespace Client {

class UploadReplyPrivate;

// An asynchronous handle for a single-upload request (POST /uploads and the
// /complete and /cancel actions). All three answer with the upload shape, so
// this reply serves them all. See RestReplyBase for the shared lifecycle
// (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT UploadReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::Upload upload() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Upload &upload);

private:
    friend class Client;
    UploadReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(UploadReply)
};

} // namespace Client
} // namespace QtOpenAi
