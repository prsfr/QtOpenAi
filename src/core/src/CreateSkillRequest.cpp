// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateSkillRequest.h"

#include <QtCore/QSharedData>

#include <utility>

namespace QtOpenAi {
namespace Core {

class CreateSkillRequestData : public QSharedData
{
public:
    QList<CreateSkillRequest::SkillFile> files;
    std::optional<bool> makeDefault;
};

CreateSkillRequest::CreateSkillRequest()
    : d(new CreateSkillRequestData)
{ }

CreateSkillRequest::CreateSkillRequest(QString fileName, QByteArray data)
    : d(new CreateSkillRequestData)
{
    d->files.append({std::move(fileName), std::move(data)});
}

CreateSkillRequest::CreateSkillRequest(const CreateSkillRequest &other) = default;
CreateSkillRequest::CreateSkillRequest(CreateSkillRequest &&other) noexcept = default;
CreateSkillRequest &CreateSkillRequest::operator=(const CreateSkillRequest &other) = default;
CreateSkillRequest &CreateSkillRequest::operator=(CreateSkillRequest &&other) noexcept = default;
CreateSkillRequest::~CreateSkillRequest() = default;

QList<CreateSkillRequest::SkillFile> CreateSkillRequest::files() const { return d->files; }
void CreateSkillRequest::setFiles(const QList<SkillFile> &files) { d->files = files; }

void CreateSkillRequest::addFile(const QString &fileName, const QByteArray &data)
{
    d->files.append({fileName, data});
}

std::optional<bool> CreateSkillRequest::makeDefault() const { return d->makeDefault; }
void CreateSkillRequest::setMakeDefault(bool makeDefault) { d->makeDefault = makeDefault; }

QList<CreateSkillRequest::FormField> CreateSkillRequest::formFields() const
{
    QList<FormField> fields;
    if (d->makeDefault) {
        fields.append({QStringLiteral("default"),
                       *d->makeDefault ? QStringLiteral("true") : QStringLiteral("false")});
    }
    return fields;
}

bool CreateSkillRequest::operator==(const CreateSkillRequest &other) const
{
    return d->files == other.d->files && d->makeDefault == other.d->makeDefault;
}

} // namespace Core
} // namespace QtOpenAi
