// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class FileUploadRequestData;

// The body of a POST /files request. Like the audio and image uploads this is a
// multipart/form-data call: the bytes travel as the `file` part (carried here as
// raw data plus a filename) and everything else is exposed through formFields()
// as ordered name/value pairs the Client turns into multipart parts.
class QTOPENAI_CORE_EXPORT FileUploadRequest
{
public:
    using FormField = QPair<QString, QString>;

    FileUploadRequest();
    FileUploadRequest(QByteArray fileData, QString fileName, QString purpose);
    FileUploadRequest(const FileUploadRequest &other);
    FileUploadRequest(FileUploadRequest &&other) noexcept;
    FileUploadRequest &operator=(const FileUploadRequest &other);
    FileUploadRequest &operator=(FileUploadRequest &&other) noexcept;
    ~FileUploadRequest();

    void swap(FileUploadRequest &other) noexcept { d.swap(other.d); }

    // The bytes to upload (the multipart `file` part).
    QByteArray fileData() const;
    void setFileData(const QByteArray &fileData);

    // The upload filename; the API derives the format from its extension.
    QString fileName() const;
    void setFileName(const QString &fileName);

    // The intended use, e.g. "fine-tune", "assistants", "batch", "user_data",
    // "vision" or "evals".
    QString purpose() const;
    void setPurpose(const QString &purpose);

    // Optional expiry policy (`expires_after`): an anchor — currently only
    // "created_at" — plus a lifetime in seconds. Both are sent together.
    QString expiresAfterAnchor() const;
    std::optional<qint64> expiresAfterSeconds() const;
    void setExpiresAfter(const QString &anchor, qint64 seconds);

    // The non-file form fields, in a stable order, ready for multipart encoding.
    QList<FormField> formFields() const;

    bool operator==(const FileUploadRequest &other) const;
    bool operator!=(const FileUploadRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<FileUploadRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::FileUploadRequest)
