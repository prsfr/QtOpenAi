// SPDX-License-Identifier: MIT
#include "QtOpenAi/Tools/FileSandbox.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Tools {

namespace {

// Containment by path component, not by characters. "/srv/docs-secret" starts
// with "/srv/docs" as a string and is a completely different directory, and a
// check that missed that would be a jail with a door in it.
bool isInside(const QString &canonicalPath, const QString &canonicalRoot)
{
    if (canonicalPath == canonicalRoot)
        return true;
    QString root = canonicalRoot;
    if (!root.endsWith(QLatin1Char('/')))
        root += QLatin1Char('/');
    return canonicalPath.startsWith(root);
}

} // namespace

class FileSandboxData : public QSharedData
{
public:
    QStringList roots;
    qint64 maxBytes = 1024 * 1024;
    bool readOnly = true;
};

FileSandbox::FileSandbox()
    : d(new FileSandboxData)
{ }

FileSandbox::FileSandbox(const QStringList &roots)
    : d(new FileSandboxData)
{
    setRoots(roots);
}

FileSandbox::FileSandbox(const FileSandbox &other) = default;
FileSandbox::FileSandbox(FileSandbox &&other) noexcept = default;
FileSandbox &FileSandbox::operator=(const FileSandbox &other) = default;
FileSandbox &FileSandbox::operator=(FileSandbox &&other) noexcept = default;
FileSandbox::~FileSandbox() = default;

QStringList FileSandbox::roots() const { return d->roots; }

void FileSandbox::setRoots(const QStringList &roots)
{
    d->roots.clear();
    for (const QString &root : roots)
        addRoot(root);
}

bool FileSandbox::addRoot(const QString &root)
{
    // Canonical once, here, rather than on every check -- and a root that
    // cannot be canonicalised does not exist, and a jail whose wall is missing
    // is not a jail.
    const QString canonical = QFileInfo(root).canonicalFilePath();
    if (canonical.isEmpty() || !QFileInfo(canonical).isDir())
        return false;
    if (!d->roots.contains(canonical))
        d->roots.append(canonical);
    return true;
}

bool FileSandbox::isReadOnly() const { return d->readOnly; }
void FileSandbox::setReadOnly(bool readOnly) { d->readOnly = readOnly; }

qint64 FileSandbox::maxBytes() const { return d->maxBytes; }
void FileSandbox::setMaxBytes(qint64 bytes) { d->maxBytes = qMax(qint64(0), bytes); }

bool FileSandbox::isEmpty() const { return d->roots.isEmpty(); }

bool FileSandbox::allowsSize(qint64 bytes) const
{
    return d->maxBytes <= 0 || bytes <= d->maxBytes;
}

QString FileSandbox::resolve(const QString &path, bool forWriting, Rejection *reason) const
{
    const auto refuse = [reason](Rejection why) {
        if (reason)
            *reason = why;
        return QString();
    };
    if (reason)
        *reason = Rejection::None;

    if (path.trimmed().isEmpty())
        return refuse(Rejection::InvalidPath);
    // Nothing is allowed through a sandbox that was never given anything to
    // allow. This is the deny-by-default, and it is checked first so that a
    // misconfigured sandbox cannot fail open through some later branch.
    if (d->roots.isEmpty())
        return refuse(Rejection::NoRoots);
    if (forWriting && d->readOnly)
        return refuse(Rejection::ReadOnly);

    const QFileInfo info(path);
    QString canonical = info.canonicalFilePath();

    if (canonical.isEmpty()) {
        // It does not exist. For a read that is the end of it; for a write, the
        // *parent* has to resolve inside a root -- a file cannot be
        // canonicalised before it is created, and canonicalising the parent is
        // what catches a symlinked directory pointing out of the jail.
        if (!forWriting)
            return refuse(Rejection::Unreadable);

        const QString parent = info.absolutePath();
        const QString canonicalParent = QFileInfo(parent).canonicalFilePath();
        if (canonicalParent.isEmpty())
            return refuse(Rejection::Unreadable);

        bool contained = false;
        for (const QString &root : d->roots) {
            if (isInside(canonicalParent, root)) {
                contained = true;
                break;
            }
        }
        if (!contained)
            return refuse(Rejection::OutsideRoots);
        return QDir(canonicalParent).filePath(info.fileName());
    }

    // It exists, so the resolved path is the truth about where it is --
    // symlinks followed, ".." collapsed. A traversal attempt and a symlink
    // escape fail here, together, for the same reason.
    for (const QString &root : d->roots) {
        if (isInside(canonical, root))
            return canonical;
    }
    return refuse(Rejection::OutsideRoots);
}

QString FileSandbox::describe(Rejection reason)
{
    // Written for the model: what it did wrong and what it could do instead.
    // "Permission denied" tells it nothing it can act on.
    switch (reason) {
    case Rejection::None:
        return {};
    case Rejection::NoRoots:
        return QStringLiteral("no directories are available to this tool");
    case Rejection::OutsideRoots:
        return QStringLiteral("that path is outside the directories this tool may use");
    case Rejection::ReadOnly:
        return QStringLiteral("this tool may only read, not write");
    case Rejection::TooLarge:
        return QStringLiteral("that file is larger than this tool is allowed to handle");
    case Rejection::Unreadable:
        return QStringLiteral("no such file, or it could not be read");
    case Rejection::InvalidPath:
        return QStringLiteral("that is not a usable path");
    }
    return {};
}

bool FileSandbox::operator==(const FileSandbox &other) const
{
    return d->roots == other.d->roots && d->readOnly == other.d->readOnly
           && d->maxBytes == other.d->maxBytes;
}

} // namespace Tools
} // namespace QtOpenAi
