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

class ImageEditRequestData;

// The body of a POST /images/edits request. Like the audio uploads this is a
// multipart/form-data request: one or more source images (and an optional mask)
// plus scalar form fields exposed through formFields(). The image/mask bytes are
// carried out-of-band via images()/maskData().
class QTOPENAI_CORE_EXPORT ImageEditRequest
{
public:
    using FormField = QPair<QString, QString>;
    // A named image blob: (fileName, bytes).
    using ImageFile = QPair<QString, QByteArray>;

    ImageEditRequest();
    ImageEditRequest(QByteArray imageData, QString fileName, QString prompt, QString model = {});
    ImageEditRequest(const ImageEditRequest &other);
    ImageEditRequest(ImageEditRequest &&other) noexcept;
    ImageEditRequest &operator=(const ImageEditRequest &other);
    ImageEditRequest &operator=(ImageEditRequest &&other) noexcept;
    ~ImageEditRequest();

    void swap(ImageEditRequest &other) noexcept { d.swap(other.d); }

    // The source image(s) to edit. A single image is uploaded as the `image`
    // part; multiple images use the `image[]` convention (gpt-image-1).
    QList<ImageFile> images() const;
    void setImages(const QList<ImageFile> &images);
    void addImage(const QString &fileName, const QByteArray &data);

    // Optional mask (transparent areas mark where to edit).
    QByteArray maskData() const;
    QString maskFileName() const;
    void setMask(const QString &fileName, const QByteArray &data);
    bool hasMask() const;

    QString prompt() const;
    void setPrompt(const QString &prompt);

    QString model() const;
    void setModel(const QString &model);

    std::optional<int> n() const;
    void setN(int n);

    QString size() const;
    void setSize(const QString &size);

    QString responseFormat() const;
    void setResponseFormat(const QString &format);

    QString background() const;
    void setBackground(const QString &background);

    QString outputFormat() const;
    void setOutputFormat(const QString &outputFormat);

    QString user() const;
    void setUser(const QString &user);

    // The non-file form fields, in a stable order, ready for multipart encoding.
    QList<FormField> formFields() const;

    bool operator==(const ImageEditRequest &other) const;
    bool operator!=(const ImageEditRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ImageEditRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ImageEditRequest)
