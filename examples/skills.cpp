// SPDX-License-Identifier: MIT
//
// Publish and version a skill (the /skills endpoints). A skill is a named
// bundle of files a model can be pointed at; the skill object itself is only a
// pointer to versions, so this walks both levels:
//   1. createSkill()                — POST /skills (multipart), version 1
//   2. createSkillVersion()         — POST /skills/{id}/versions, version 2
//   3. listSkillVersions()          — GET /skills/{id}/versions
//   4. setDefaultSkillVersion()     — POST /skills/{id}, promoting version 2
//   5. downloadSkillContent()       — GET /skills/{id}/content (the zip)
//   6. deleteSkill()                — DELETE /skills/{id}
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./skills [path-to-zip]          # defaults to a small generated bundle
//
// Publishing does not promote: step 2 leaves `default_version` at 1 until
// step 4 moves it, which is what makes rolling a skill forward (or back) a
// single call that touches no content.

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

class SkillDemo
{
public:
    SkillDemo(Client::Client *client, Core::CreateSkillRequest bundle)
        : m_client(client)
        , m_bundle(std::move(bundle))
    { }

    // 1. The first upload creates the skill and its version 1 in one call.
    void start()
    {
        Client::SkillReply *reply = m_client->createSkill(m_bundle);
        watch(reply);
        QObject::connect(reply, &Client::SkillReply::finished, [this](const Core::Skill &skill) {
            m_skillId = skill.id();
            print(QStringLiteral("Created skill %1 (%2), version %3")
                          .arg(skill.id(), skill.name(), skill.latestVersion()));
            publishSecondVersion();
        });
    }

private:
    // 2. Every later upload is a new immutable version. The bundle differs only
    // by a line here; in a real project it is whatever the build produced.
    void publishSecondVersion()
    {
        Core::CreateSkillRequest revision;
        revision.addFile(QStringLiteral("SKILL.md"),
                         QByteArray("# qtopenai-example\n\nSecond revision.\n"));

        Client::SkillVersionReply *reply = m_client->createSkillVersion(m_skillId, revision);
        watch(reply);
        QObject::connect(reply, &Client::SkillVersionReply::finished,
                         [this](const Core::SkillVersion &version) {
                             m_newVersion = version.version();
                             print(QStringLiteral("Published version %1").arg(version.version()));
                             listVersions();
                         });
    }

    // 3. Both versions are now there, oldest last.
    void listVersions()
    {
        Client::SkillVersionListReply *reply = m_client->listSkillVersions(m_skillId);
        watch(reply);
        QObject::connect(reply, &Client::SkillVersionListReply::finished,
                         [this](const Core::SkillVersionList &list) {
                             print(QStringLiteral("Versions (%1):").arg(list.size()));
                             for (const Core::SkillVersion &version : list.data) {
                                 print(QStringLiteral("  %1  %2")
                                               .arg(version.version(), version.description()));
                             }
                             promoteNewVersion();
                         });
    }

    // 4. Move the pointer callers get when they name no version.
    void promoteNewVersion()
    {
        Client::SkillReply *reply = m_client->setDefaultSkillVersion(m_skillId, m_newVersion);
        watch(reply);
        QObject::connect(reply, &Client::SkillReply::finished, [this](const Core::Skill &skill) {
            print(QStringLiteral("Default version is now %1").arg(skill.defaultVersion()));
            downloadBundle();
        });
    }

    // 5. The bundle comes back as a zip — raw bytes, not JSON.
    void downloadBundle()
    {
        Client::BinaryReply *reply = m_client->downloadSkillContent(m_skillId);
        watch(reply);
        QObject::connect(reply, &Client::BinaryReply::finished, [this](const QByteArray &bytes) {
            print(QStringLiteral("Downloaded %1 bytes of the default version").arg(bytes.size()));
            cleanUp();
        });
    }

    // 6. Deleting the skill takes its versions with it, so repeated runs leave
    // nothing behind.
    void cleanUp()
    {
        Client::SkillReply *reply = m_client->deleteSkill(m_skillId);
        watch(reply);
        QObject::connect(reply, &Client::SkillReply::finished, [this](const Core::Skill &) {
            print(QStringLiteral("Deleted skill %1").arg(m_skillId));
            qApp->quit();
        });
    }

    // Every request in the chain reports a failure the same way.
    template <typename Reply>
    void watch(Reply *reply)
    {
        QObject::connect(reply, &Reply::failed, [this](const Client::ClientError &error) {
            print(QStringLiteral("Error: %1").arg(error.message()));
            qApp->exit(1);
        });
    }

    void print(const QString &line)
    {
        QTextStream out(stdout);
        out << line << "\n";
    }

    Client::Client *m_client;
    Core::CreateSkillRequest m_bundle;
    QString m_skillId;
    QString m_newVersion;
};

} // namespace

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

    // Either upload the zip named on the command line, or a generated directory
    // bundle — the API takes both, and so does CreateSkillRequest.
    Core::CreateSkillRequest bundle;
    if (argc > 1) {
        QFile file(QString::fromLocal8Bit(argv[1]));
        if (!file.open(QIODevice::ReadOnly)) {
            out << "Cannot read " << file.fileName() << "\n";
            return 1;
        }
        bundle.addFile(QFileInfo(file).fileName(), file.readAll());
    } else {
        // A directory upload: file names are paths relative to the skill root.
        bundle.addFile(QStringLiteral("SKILL.md"),
                       QByteArray("# qtopenai-example\n\nA skill uploaded by the example.\n"));
        bundle.addFile(QStringLiteral("scripts/hello.py"),
                       QByteArray("print(\"hello from the skill bundle\")\n"));
    }

    Client::Client client(QUrl(baseUrl), apiKey, &app);
    SkillDemo demo(&client, std::move(bundle));
    demo.start();

    return app.exec();
}
