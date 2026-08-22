// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class VideoSourceRequestData;

// The body of POST /videos/edits and POST /videos/extensions: a source video
// plus a prompt (and, for an extension, how much to add).
//
// **One type for two endpoints, because the API sends one shape to both.** Edit
// and extend differ in what they do and not in what they take -- extend adds
// `seconds` and nothing else -- so two classes would have been the same fields
// twice with the same encoding rules and the same easy mistake available in
// both. Client::editVideo() and Client::extendVideo() are what tell them apart.
//
// **The source can be named or uploaded, and that choice picks the encoding.**
// Point at a video the API already has with setSourceVideoId() and the request
// goes out as JSON; hand over bytes with setSourceVideo() and it goes out as
// multipart/form-data. The two are exclusive: setting one clears the other,
// because a body carrying both is not something the endpoint accepts and
// silently sending the wrong half would produce a plausible render of the wrong
// video. Client picks the encoding from hasSourceUpload().
class QTOPENAI_CORE_EXPORT VideoSourceRequest
{
public:
    using FormField = QPair<QString, QString>;

    VideoSourceRequest();
    // The common case: edit or extend a video the API already holds.
    VideoSourceRequest(QString sourceVideoId, QString prompt);
    VideoSourceRequest(const VideoSourceRequest &other);
    VideoSourceRequest(VideoSourceRequest &&other) noexcept;
    VideoSourceRequest &operator=(const VideoSourceRequest &other);
    VideoSourceRequest &operator=(VideoSourceRequest &&other) noexcept;
    ~VideoSourceRequest();

    void swap(VideoSourceRequest &other) noexcept { d.swap(other.d); }

    // What to do to the source. Required by both endpoints.
    QString prompt() const;
    void setPrompt(const QString &prompt);

    // The id of a completed video to work from. Clears any uploaded source.
    QString sourceVideoId() const;
    void setSourceVideoId(const QString &videoId);

    // Bytes to work from instead of an id. Clears any source video id, and
    // switches the request to a multipart upload.
    QByteArray sourceVideoData() const;
    QString sourceVideoFileName() const;
    void setSourceVideo(const QString &fileName, const QByteArray &data);
    bool hasSourceUpload() const;

    // Extensions only: how long the *new segment* is, as a string ("4", "8",
    // "12"). The endpoint requires it; edits ignore it, so editVideo() does not
    // send it.
    //
    // The published schema enumerates "4", "8" and "12" while the prose beside
    // it says 4, 8, 12, 16 and 20. They cannot both be right, and guessing
    // wrongly in either direction would reject a request the API accepts or
    // send one it does not. The value is therefore passed through as the string
    // the API models rather than validated here.
    QString seconds() const;
    void setSeconds(const QString &seconds);

    // Extra provider-specific fields merged verbatim into the JSON body.
    QJsonObject extraBody() const;
    void setExtraBody(const QJsonObject &extra);

    // The non-file form fields, in a stable order, for multipart encoding.
    // `withSeconds` is false for an edit, which has no such parameter.
    QList<FormField> formFields(bool withSeconds) const;

    // The JSON body. `withSeconds` as above.
    QJsonObject toJson(bool withSeconds) const;

    bool operator==(const VideoSourceRequest &other) const;
    bool operator!=(const VideoSourceRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VideoSourceRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::VideoSourceRequest)
Q_DECLARE_METATYPE(QtOpenAi::Core::VideoSourceRequest)
