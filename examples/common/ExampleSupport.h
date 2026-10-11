// SPDX-License-Identifier: MIT
//
// Plumbing the examples share: reading the key from the environment, the
// error handler that ends the program, reading a file named on the command
// line. Nothing here uses the library beyond the type of a failure -- every
// QtOpenAi call stays spelled out in the example that shows it (#175).

#pragma once

#include <QtOpenAi/Client/ClientError.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>

#include <optional>

enum class KeyKind {
    Api,
    Admin
};

struct ExampleEnv
{
    QString key;
    QUrl baseUrl;

    QString model(const QString &fallback) const
    {
        return qEnvironmentVariable("OPENAI_MODEL", fallback);
    }
};

// Never prints; the key is empty when it is not set.
inline ExampleEnv readExampleEnv(KeyKind kind = KeyKind::Api)
{
    return {qEnvironmentVariable(kind == KeyKind::Admin ? "OPENAI_ADMIN_KEY" : "OPENAI_API_KEY"),
            QUrl(qEnvironmentVariable("OPENAI_BASE_URL",
                                      QStringLiteral("https://api.openai.com/v1")))};
}

// Says which variable to set and returns nothing when the key is missing.
// orElse names a way to run without one (", or pass --schema.").
inline std::optional<ExampleEnv> exampleEnv(QTextStream &out, KeyKind kind = KeyKind::Api,
                                            const QString &orElse = {})
{
    ExampleEnv env = readExampleEnv(kind);
    if (!env.key.isEmpty())
        return env;
    if (kind == KeyKind::Admin)
        out << "Set OPENAI_ADMIN_KEY to run this example.\n"
               "It must be an admin key; a standard API key cannot read this surface.\n";
    else
        out << "Set OPENAI_API_KEY to run this example"
            << (orElse.isEmpty() ? QString() : QStringLiteral(", or ") + orElse) << ".\n";
    return std::nullopt;
}

// Exit status 1, never quit(): a script must see a failed request fail.
inline auto failAndExit(QTextStream &out, QCoreApplication &app)
{
    return [&out, &app](const QtOpenAi::Client::ClientError &error) {
        out << "Error: " << error.message() << "\n";
        app.exit(1);
    };
}

inline bool readFileArg(const QString &path, QByteArray &bytes, QString &name, QTextStream &out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        out << "Cannot read " << path << "\n";
        return false;
    }
    bytes = file.readAll();
    name = QFileInfo(file).fileName();
    return true;
}
