// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Tools/GlobalTools.h>

#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Tools {

class FileSandboxData;

// The jail the filesystem tools run inside.
//
//     FileSandbox sandbox({QStringLiteral("/srv/docs")});
//     sandbox.setReadOnly(true);
//     sandbox.setMaxBytes(256 * 1024);
//
// A model that can name a file can name any file, and it is being steered by
// whatever text happens to be in its context -- including text an attacker
// wrote. So the question this class answers is never "is this path suspicious"
// but "is this path, after every symlink has been followed, inside a directory
// the application named". Anything else is refused.
//
// It refuses by **default**: a default-constructed sandbox has no roots, and a
// sandbox with no roots allows nothing. There is deliberately no "allow
// everything" switch -- an application that means the whole filesystem can say
// so by naming "/", which is a sentence a reviewer will notice.
//
// What it enforces:
//
//   * **Containment.** A path is resolved to its canonical form, symlinks and
//     `..` and all, and must then be inside a root. `/srv/docs/../etc` and a
//     symlink from inside the jail to `/etc/shadow` fail the same check for the
//     same reason, which is why the check is on the resolved path rather than
//     on the spelling.
//   * **Prefix honesty.** `/srv/docs-secret` is not inside `/srv/docs`, even
//     though one string starts with the other. Containment is by path
//     component, not by character.
//   * **Read-only.** Separate from access: reading a corpus and writing to it
//     are different powers, and the second is the one that cannot be undone.
//   * **Size.** A cap on what may be read or written, because a tool result is
//     pasted straight back into a context window and a multi-gigabyte file is
//     a denial of service against the caller's own wallet.
//
// An implicitly-shared value type, so a policy can be copied into each tool
// that needs it without anyone owning it.
class QTOPENAI_TOOLS_EXPORT FileSandbox
{
public:
    // Why a path was refused. Reported to the model so it can correct itself,
    // and to the application so it can log the attempt.
    enum class Rejection {
        None,
        NoRoots,      // nothing was allowed in the first place
        OutsideRoots, // resolved outside every root -- traversal or symlink escape
        ReadOnly,     // a write against a read-only sandbox
        TooLarge,     // over maxBytes()
        Unreadable,   // does not exist, or the OS said no
        InvalidPath   // empty, or not a path at all
    };

    FileSandbox();
    explicit FileSandbox(const QStringList &roots);
    FileSandbox(const FileSandbox &other);
    FileSandbox(FileSandbox &&other) noexcept;
    FileSandbox &operator=(const FileSandbox &other);
    FileSandbox &operator=(FileSandbox &&other) noexcept;
    ~FileSandbox();

    void swap(FileSandbox &other) noexcept { d.swap(other.d); }

    // Directories the tools may touch. Stored canonically, so a root that is
    // itself a symlink is resolved once here rather than on every check.
    // A root that does not exist is not added: a jail whose wall is missing is
    // not a jail.
    QStringList roots() const;
    void setRoots(const QStringList &roots);
    bool addRoot(const QString &root);

    // No writing, no creating, no deleting. True by default -- read access is
    // the smaller power and the one an application is more likely to have meant.
    bool isReadOnly() const;
    void setReadOnly(bool readOnly);

    // Largest file that may be read or written, in bytes. Default 1 MiB, which
    // is already a large slice of a context window. 0 means no limit and should
    // be a deliberate decision.
    qint64 maxBytes() const;
    void setMaxBytes(qint64 bytes);

    // Nothing is allowed through a sandbox with no roots.
    bool isEmpty() const;

    // The canonical path to use, or an empty string if `path` is not allowed.
    // `reason` reports why when it is not.
    //
    // For a path that does not exist yet -- the write case -- the *parent* must
    // resolve inside a root, since a file cannot be canonicalised before it is
    // created. Creating a file through a symlinked parent that points outside
    // is therefore caught, which is the case that matters.
    QString resolve(const QString &path, bool forWriting = false,
                    Rejection *reason = nullptr) const;

    // Whether a size is within the cap.
    bool allowsSize(qint64 bytes) const;

    // A sentence for the model: why it was refused, in terms it can act on.
    static QString describe(Rejection reason);

    bool operator==(const FileSandbox &other) const;
    bool operator!=(const FileSandbox &other) const { return !(*this == other); }

private:
    QSharedDataPointer<FileSandboxData> d;
};

} // namespace Tools
} // namespace QtOpenAi
