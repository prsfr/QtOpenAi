// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/MetaSchema.h>
#include <QtOpenAi/Tools/FileSandbox.h>
#include <QtOpenAi/Tools/GlobalTools.h>

#include <QtCore/QObject>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Tools {

// Filesystem tools, every one of them inside a FileSandbox.
//
//     FileTools files(FileSandbox({docsPath}));
//     registry.registerMethod(&files, QStringLiteral("read_file"));
//
// The methods are Q_INVOKABLE and annotated, so ToolRegistry::registerMethod()
// derives each tool's JSON-Schema from the method itself -- renaming an
// argument cannot leave a stale schema behind.
//
// Every method resolves its path through the sandbox before touching anything,
// and returns a plain sentence when the sandbox says no. A refusal is a *result*
// rather than an error because the model has to be able to read it and try
// something else; a tool that threw would end the turn instead of correcting it.
class QTOPENAI_TOOLS_EXPORT FileTools : public QObject
{
    Q_OBJECT
    QTOPENAI_DOC("Read and inspect files inside an allowed set of directories.")
    QTOPENAI_DOC_METHOD(read_file, "Read a UTF-8 text file and return its contents.", path,
                        "Path to the file to read.")
    QTOPENAI_DOC_METHOD(write_file, "Write UTF-8 text to a file, replacing it if it exists.", path,
                        "Path to the file to write.", content, "The text to write.")
    QTOPENAI_DOC_METHOD(list_directory, "List the names of the entries in a directory.", path,
                        "Path to the directory to list.")
    QTOPENAI_DOC_METHOD(file_exists, "Report whether a path exists and is readable.", path,
                        "Path to check.")
public:
    explicit FileTools(QObject *parent = nullptr);
    explicit FileTools(const FileSandbox &sandbox, QObject *parent = nullptr);
    ~FileTools() override;

    FileSandbox sandbox() const;
    void setSandbox(const FileSandbox &sandbox);

    Q_INVOKABLE QString read_file(const QString &path);
    Q_INVOKABLE QString write_file(const QString &path, const QString &content);
    Q_INVOKABLE QString list_directory(const QString &path);
    Q_INVOKABLE QString file_exists(const QString &path);

    // The names of the read-only tools, and of the ones that change something.
    // Kept apart because "let the model read the docs" and "let the model
    // rewrite them" are different decisions.
    static QStringList readingTools();
    static QStringList writingTools();

Q_SIGNALS:
    // Every attempt the sandbox turned down, with the path as the model asked
    // for it. An application that never sees this will not know it is under
    // attack; one that logs it will.
    void refused(const QString &tool, const QString &path, const QString &reason);
    // Every call that went through, for an audit trail.
    void performed(const QString &tool, const QString &path);

private:
    FileSandbox m_sandbox;
};

} // namespace Tools
} // namespace QtOpenAi
