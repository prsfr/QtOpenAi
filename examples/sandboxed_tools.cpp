// SPDX-License-Identifier: MIT
//
// Giving a model tools, and keeping it to them.
//
//   ToolPolicy policy;                    // everything off
//   policy.utilities = true;
//   policy.fileRead = true;
//   policy.sandbox = FileSandbox({docs});
//   tools.install(&registry, policy);
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./sandboxed_tools /path/to/docs "which file mentions refunds?"
//
// Reads a directory on the model's behalf and answers questions about it. The
// model may name any file it likes; the sandbox decides which of those names
// resolve to something it is allowed to read, and it decides on the *resolved*
// path, so `../../etc/passwd` and a symlink pointing out of the directory fail
// the same check for the same reason.
//
// Two things worth watching in the output:
//
//   * Every refusal is printed. An application that never looks at these will
//     not know it is being probed; one that logs them will.
//   * The approval handler is asked before anything that changes state. Here it
//     asks on the terminal; a GUI would show a dialog. Refusing is answered with
//     a sentence the model can read and work around rather than an error that
//     would end the turn.

#include <QtOpenAi/Chat/Agent.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/ToolRegistry.h>
#include <QtOpenAi/Tools/DefaultTools.h>
#include <QtOpenAi/Tools/FileTools.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

#include <iostream>

using namespace QtOpenAi;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value(QStringLiteral("OPENAI_API_KEY"));
    const QString baseUrl = env.value(QStringLiteral("OPENAI_BASE_URL"),
                                      QStringLiteral("https://api.openai.com/v1"));
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example.\n";
        return 1;
    }
    if (argc < 2) {
        out << "Usage: sandboxed_tools <directory> [question]\n";
        return 1;
    }

    const QString directory = QString::fromLocal8Bit(argv[1]);
    const QString question = argc > 2 ? QString::fromLocal8Bit(argv[2])
                                      : QStringLiteral("What files are here, and what are they "
                                                       "about?");
    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-4o-mini"));

    Client::Client client(QUrl(baseUrl), apiKey);
    Client::ToolRegistry registry;
    // The model's arguments are checked against each tool's own schema before
    // dispatch, so a malformed call is corrected on the next turn rather than
    // reaching the sandbox at all.
    registry.setValidateArguments(true);

    Tools::DefaultTools tools;

    // Asked before anything that changes state. A GUI would show a dialog; this
    // asks on the terminal, which is the same decision in a smaller frame.
    tools.setApprovalHandler([&out](const QString &name, const QJsonObject &arguments) {
        out << "\nThe model wants to run " << name << " with "
            << QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact))
            << "\nAllow it? [y/N] ";
        out.flush();
        std::string answer;
        std::getline(std::cin, answer);
        return answer == "y" || answer == "Y";
    });

    // Everything starts off. Each line below is a power being granted, on
    // purpose, and a reviewer can see exactly which ones.
    Tools::ToolPolicy policy;
    policy.utilities = true;
    policy.fileRead = true;
    policy.fileWrite = true;
    policy.sandbox = Tools::FileSandbox({directory});
    policy.sandbox.setReadOnly(false); // ... and the second switch for writing
    policy.maxFileBytes = 64 * 1024;

    const QStringList installed = tools.install(&registry, policy);
    if (installed.isEmpty()) {
        // Which is what happens when the directory does not exist: a sandbox
        // with no roots allows nothing, so no tool is installed at all.
        out << "No tools were installed -- is " << directory << " a directory?\n";
        return 1;
    }
    out << "Tools: " << installed.join(QStringLiteral(", ")) << "\n\n";

    if (Tools::FileTools *files = tools.fileTools()) {
        QObject::connect(files, &Tools::FileTools::refused,
                         [&out](const QString &tool, const QString &path, const QString &reason) {
                             out << "[refused] " << tool << " " << path << " -- " << reason << "\n";
                         });
        QObject::connect(files, &Tools::FileTools::performed,
                         [&out](const QString &tool, const QString &path) {
                             out << "[ok] " << tool << " " << path << "\n";
                         });
    }

    Chat::Agent agent(&client, &registry);
    agent.setModel(model);
    agent.setSystemPrompt(QStringLiteral(
            "You can read files in one directory using the tools provided. Answer the user's "
            "question from what you find there. If a tool refuses, read what it says and try "
            "something else -- do not guess at the contents of files you could not read."));

    QObject::connect(&agent, &Chat::Agent::finished, [&out, &app](const Core::Message &answer) {
        out << "\n" << answer.content() << "\n";
        app.quit();
    });
    QObject::connect(&agent, &Chat::Agent::failed, [&out, &app](const Client::ClientError &error) {
        out << "\nError: " << error.message() << "\n";
        app.quit();
    });

    agent.run(question);
    return app.exec();
}
