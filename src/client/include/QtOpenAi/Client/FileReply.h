// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/FileObject.h>

namespace QtOpenAi {
namespace Client {

class FileReplyPrivate;

// An asynchronous handle for a single-file request (POST /files, GET
// /files/{id}, DELETE /files/{id}). All three answer with the file shape, so
// this reply serves them all. See RestReplyBase for the shared lifecycle
// (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT FileReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::FileObject file() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FileObject &file);

private:
    friend class Client;
    FileReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
              QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(FileReply)
};

} // namespace Client
} // namespace QtOpenAi
