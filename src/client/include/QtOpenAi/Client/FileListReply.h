// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>
#include <QtOpenAi/Core/FileObject.h>

namespace QtOpenAi {
namespace Client {

class FileListReplyPrivate;

// A files-list request (GET /files).
// See RestReplyBase for the shared lifecycle (finished/failed/done, auto-delete).
class QTOPENAI_CLIENT_EXPORT FileListReply : public RestReplyBase
{
    Q_OBJECT
public:
    Core::FileList list() const;

Q_SIGNALS:
    void finished(const QtOpenAi::Core::FileList &list);

private:
    friend class Client;
    FileListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                  QObject *parent = nullptr);

    bool dispatchSuccess(const QByteArray &body, int httpStatus) override;

    Q_DECLARE_PRIVATE(FileListReply)
};

} // namespace Client
} // namespace QtOpenAi
