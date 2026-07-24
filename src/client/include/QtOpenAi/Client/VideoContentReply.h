// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/BinaryReply.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a rendered-video download (GET /videos/{id}/content).
// The endpoint returns a binary video blob; this is a BinaryReply whose
// videoData() is a domain alias for data(). See BinaryReply / RestReplyBase for
// the raw-bytes handling and the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT VideoContentReply : public BinaryReply
{
    Q_OBJECT
public:
    // The raw video bytes returned by the server (alias for data()).
    QByteArray videoData() const;

private:
    friend class Client;
    VideoContentReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                      QObject *parent = nullptr);
};

} // namespace Client
} // namespace QtOpenAi
