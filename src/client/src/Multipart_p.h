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

// One binary file part (a named upload with a filename and content type).
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
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QStringLiteral("form-data; name=\"%1\"; filename=\"%2\"")
                               .arg(QString::fromUtf8(file.fieldName), file.fileName));
        part.setHeader(QNetworkRequest::ContentTypeHeader, file.contentType);
        part.setBody(file.data);
        multiPart->append(part);
    }

    return multiPart;
}

} // namespace detail
} // namespace Client
} // namespace QtOpenAi
