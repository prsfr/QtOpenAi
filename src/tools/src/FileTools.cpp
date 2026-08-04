// SPDX-License-Identifier: MIT
#include "QtOpenAi/Tools/FileTools.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

namespace QtOpenAi {
namespace Tools {

FileTools::FileTools(QObject *parent)
    : QObject(parent)
{ }

FileTools::FileTools(const FileSandbox &sandbox, QObject *parent)
    : QObject(parent)
    , m_sandbox(sandbox)
{ }

FileTools::~FileTools() = default;

FileSandbox FileTools::sandbox() const { return m_sandbox; }
void FileTools::setSandbox(const FileSandbox &sandbox) { m_sandbox = sandbox; }

QStringList FileTools::readingTools()
{
    return {QStringLiteral("read_file"), QStringLiteral("list_directory"),
            QStringLiteral("file_exists")};
}

QStringList FileTools::writingTools() { return {QStringLiteral("write_file")}; }

QString FileTools::read_file(const QString &path)
{
    FileSandbox::Rejection reason = FileSandbox::Rejection::None;
    const QString resolved = m_sandbox.resolve(path, false, &reason);
    if (resolved.isEmpty()) {
        Q_EMIT refused(QStringLiteral("read_file"), path, FileSandbox::describe(reason));
        return FileSandbox::describe(reason);
    }

    QFileInfo info(resolved);
    if (!info.isFile()) {
        const QString why = FileSandbox::describe(FileSandbox::Rejection::Unreadable);
        Q_EMIT refused(QStringLiteral("read_file"), path, why);
        return why;
    }
    // Checked before opening: a tool result is pasted straight back into a
    // context window, so reading the file first and then refusing would have
    // already paid the cost the cap exists to avoid.
    if (!m_sandbox.allowsSize(info.size())) {
        const QString why = FileSandbox::describe(FileSandbox::Rejection::TooLarge);
        Q_EMIT refused(QStringLiteral("read_file"), path, why);
        return why;
    }

    QFile file(resolved);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString why = FileSandbox::describe(FileSandbox::Rejection::Unreadable);
        Q_EMIT refused(QStringLiteral("read_file"), path, why);
        return why;
    }

    Q_EMIT performed(QStringLiteral("read_file"), resolved);
    return QString::fromUtf8(file.readAll());
}

QString FileTools::write_file(const QString &path, const QString &content)
{
    FileSandbox::Rejection reason = FileSandbox::Rejection::None;
    const QString resolved = m_sandbox.resolve(path, true, &reason);
    if (resolved.isEmpty()) {
        Q_EMIT refused(QStringLiteral("write_file"), path, FileSandbox::describe(reason));
        return FileSandbox::describe(reason);
    }

    const QByteArray bytes = content.toUtf8();
    if (!m_sandbox.allowsSize(bytes.size())) {
        const QString why = FileSandbox::describe(FileSandbox::Rejection::TooLarge);
        Q_EMIT refused(QStringLiteral("write_file"), path, why);
        return why;
    }

    QFile file(resolved);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString why = FileSandbox::describe(FileSandbox::Rejection::Unreadable);
        Q_EMIT refused(QStringLiteral("write_file"), path, why);
        return why;
    }
    file.write(bytes);
    file.close();

    Q_EMIT performed(QStringLiteral("write_file"), resolved);
    return QStringLiteral("wrote %1 bytes").arg(bytes.size());
}

QString FileTools::list_directory(const QString &path)
{
    FileSandbox::Rejection reason = FileSandbox::Rejection::None;
    const QString resolved = m_sandbox.resolve(path, false, &reason);
    if (resolved.isEmpty()) {
        Q_EMIT refused(QStringLiteral("list_directory"), path, FileSandbox::describe(reason));
        return FileSandbox::describe(reason);
    }

    const QFileInfo info(resolved);
    if (!info.isDir()) {
        const QString why = FileSandbox::describe(FileSandbox::Rejection::Unreadable);
        Q_EMIT refused(QStringLiteral("list_directory"), path, why);
        return why;
    }

    // Names only, sorted. Absolute paths would tell the model where the jail
    // is, which is not information it needs in order to name a file inside it.
    const QStringList entries
            = QDir(resolved).entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);

    Q_EMIT performed(QStringLiteral("list_directory"), resolved);
    return entries.isEmpty() ? QStringLiteral("(empty)") : entries.join(QLatin1Char('\n'));
}

QString FileTools::file_exists(const QString &path)
{
    FileSandbox::Rejection reason = FileSandbox::Rejection::None;
    const QString resolved = m_sandbox.resolve(path, false, &reason);
    if (resolved.isEmpty()) {
        // A path outside the jail answers "no" rather than "you are not allowed
        // to ask": an existence oracle over the whole filesystem is exactly what
        // this tool must not be.
        Q_EMIT refused(QStringLiteral("file_exists"), path, FileSandbox::describe(reason));
        return QStringLiteral("false");
    }
    Q_EMIT performed(QStringLiteral("file_exists"), resolved);
    return QFileInfo::exists(resolved) ? QStringLiteral("true") : QStringLiteral("false");
}

} // namespace Tools
} // namespace QtOpenAi
