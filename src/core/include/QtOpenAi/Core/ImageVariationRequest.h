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

class ImageVariationRequestData;

// The body of a POST /images/variations request (dall-e-2). A multipart upload:
// one source image plus scalar form fields exposed through formFields(). The
// image bytes are carried out-of-band via imageData().
class QTOPENAI_CORE_EXPORT ImageVariationRequest
{
public:
    using FormField = QPair<QString, QString>;

    ImageVariationRequest();
    ImageVariationRequest(QByteArray imageData, QString fileName, QString model = {});
    ImageVariationRequest(const ImageVariationRequest &other);
    ImageVariationRequest(ImageVariationRequest &&other) noexcept;
    ImageVariationRequest &operator=(const ImageVariationRequest &other);
    ImageVariationRequest &operator=(ImageVariationRequest &&other) noexcept;
    ~ImageVariationRequest();

    void swap(ImageVariationRequest &other) noexcept { d.swap(other.d); }

    QByteArray imageData() const;
    void setImageData(const QByteArray &imageData);

    QString fileName() const;
    void setFileName(const QString &fileName);

    QString model() const;
    void setModel(const QString &model);

    std::optional<int> n() const;
    void setN(int n);

    QString size() const;
    void setSize(const QString &size);

    QString responseFormat() const;
    void setResponseFormat(const QString &format);

    QString user() const;
    void setUser(const QString &user);

    QList<FormField> formFields() const;

    bool operator==(const ImageVariationRequest &other) const;
    bool operator!=(const ImageVariationRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ImageVariationRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ImageVariationRequest)
