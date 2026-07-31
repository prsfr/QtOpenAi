// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/CreateSkillRequest.h>
#include <QtOpenAi/Core/Skill.h>
#include <QtOpenAi/Core/SkillVersion.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Skills types (#27): the skill and its immutable versions,
// the two deletion acknowledgements that share those shapes, and the multipart
// upload body that both creates a skill and adds a version to one.
class TestSkills : public QObject
{
    Q_OBJECT
private slots:
    void parsesSkill();
    void skillRoundTrip();
    void parsesDeletedSkill();
    void parsesSkillVersion();
    void skillVersionRoundTrip();
    void parsesDeletedSkillVersion();
    void parsesSkillList();
    void requestCollectsFiles();
    void requestOmitsUnsetDefault();
};

void TestSkills::parsesSkill()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("skill_123")},
            {QStringLiteral("object"), QStringLiteral("skill")},
            {QStringLiteral("name"), QStringLiteral("pdf-report")},
            {QStringLiteral("description"), QStringLiteral("Builds a PDF report")},
            {QStringLiteral("created_at"), 1716028800},
            {QStringLiteral("default_version"), QStringLiteral("2")},
            {QStringLiteral("latest_version"), QStringLiteral("3")},
    };

    const Skill skill = Skill::fromJson(json);
    QCOMPARE(skill.id(), QStringLiteral("skill_123"));
    QCOMPARE(skill.object(), QStringLiteral("skill"));
    QCOMPARE(skill.name(), QStringLiteral("pdf-report"));
    QCOMPARE(skill.description(), QStringLiteral("Builds a PDF report"));
    QCOMPARE(skill.createdAt(), Q_INT64_C(1716028800));
    // The default and latest versions move independently: publishing a version
    // does not promote it unless asked.
    QCOMPARE(skill.defaultVersion(), QStringLiteral("2"));
    QCOMPARE(skill.latestVersion(), QStringLiteral("3"));
}

void TestSkills::skillRoundTrip()
{
    Skill skill;
    skill.setId(QStringLiteral("skill_123"));
    skill.setObject(QStringLiteral("skill"));
    skill.setName(QStringLiteral("pdf-report"));
    skill.setDescription(QStringLiteral("Builds a PDF report"));
    skill.setCreatedAt(1716028800);
    skill.setDefaultVersion(QStringLiteral("2"));
    skill.setLatestVersion(QStringLiteral("3"));

    QCOMPARE(Skill::fromJson(skill.toJson()), skill);
}

void TestSkills::parsesDeletedSkill()
{
    // DELETE /skills/{id} answers with the same shape, so it decodes into the
    // same type; the object name is what distinguishes it.
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("skill_123")},
            {QStringLiteral("object"), QStringLiteral("skill.deleted")},
            {QStringLiteral("deleted"), true},
    };

    const Skill skill = Skill::fromJson(json);
    QCOMPARE(skill.id(), QStringLiteral("skill_123"));
    QCOMPARE(skill.object(), QStringLiteral("skill.deleted"));
    QVERIFY(skill.name().isEmpty());
}

void TestSkills::parsesSkillVersion()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("skill.version")},
            {QStringLiteral("id"), QStringLiteral("skillver_456")},
            {QStringLiteral("skill_id"), QStringLiteral("skill_123")},
            {QStringLiteral("version"), QStringLiteral("3")},
            {QStringLiteral("created_at"), 1716032400},
            {QStringLiteral("name"), QStringLiteral("pdf-report")},
            {QStringLiteral("description"), QStringLiteral("Now with charts")},
    };

    const SkillVersion version = SkillVersion::fromJson(json);
    QCOMPARE(version.object(), QStringLiteral("skill.version"));
    QCOMPARE(version.id(), QStringLiteral("skillver_456"));
    QCOMPARE(version.skillId(), QStringLiteral("skill_123"));
    // A version number is a string on the wire, not an integer.
    QCOMPARE(version.version(), QStringLiteral("3"));
    QCOMPARE(version.createdAt(), Q_INT64_C(1716032400));
    QCOMPARE(version.name(), QStringLiteral("pdf-report"));
    QCOMPARE(version.description(), QStringLiteral("Now with charts"));
}

void TestSkills::skillVersionRoundTrip()
{
    SkillVersion version;
    version.setObject(QStringLiteral("skill.version"));
    version.setId(QStringLiteral("skillver_456"));
    version.setSkillId(QStringLiteral("skill_123"));
    version.setVersion(QStringLiteral("3"));
    version.setCreatedAt(1716032400);
    version.setName(QStringLiteral("pdf-report"));
    version.setDescription(QStringLiteral("Now with charts"));

    QCOMPARE(SkillVersion::fromJson(version.toJson()), version);
}

void TestSkills::parsesDeletedSkillVersion()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("skill.version.deleted")},
            {QStringLiteral("id"), QStringLiteral("skillver_456")},
            {QStringLiteral("version"), QStringLiteral("3")},
            {QStringLiteral("deleted"), true},
    };

    const SkillVersion version = SkillVersion::fromJson(json);
    QCOMPARE(version.object(), QStringLiteral("skill.version.deleted"));
    QCOMPARE(version.version(), QStringLiteral("3"));
}

void TestSkills::parsesSkillList()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("list")},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), QStringLiteral("skill_1")}},
                         QJsonObject {{QStringLiteral("id"), QStringLiteral("skill_2")}}}},
            {QStringLiteral("first_id"), QStringLiteral("skill_1")},
            {QStringLiteral("last_id"), QStringLiteral("skill_2")},
            {QStringLiteral("has_more"), true},
    };

    const SkillList list = SkillList::fromJson(json);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.data.at(1).id(), QStringLiteral("skill_2"));
    QCOMPARE(list.firstId, QStringLiteral("skill_1"));
    QVERIFY(list.hasMore);
}

void TestSkills::requestCollectsFiles()
{
    // A directory upload: every file keeps its path relative to the skill root,
    // which is how the server rebuilds the bundle.
    CreateSkillRequest request;
    request.addFile(QStringLiteral("SKILL.md"), QByteArray("# pdf-report"));
    request.addFile(QStringLiteral("scripts/build.py"), QByteArray("print(1)"));
    request.setMakeDefault(true);

    QCOMPARE(request.files().size(), 2);
    QCOMPARE(request.files().at(1).first, QStringLiteral("scripts/build.py"));
    QCOMPARE(request.files().at(1).second, QByteArray("print(1)"));
    QCOMPARE(request.makeDefault().value(), true);
    const QList<CreateSkillRequest::FormField> expected {
            {QStringLiteral("default"), QStringLiteral("true")},
    };
    QCOMPARE(request.formFields(), expected);
}

void TestSkills::requestOmitsUnsetDefault()
{
    // POST /skills has no `default` field at all, so an untouched request must
    // not invent one.
    CreateSkillRequest request(QStringLiteral("pdf-report.zip"), QByteArray("PK\x03\x04"));

    QCOMPARE(request.files().size(), 1);
    QCOMPARE(request.files().first().first, QStringLiteral("pdf-report.zip"));
    QVERIFY(!request.makeDefault().has_value());
    QVERIFY(request.formFields().isEmpty());
}

QTEST_MAIN(TestSkills)
#include "tst_skills.moc"
