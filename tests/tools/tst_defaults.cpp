// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/MetaSchema.h>
#include <QtOpenAi/Tools/DefaultTools.h>
#include <QtOpenAi/Tools/FileTools.h>
#include <QtOpenAi/Tools/HttpTools.h>
#include <QtOpenAi/Tools/UtilityTools.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Client;
using namespace QtOpenAi::Tools;

namespace {

ToolCall call(const QString &name, const QJsonObject &arguments)
{
    return ToolCall(QStringLiteral("call-1"),
                    FunctionCall(name, QString::fromUtf8(QJsonDocument(arguments).toJson(
                                               QJsonDocument::Compact))));
}

} // namespace

// Coverage for the opt-in installer (#53).
class TestDefaultTools : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void anEmptyPolicyInstallsNothing();
    void utilitiesNeedNoPolicy();
    void fileAccessNeedsASandboxWithRoots();
    void writingNeedsBothSwitches();
    void httpNeedsAnAllowList();
    void theApprovalHandlerGatesSideEffects();
    void readsAreNotGatedUnlessAsked();
    void schemasComeFromTheMethods();
    void everyAnnotationOnTheseToolsDescribesSomething();

private:
    QTemporaryDir m_temp;
    QString m_jail;
};

void TestDefaultTools::initTestCase()
{
    QVERIFY(m_temp.isValid());
    m_jail = m_temp.path();
    QFile file(m_jail + QStringLiteral("/notes.txt"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("the notes");
}

void TestDefaultTools::anEmptyPolicyInstallsNothing()
{
    // The whole point of the default: forgetting to configure this cannot be
    // the thing that gives a model your filesystem.
    ToolRegistry registry;
    DefaultTools tools;
    const ToolPolicy nothing;

    QVERIFY(!nothing.utilities);
    QVERIFY(!nothing.fileRead);
    QVERIFY(!nothing.fileWrite);
    QVERIFY(!nothing.httpGet);

    QVERIFY(tools.install(&registry, nothing).isEmpty());
    QVERIFY(registry.toolNames().isEmpty());
    QVERIFY(registry.tools().isEmpty());
}

void TestDefaultTools::utilitiesNeedNoPolicy()
{
    ToolRegistry registry;
    DefaultTools tools;
    ToolPolicy policy;
    policy.utilities = true;

    const QStringList installed = tools.install(&registry, policy);
    QCOMPARE(installed.size(), 3);
    QVERIFY(registry.contains(QStringLiteral("calculate")));
    QVERIFY(registry.contains(QStringLiteral("current_time")));
    QVERIFY(registry.contains(QStringLiteral("uuid")));
    // And nothing that needs one.
    QVERIFY(!registry.contains(QStringLiteral("read_file")));
    QVERIFY(!registry.contains(QStringLiteral("http_get")));

    const Message result
            = registry.invoke(call(QStringLiteral("calculate"),
                                   {{QStringLiteral("expression"), QStringLiteral("6 * 7")}}));
    QCOMPARE(result.content(), QStringLiteral("42"));
}

void TestDefaultTools::fileAccessNeedsASandboxWithRoots()
{
    ToolRegistry registry;
    DefaultTools tools;
    ToolPolicy policy;
    policy.fileRead = true;
    // ... but no sandbox. A tool built on an empty sandbox could only ever
    // refuse, so not installing it is the honest outcome -- and the returned
    // list is how the caller finds out they misconfigured it.
    QVERIFY(tools.install(&registry, policy).isEmpty());
    QVERIFY(registry.toolNames().isEmpty());

    policy.sandbox = FileSandbox({m_jail});
    const QStringList installed = tools.install(&registry, policy);
    QCOMPARE(installed.size(), 3);
    QVERIFY(registry.contains(QStringLiteral("read_file")));
    QVERIFY(registry.contains(QStringLiteral("list_directory")));
    QVERIFY(registry.contains(QStringLiteral("file_exists")));
    // Read access does not imply write access.
    QVERIFY(!registry.contains(QStringLiteral("write_file")));

    const Message read = registry.invoke(
            call(QStringLiteral("read_file"),
                 {{QStringLiteral("path"), m_jail + QStringLiteral("/notes.txt")}}));
    QCOMPARE(read.content(), QStringLiteral("the notes"));
}

void TestDefaultTools::writingNeedsBothSwitches()
{
    // Two switches for the one irreversible power, so neither can turn it on
    // by accident.
    ToolRegistry registry;
    DefaultTools tools;
    ToolPolicy policy;
    policy.fileWrite = true;
    policy.sandbox = FileSandbox({m_jail}); // read-only by default

    QVERIFY(tools.install(&registry, policy).isEmpty());
    QVERIFY(!registry.contains(QStringLiteral("write_file")));

    policy.sandbox.setReadOnly(false);
    const QStringList installed = tools.install(&registry, policy);
    QCOMPARE(installed, QStringList({QStringLiteral("write_file")}));

    const Message wrote = registry.invoke(
            call(QStringLiteral("write_file"),
                 {{QStringLiteral("path"), m_jail + QStringLiteral("/written.txt")},
                  {QStringLiteral("content"), QStringLiteral("by the model")}}));
    QVERIFY(wrote.content().contains(QStringLiteral("wrote")));
    QVERIFY(QFile::exists(m_jail + QStringLiteral("/written.txt")));
}

void TestDefaultTools::httpNeedsAnAllowList()
{
    ToolRegistry registry;
    DefaultTools tools;
    ToolPolicy policy;
    policy.httpGet = true;

    // An HTTP tool with an empty allow-list can do nothing but refuse, and
    // advertising it to the model would only waste tokens.
    QVERIFY(tools.install(&registry, policy).isEmpty());

    policy.allowedHosts = {QStringLiteral("docs.example.com")};
    policy.maxResponseBytes = 1024;
    policy.httpTimeoutMs = 500;
    QCOMPARE(tools.install(&registry, policy), QStringList({QStringLiteral("http_get")}));

    QVERIFY(tools.httpTools());
    QCOMPARE(tools.httpTools()->allowedHosts(), QStringList({QStringLiteral("docs.example.com")}));
    QCOMPARE(tools.httpTools()->maxBytes(), qint64(1024));
    QCOMPARE(tools.httpTools()->timeoutMs(), 500);
    QVERIFY(tools.httpTools()->requiresHttps());
}

void TestDefaultTools::theApprovalHandlerGatesSideEffects()
{
    ToolRegistry registry;
    DefaultTools tools;
    QSignalSpy denied(&tools, &DefaultTools::denied);

    QString askedAbout;
    bool allow = false;
    tools.setApprovalHandler([&](const QString &name, const QJsonObject &arguments) {
        askedAbout = name;
        Q_UNUSED(arguments)
        return allow;
    });
    QVERIFY(tools.hasApprovalHandler());

    ToolPolicy policy;
    policy.fileWrite = true;
    policy.sandbox = FileSandbox({m_jail});
    policy.sandbox.setReadOnly(false);
    tools.install(&registry, policy);

    const auto write = [&](const QString &name) {
        return registry.invoke(call(QStringLiteral("write_file"),
                                    {{QStringLiteral("path"), m_jail + QLatin1Char('/') + name},
                                     {QStringLiteral("content"), QStringLiteral("x")}}));
    };

    // Refused: nothing happens, and the model is told in words it can act on
    // rather than with an error that would end the turn.
    const Message refused = write(QStringLiteral("denied.txt"));
    QCOMPARE(askedAbout, QStringLiteral("write_file"));
    QVERIFY(refused.content().contains(QStringLiteral("not approved")));
    QVERIFY(!QFile::exists(m_jail + QStringLiteral("/denied.txt")));
    QCOMPARE(denied.count(), 1);

    // Approved: it goes through, with the arguments mapped exactly as the
    // ungated path would have mapped them.
    allow = true;
    QVERIFY(write(QStringLiteral("approved.txt")).content().contains(QStringLiteral("wrote")));
    QVERIFY(QFile::exists(m_jail + QStringLiteral("/approved.txt")));
    QCOMPARE(denied.count(), 1);
}

void TestDefaultTools::readsAreNotGatedUnlessAsked()
{
    ToolRegistry registry;
    DefaultTools tools;
    int asked = 0;
    tools.setApprovalHandler([&](const QString &, const QJsonObject &) {
        ++asked;
        return false;
    });

    ToolPolicy policy;
    policy.fileRead = true;
    policy.sandbox = FileSandbox({m_jail});
    tools.install(&registry, policy);

    const auto read = [&]() {
        return registry
                .invoke(call(QStringLiteral("read_file"),
                             {{QStringLiteral("path"), m_jail + QStringLiteral("/notes.txt")}}))
                .content();
    };

    // A prompt per read makes a UI unusable, and the sandbox already bounds
    // what is readable.
    QVERIFY(!tools.approvesReads());
    QCOMPARE(read(), QStringLiteral("the notes"));
    QCOMPARE(asked, 0);

    // But an application that wants to confirm every read can say so.
    ToolRegistry gated;
    DefaultTools strict;
    strict.setApprovalHandler([&](const QString &, const QJsonObject &) {
        ++asked;
        return false;
    });
    strict.setApproveReads(true);
    strict.install(&gated, policy);
    QVERIFY(gated.invoke(call(QStringLiteral("read_file"),
                              {{QStringLiteral("path"), m_jail + QStringLiteral("/notes.txt")}}))
                    .content()
                    .contains(QStringLiteral("not approved")));
    QCOMPARE(asked, 1);
}

void TestDefaultTools::schemasComeFromTheMethods()
{
    // Derived, not written twice: renaming an argument cannot leave a stale
    // schema behind for the model to call with.
    ToolRegistry registry;
    DefaultTools tools;
    ToolPolicy policy;
    policy.utilities = true;
    policy.fileRead = true;
    policy.sandbox = FileSandbox({m_jail});
    tools.install(&registry, policy);

    for (const Tool &tool : registry.tools()) {
        const FunctionDefinition function = tool.function();
        QVERIFY2(!function.description().isEmpty(), qPrintable(function.name()));

        const QJsonObject parameters = function.parameters();
        QCOMPARE(parameters.value(QStringLiteral("type")).toString(), QStringLiteral("object"));

        if (function.name() == QLatin1String("read_file")) {
            const QJsonObject properties
                    = parameters.value(QStringLiteral("properties")).toObject();
            QVERIFY(properties.contains(QStringLiteral("path")));
            QVERIFY(!properties.value(QStringLiteral("path"))
                             .toObject()
                             .value(QStringLiteral("description"))
                             .toString()
                             .isEmpty());
        }
    }

    // Validation is available on top, and rejects a call with the wrong shape
    // before it reaches the sandbox.
    registry.setValidateArguments(true);
    const Message rejected
            = registry.invoke(call(QStringLiteral("read_file"), {{QStringLiteral("path"), 42}}));
    QVERIFY(rejected.content().contains(QStringLiteral("path")));
}

void TestDefaultTools::everyAnnotationOnTheseToolsDescribesSomething()
{
    // These classes declare their tools with QTOPENAI_DOC_INVOKABLE, which
    // writes each name once -- so a name cannot drift from its description here
    // the way it can when the two are written separately. It can still be
    // misspelt in the one place it appears, and a key that matches nothing is
    // simply never read, so the model would be handed a tool with no
    // description and nobody would notice.
    //
    // Only the meta-object knows the real names, so this is the one check that
    // can answer it, and it costs a line per class. Renaming a method or an
    // argument without moving its description fails here.
    QCOMPARE(MetaSchema::danglingAnnotations<FileTools>(), QStringList());
    QCOMPARE(MetaSchema::danglingAnnotations<HttpTools>(), QStringList());
    QCOMPARE(MetaSchema::danglingAnnotations<UtilityTools>(), QStringList());

    // And the descriptions really are on the tools, not merely well-formed:
    // every method these classes offer has one.
    FileTools files;
    HttpTools http;
    UtilityTools utilities;
    const QList<QObject *> everything {&files, &http, &utilities};

    for (QObject *tools : everything) {
        ToolRegistry registry;
        const QMetaObject *meta = tools->metaObject();
        for (int i = meta->methodOffset(); i < meta->methodCount(); ++i) {
            const QMetaMethod method = meta->method(i);
            if (method.methodType() != QMetaMethod::Method)
                continue;
            const QString name = QString::fromUtf8(method.name());
            QVERIFY2(registry.registerMethod(tools, name), qPrintable(name));
        }
        for (const Tool &tool : registry.tools()) {
            QVERIFY2(!tool.function().description().isEmpty(), qPrintable(tool.function().name()));
        }
    }

    // Non-empty is a weak claim: it survives two descriptions swapped between
    // arguments, which is the one mistake the macro cannot rule out by writing
    // each name once. So pin the whole of one tool by content -- write_file,
    // because two parameters is where the expansion has something to get wrong,
    // and because clang-format packs that invocation tightly enough that the
    // triples stop being visually obvious in the header.
    ToolRegistry registry;
    QVERIFY(registry.registerMethod(&files, QStringLiteral("write_file")));

    const FunctionDefinition writeFile = registry.tools().constFirst().function();
    QCOMPARE(writeFile.name(), QStringLiteral("write_file"));
    QCOMPARE(writeFile.description(),
             QStringLiteral("Write UTF-8 text to a file, replacing it if it exists."));

    const QJsonObject properties
            = writeFile.parameters().value(QStringLiteral("properties")).toObject();
    QCOMPARE(properties.value(QStringLiteral("path")).toObject(),
             QJsonObject({{QStringLiteral("type"), QStringLiteral("string")},
                          {QStringLiteral("description"), QStringLiteral("Path to the file to "
                                                                         "write.")}}));
    QCOMPARE(properties.value(QStringLiteral("content")).toObject(),
             QJsonObject({{QStringLiteral("type"), QStringLiteral("string")},
                          {QStringLiteral("description"), QStringLiteral("The text to write.")}}));
}

QTEST_MAIN(TestDefaultTools)
#include "tst_defaults.moc"
