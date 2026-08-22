// SPDX-License-Identifier: MIT
#pragma once

// Internal helper for building multipart/form-data request bodies, shared by the
// endpoints that upload files (audio transcriptions/translations, and later
// image edits/variations, file uploads, ...). Not installed / not public API.
//
// Each call allocates a fresh QHttpMultiPart so the owning reply's request
// factory can rebuild the body on every retry attempt (a QHttpMultiPart is
// consumed once it has been posted).

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QHttpPart>

namespace QtOpenAi {
namespace Client {
namespace detail {

// One part carrying bytes and a content type.
//
// With a fileName it is an ordinary upload. **Leaving fileName empty makes it a
// typed field instead**: the part keeps its Content-Type but loses the
// `filename=` parameter, which is what distinguishes "here is a file" from
// "here is a value that happens not to be text". POST /realtime/calls needs
// both of those in one body -- an `sdp` part typed application/sdp and a
// `session` part typed application/json, neither of them a file -- and the
// scalar fields below cannot carry a content type at all.
struct FormFilePart
{
    QByteArray fieldName;
    QString fileName;
    QByteArray data;
    QByteArray contentType = "application/octet-stream";
};

// Build a multipart/form-data body from scalar text fields and file parts.
// Ownership of the returned object is the caller's; parent it to the reply so it
// is freed when the request completes.
inline QHttpMultiPart *buildMultipart(const QList<QPair<QString, QString>> &fields,
                                      const QList<FormFilePart> &files)
{
    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    for (const auto &field : fields) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QStringLiteral("form-data; name=\"%1\"").arg(field.first));
        part.setBody(field.second.toUtf8());
        multiPart->append(part);
    }

    for (const FormFilePart &file : files) {
        QHttpPart part;
        const QString disposition
                = file.fileName.isEmpty()
                          ? QStringLiteral("form-data; name=\"%1\"")
                                    .arg(QString::fromUtf8(file.fieldName))
                          : QStringLiteral("form-data; name=\"%1\"; filename=\"%2\"")
                                    .arg(QString::fromUtf8(file.fieldName), file.fileName);
        part.setHeader(QNetworkRequest::ContentDispositionHeader, disposition);
        part.setHeader(QNetworkRequest::ContentTypeHeader, file.contentType);
        part.setBody(file.data);
        multiPart->append(part);
    }

    return multiPart;
}

} // namespace detail
} // namespace Client
} // namespace QtOpenAi
