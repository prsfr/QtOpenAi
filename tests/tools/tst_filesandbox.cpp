// SPDX-License-Identifier: MIT
#include <QtOpenAi/Tools/FileTools.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

using namespace QtOpenAi::Tools;

namespace {

void write(const QString &path, const QByteArray &content)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
}

} // namespace

// Coverage for the filesystem sandbox (#53). These are the tests that matter:
// a model that can name a file can name any file, and it is steered by whatever
// text is in its context -- including text an attacker wrote.
class TestFileSandbox : public QObject
{
    Q_OBJECT
private slots:
    void init();

    void nothingIsAllowedByDefault();
    void pathsInsideARootResolve();
    void traversalOutOfTheJailIsRefused();
    void aSymlinkOutOfTheJailIsRefused();
    void aPrefixIsNotAParent();
    void readOnlyBlocksWriting();
    void sizeIsCapped();
    void writingNeedsAResolvableParent();
    void theToolsRefuseAndSayWhy();

private:
    QTemporaryDir m_temp;
    QString m_jail;
    QString m_outside;
};

void TestFileSandbox::init()
{
    QVERIFY(m_temp.isValid());
    // A fresh pair per test: a jail and a directory next to it that must stay
    // unreachable from inside.
    // A fresh subdirectory per test, so one test's symlinks cannot be another's
    // surprise.
    static int counter = 0;
    const QString base = m_temp.filePath(QStringLiteral("case%1").arg(++counter));
    QDir().mkpath(base + QStringLiteral("/jail"));
    QDir().mkpath(base + QStringLiteral("/outside"));
    m_jail = QFileInfo(base + QStringLiteral("/jail")).canonicalFilePath();
    m_outside = QFileInfo(base + QStringLiteral("/outside")).canonicalFilePath();

    write(m_jail + QStringLiteral("/allowed.txt"), "inside the jail");
    write(m_outside + QStringLiteral("/secret.txt"), "not for the model");
}

void TestFileSandbox::nothingIsAllowedByDefault()
{
    // Deny by default. There is no "allow everything" switch, so a sandbox
    // nobody configured cannot be the thing that leaks a filesystem.
    FileSandbox sandbox;
    QVERIFY(sandbox.isEmpty());
    QVERIFY(sandbox.isReadOnly());

    FileSandbox::Rejection reason = FileSandbox::Rejection::None;
    QVERIFY(sandbox.resolve(m_jail + QStringLiteral("/allowed.txt"), false, &reason).isEmpty());
    QCOMPARE(reason, FileSandbox::Rejection::NoRoots);

    // A root that does not exist is not a root: a jail whose wall is missing is
    // not a jail.
    QVERIFY(!sandbox.addRoot(m_jail + QStringLiteral("/nowhere")));
    // Nor is a file.
    QVERIFY(!sandbox.addRoot(m_jail + QStringLiteral("/allowed.txt")));
    QVERIFY(sandbox.isEmpty());

    QVERIFY(sandbox.addRoot(m_jail));
    QVERIFY(!sandbox.isEmpty());
}

void TestFileSandbox::pathsInsideARootResolve()
{
    FileSandbox sandbox({m_jail});
    QDir().mkpath(m_jail + QStringLiteral("/nested"));
    write(m_jail + QStringLiteral("/nested/deep.txt"), "deeper");

    QCOMPARE(sandbox.resolve(m_jail + QStringLiteral("/allowed.txt")),
             m_jail + QStringLiteral("/allowed.txt"));
    QCOMPARE(sandbox.resolve(m_jail + QStringLiteral("/nested/deep.txt")),
             m_jail + QStringLiteral("/nested/deep.txt"));
    // The root itself, and a path that walks out and back in again.
    QCOMPARE(sandbox.resolve(m_jail), m_jail);
    QCOMPARE(sandbox.resolve(m_jail + QStringLiteral("/nested/../allowed.txt")),
             m_jail + QStringLiteral("/allowed.txt"));
}

void TestFileSandbox::traversalOutOfTheJailIsRefused()
{
    FileSandbox sandbox({m_jail});
    FileSandbox::Rejection reason = FileSandbox::Rejection::None;

    // The obvious attempt ...
    QVERIFY(sandbox.resolve(m_jail + QStringLiteral("/../outside/secret.txt"), false, &reason)
                    .isEmpty());
    QCOMPARE(reason, FileSandbox::Rejection::OutsideRoots);

    // ... and the same thing spelled at length, which resolves identically
    // because the check is on the resolved path rather than on the spelling.
    QVERIFY(sandbox.resolve(m_jail + QStringLiteral("/./../outside/./secret.txt")).isEmpty());
    QVERIFY(sandbox.resolve(m_outside + QStringLiteral("/secret.txt")).isEmpty());
    QVERIFY(sandbox.resolve(QStringLiteral("/etc/passwd")).isEmpty());

    QVERIFY(sandbox.resolve(QString(), false, &reason).isEmpty());
    QCOMPARE(reason, FileSandbox::Rejection::InvalidPath);
}

void TestFileSandbox::aSymlinkOutOfTheJailIsRefused()
{
    // The attack a spelling check misses entirely: the path looks like it is
    // inside, and it is not.
    QVERIFY(QFile::link(m_outside + QStringLiteral("/secret.txt"),
                        m_jail + QStringLiteral("/looks-fine.txt")));
    QVERIFY(QFile::link(m_outside, m_jail + QStringLiteral("/looks-like-a-dir")));

    FileSandbox sandbox({m_jail});
    FileSandbox::Rejection reason = FileSandbox::Rejection::None;

    QVERIFY(sandbox.resolve(m_jail + QStringLiteral("/looks-fine.txt"), false, &reason).isEmpty());
    QCOMPARE(reason, FileSandbox::Rejection::OutsideRoots);
    QVERIFY(sandbox.resolve(m_jail + QStringLiteral("/looks-like-a-dir/secret.txt")).isEmpty());

    // And the same for a write through a symlinked parent, which is the case
    // resolving the parent exists to catch.
    sandbox.setReadOnly(false);
    QVERIFY(sandbox.resolve(m_jail + QStringLiteral("/looks-like-a-dir/new.txt"), true, &reason)
                    .isEmpty());
    QCOMPARE(reason, FileSandbox::Rejection::OutsideRoots);
}

void TestFileSandbox::aPrefixIsNotAParent()
{
    // "/srv/docs-secret" starts with "/srv/docs" as a string and is a
    // completely different directory. Containment is by path component.
    const QString sibling = m_jail + QStringLiteral("-secret");
    QVERIFY(QDir().mkpath(sibling));
    write(sibling + QStringLiteral("/secret.txt"), "not for the model");

    FileSandbox sandbox({m_jail});
    QVERIFY(sandbox.resolve(sibling + QStringLiteral("/secret.txt")).isEmpty());
    QVERIFY(sandbox.resolve(sibling).isEmpty());
}

void TestFileSandbox::readOnlyBlocksWriting()
{
    FileSandbox sandbox({m_jail});
    QVERIFY(sandbox.isReadOnly()); // reading is the smaller power, and the default

    FileSandbox::Rejection reason = FileSandbox::Rejection::None;
    QVERIFY(sandbox.resolve(m_jail + QStringLiteral("/allowed.txt"), true, &reason).isEmpty());
    QCOMPARE(reason, FileSandbox::Rejection::ReadOnly);
    // Reading the same path is still fine, which is the point of separating them.
    QVERIFY(!sandbox.resolve(m_jail + QStringLiteral("/allowed.txt"), false).isEmpty());

    sandbox.setReadOnly(false);
    QVERIFY(!sandbox.resolve(m_jail + QStringLiteral("/allowed.txt"), true).isEmpty());
    // A file that does not exist yet resolves for writing, inside the jail.
    QCOMPARE(sandbox.resolve(m_jail + QStringLiteral("/new.txt"), true),
             m_jail + QStringLiteral("/new.txt"));
}

void TestFileSandbox::sizeIsCapped()
{
    FileSandbox sandbox({m_jail});
    QCOMPARE(sandbox.maxBytes(), qint64(1024 * 1024));

    sandbox.setMaxBytes(10);
    QVERIFY(sandbox.allowsSize(10));
    QVERIFY(!sandbox.allowsSize(11));

    // 0 means no limit, and should be a deliberate decision.
    sandbox.setMaxBytes(0);
    QVERIFY(sandbox.allowsSize(1024 * 1024 * 1024));
    // Negative is nonsense and clamps rather than inverting the check.
    sandbox.setMaxBytes(-5);
    QCOMPARE(sandbox.maxBytes(), qint64(0));
}

void TestFileSandbox::writingNeedsAResolvableParent()
{
    FileSandbox sandbox({m_jail});
    sandbox.setReadOnly(false);
    FileSandbox::Rejection reason = FileSandbox::Rejection::None;

    // A parent that does not exist cannot be checked, so it is refused rather
    // than assumed to be inside.
    QVERIFY(sandbox.resolve(m_jail + QStringLiteral("/no/such/dir/file.txt"), true, &reason)
                    .isEmpty());
    QCOMPARE(reason, FileSandbox::Rejection::Unreadable);

    // Reading something that is not there is Unreadable, not OutsideRoots --
    // the model can tell "wrong name" from "not allowed" and act on it.
    QVERIFY(sandbox.resolve(m_jail + QStringLiteral("/missing.txt"), false, &reason).isEmpty());
    QCOMPARE(reason, FileSandbox::Rejection::Unreadable);
}

void TestFileSandbox::theToolsRefuseAndSayWhy()
{
    FileTools tools(FileSandbox({m_jail}));
    QSignalSpy refused(&tools, &FileTools::refused);
    QSignalSpy performed(&tools, &FileTools::performed);

    QCOMPARE(tools.read_file(m_jail + QStringLiteral("/allowed.txt")),
             QStringLiteral("inside the jail"));
    QCOMPARE(performed.count(), 1);

    // A refusal is a *result*, not an exception: the model has to be able to
    // read it and try something else, and a thrown error would end the turn
    // instead of correcting it.
    const QString denied = tools.read_file(m_outside + QStringLiteral("/secret.txt"));
    QVERIFY2(!denied.contains(QStringLiteral("not for the model")), qPrintable(denied));
    QVERIFY(denied.contains(QStringLiteral("outside")));
    QCOMPARE(refused.count(), 1);

    // file_exists answers "false" rather than "not allowed": an existence
    // oracle over the whole filesystem is exactly what it must not be.
    QCOMPARE(tools.file_exists(m_outside + QStringLiteral("/secret.txt")), QStringLiteral("false"));
    QCOMPARE(tools.file_exists(m_jail + QStringLiteral("/allowed.txt")), QStringLiteral("true"));

    // Read-only by default, so a write is refused before it happens.
    const QString write
            = tools.write_file(m_jail + QStringLiteral("/new.txt"), QStringLiteral("content"));
    QVERIFY(write.contains(QStringLiteral("only read")));
    QVERIFY(!QFile::exists(m_jail + QStringLiteral("/new.txt")));

    FileSandbox writable({m_jail});
    writable.setReadOnly(false);
    tools.setSandbox(writable);
    QVERIFY(tools.write_file(m_jail + QStringLiteral("/new.txt"), QStringLiteral("content"))
                    .contains(QStringLiteral("wrote")));
    QVERIFY(QFile::exists(m_jail + QStringLiteral("/new.txt")));
    // And still not outside it.
    QVERIFY(tools.write_file(m_outside + QStringLiteral("/new.txt"), QStringLiteral("content"))
                    .contains(QStringLiteral("outside")));
    QVERIFY(!QFile::exists(m_outside + QStringLiteral("/new.txt")));

    // Listing gives names, not absolute paths: where the jail is on disk is not
    // information the model needs in order to name a file inside it.
    const QString listing = tools.list_directory(m_jail);
    QVERIFY(listing.contains(QStringLiteral("allowed.txt")));
    QVERIFY(!listing.contains(m_jail));
    QVERIFY(tools.list_directory(m_outside).contains(QStringLiteral("outside")));

    // The cap is checked before the file is read, not after.
    FileSandbox tiny({m_jail});
    tiny.setMaxBytes(4);
    tools.setSandbox(tiny);
    const QString capped = tools.read_file(m_jail + QStringLiteral("/allowed.txt"));
    QVERIFY(capped.contains(QStringLiteral("larger")));
    QVERIFY(!capped.contains(QStringLiteral("inside the jail")));
}

QTEST_MAIN(TestFileSandbox)
#include "tst_filesandbox.moc"
