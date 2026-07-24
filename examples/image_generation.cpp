// SPDX-License-Identifier: MIT
//
// Generate an image from a text prompt (POST /images/generations). Depending on
// the model the result is a hosted URL or inline base64; this prints the URL (or
// writes the decoded bytes to a file for base64 responses).
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./image "a red cube on a white table" [out.png]

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

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

    const QString prompt = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                    : QStringLiteral("a red cube on a white table");
    const QString path = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QStringLiteral("out.png");

    Client::Client client(QUrl(baseUrl), apiKey);

    Core::ImageGenerationRequest request(prompt, QStringLiteral("gpt-image-1"));
    request.setSize(QStringLiteral("1024x1024"));

    Client::ImageReply *reply = client.createImage(request);

    QObject::connect(reply, &Client::ImageReply::failed,
                     [&out, &app](const Client::ClientError &error) {
                         out << "Error: " << error.message() << "\n";
                         app.exit(1);
                     });

    QObject::connect(reply, &Client::ImageReply::finished,
                     [&out, &app, path](const Core::ImageResponse &response) {
                         const Core::Image image = response.firstImage();
                         if (!image.url().isEmpty()) {
                             out << "Image URL: " << image.url() << "\n";
                         } else if (!image.b64Json().isEmpty()) {
                             QFile file(path);
                             if (file.open(QIODevice::WriteOnly)) {
                                 file.write(QByteArray::fromBase64(image.b64Json().toLatin1()));
                                 out << "Wrote image to " << path << "\n";
                             }
                         }
                         if (!image.revisedPrompt().isEmpty())
                             out << "Revised prompt: " << image.revisedPrompt() << "\n";
                         app.quit();
                     });

    return app.exec();
}
