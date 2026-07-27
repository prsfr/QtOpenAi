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

class CreateSkillRequestData;

// The body of a POST /skills request — and of POST /skills/{id}/versions, which
// publishes a new version from the very same bundle. Like the audio and image
// uploads this is a multipart/form-data call: the bundle travels as the `files`
// parts (carried here as filename/bytes pairs) and everything else is exposed
// through formFields() as ordered name/value pairs the Client turns into
// multipart parts.
//
// A bundle is uploaded either way the API accepts: a single zip, or a directory
// as one part per file. For the directory form the file name is the path
// relative to the skill root ("scripts/build.py"), which is how the server
// rebuilds the layout.
class QTOPENAI_CORE_EXPORT CreateSkillRequest
{
public:
    using FormField = QPair<QString, QString>;
    // A named bundle file: (fileName, bytes).
    using SkillFile = QPair<QString, QByteArray>;

    CreateSkillRequest();
    // The single-file form, typically a zipped bundle.
    CreateSkillRequest(QString fileName, QByteArray data);
    CreateSkillRequest(const CreateSkillRequest &other);
    CreateSkillRequest(CreateSkillRequest &&other) noexcept;
    CreateSkillRequest &operator=(const CreateSkillRequest &other);
    CreateSkillRequest &operator=(CreateSkillRequest &&other) noexcept;
    ~CreateSkillRequest();

    void swap(CreateSkillRequest &other) noexcept { d.swap(other.d); }

    // The bundle files, in upload order (the multipart `files` parts).
    QList<SkillFile> files() const;
    void setFiles(const QList<SkillFile> &files);
    void addFile(const QString &fileName, const QByteArray &data);

    // Whether the published version becomes the skill's default (`default`).
    // Named makeDefault() because `default` is a keyword; unset leaves the
    // field out, which is what POST /skills — which has no such field — needs.
    std::optional<bool> makeDefault() const;
    void setMakeDefault(bool makeDefault);

    // The non-file form fields, in a stable order, ready for multipart encoding.
    QList<FormField> formFields() const;

    bool operator==(const CreateSkillRequest &other) const;
    bool operator!=(const CreateSkillRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateSkillRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateSkillRequest)
