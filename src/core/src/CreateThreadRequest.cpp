// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateThreadRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- ThreadMessageInput ----------------------------------------------------

QJsonObject ThreadMessageInput::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("role"), roleToString(role));
    // Content is either a bare string or an array of parts; the parts win.
    if (!content.isEmpty())
        json.insert(QStringLiteral("content"), content);
    else
        json.insert(QStringLiteral("content"), text);
    if (!attachments.isEmpty())
        json.insert(QStringLiteral("attachments"), attachments);
    if (!metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), metadata);
    return json;
}

ThreadMessageInput ThreadMessageInput::fromJson(const QJsonObject &json)
{
    ThreadMessageInput message;
    message.role = roleFromString(detail::stringOr(json, QStringLiteral("role")));
    const QJsonValue content = json.value(QStringLiteral("content"));
    if (content.isArray())
        message.content = content.toArray();
    else
        message.text = content.toString();
    message.attachments = json.value(QStringLiteral("attachments")).toArray();
    message.metadata = json.value(QStringLiteral("metadata")).toObject();
    return message;
}

// --- CreateThreadRequest ---------------------------------------------------

class CreateThreadRequestData : public QSharedData
{
public:
    QList<ThreadMessageInput> messages;
    QJsonObject toolResources;
    QJsonObject metadata;
};

CreateThreadRequest::CreateThreadRequest()
    : d(new CreateThreadRequestData)
{ }

CreateThreadRequest::CreateThreadRequest(const CreateThreadRequest &other) = default;
CreateThreadRequest::CreateThreadRequest(CreateThreadRequest &&other) noexcept = default;
CreateThreadRequest &CreateThreadRequest::operator=(const CreateThreadRequest &other) = default;
CreateThreadRequest &CreateThreadRequest::operator=(CreateThreadRequest &&other) noexcept = default;
CreateThreadRequest::~CreateThreadRequest() = default;

QList<ThreadMessageInput> CreateThreadRequest::messages() const { return d->messages; }
void CreateThreadRequest::setMessages(const QList<ThreadMessageInput> &messages)
{
    d->messages = messages;
}

void CreateThreadRequest::addMessage(const ThreadMessageInput &message)
{
    d->messages.append(message);
}

void CreateThreadRequest::addUserMessage(const QString &text)
{
    ThreadMessageInput message;
    message.role = Role::User;
    message.text = text;
    d->messages.append(message);
}

QJsonObject CreateThreadRequest::toolResources() const { return d->toolResources; }
void CreateThreadRequest::setToolResources(const QJsonObject &toolResources)
{
    d->toolResources = toolResources;
}

QJsonObject CreateThreadRequest::metadata() const { return d->metadata; }
void CreateThreadRequest::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

bool CreateThreadRequest::isEmpty() const
{
    return d->messages.isEmpty() && d->toolResources.isEmpty() && d->metadata.isEmpty();
}

QJsonObject CreateThreadRequest::toJson() const
{
    QJsonObject json;
    if (!d->messages.isEmpty()) {
        QJsonArray messages;
        for (const ThreadMessageInput &message : d->messages)
            messages.append(message.toJson());
        json.insert(QStringLiteral("messages"), messages);
    }
    if (!d->toolResources.isEmpty())
        json.insert(QStringLiteral("tool_resources"), d->toolResources);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

bool CreateThreadRequest::operator==(const CreateThreadRequest &other) const
{
    return d->messages == other.d->messages && d->toolResources == other.d->toolResources
           && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
