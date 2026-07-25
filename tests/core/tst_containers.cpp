// SPDX-License-Identifier: MIT
#include <QtOpenAi/Core/Container.h>
#include <QtOpenAi/Core/CreateContainerRequest.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

using namespace QtOpenAi::Core;

// Coverage for the Containers types (#19): container and container-file parsing
// and round-trips, the list shapes, and the create body.
class TestContainers : public QObject
{
    Q_OBJECT
private slots:
    void parsesContainer();
    void containerRoundTrip();
    void parsesDeletionAcknowledgement();
    void parsesContainerList();
    void parsesContainerFile();
    void containerFileRoundTrip();
    void parsesContainerFileList();
    void createRequestJsonBody();
    void createRequestOmitsUnsetFields();
};

void TestContainers::parsesContainer()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("cntr_abc")},
            {QStringLiteral("object"), QStringLiteral("container")},
            {QStringLiteral("created_at"), 1747844794},
            {QStringLiteral("name"), QStringLiteral("analysis")},
            {QStringLiteral("status"), QStringLiteral("running")},
            {QStringLiteral("expires_after"),
             QJsonObject {{QStringLiteral("anchor"), QStringLiteral("last_active_at")},
                          {QStringLiteral("minutes"), 20}}},
    };

    const Container container = Container::fromJson(json);
    QCOMPARE(container.id(), QStringLiteral("cntr_abc"));
    QCOMPARE(container.name(), QStringLiteral("analysis"));
    QCOMPARE(container.status(), QStringLiteral("running"));
    QCOMPARE(container.createdAt(), Q_INT64_C(1747844794));
    QCOMPARE(container.expiresAfterAnchor(), QStringLiteral("last_active_at"));
    QCOMPARE(container.expiresAfterMinutes(), 20);
}

void TestContainers::containerRoundTrip()
{
    Container container;
    container.setId(QStringLiteral("cntr_1"));
    container.setObject(QStringLiteral("container"));
    container.setCreatedAt(1700000000);
    container.setName(QStringLiteral("sandbox"));
    container.setStatus(QStringLiteral("running"));
    container.setExpiresAfter(QStringLiteral("last_active_at"), 20);

    QCOMPARE(Container::fromJson(container.toJson()), container);
}

void TestContainers::parsesDeletionAcknowledgement()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("cntr_1")},
            {QStringLiteral("object"), QStringLiteral("container.deleted")},
            {QStringLiteral("deleted"), true},
    };

    const Container container = Container::fromJson(json);
    QCOMPARE(container.id(), QStringLiteral("cntr_1"));
    QCOMPARE(container.object(), QStringLiteral("container.deleted"));
    QCOMPARE(container.expiresAfterMinutes(), 0);
}

void TestContainers::parsesContainerList()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("list")},
            {QStringLiteral("data"),
             QJsonArray {QJsonObject {{QStringLiteral("id"), QStringLiteral("cntr_1")}},
                         QJsonObject {{QStringLiteral("id"), QStringLiteral("cntr_2")}}}},
            {QStringLiteral("first_id"), QStringLiteral("cntr_1")},
            {QStringLiteral("last_id"), QStringLiteral("cntr_2")},
            {QStringLiteral("has_more"), false},
    };

    const ContainerList list = ContainerList::fromJson(json);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.data.at(1).id(), QStringLiteral("cntr_2"));
    QCOMPARE(ContainerList::fromJson(list.toJson()), list);
}

void TestContainers::parsesContainerFile()
{
    const QJsonObject json {
            {QStringLiteral("id"), QStringLiteral("cfile_abc")},
            {QStringLiteral("object"), QStringLiteral("container.file")},
            {QStringLiteral("created_at"), 1747848842},
            {QStringLiteral("bytes"), 880},
            {QStringLiteral("container_id"), QStringLiteral("cntr_abc")},
            {QStringLiteral("path"), QStringLiteral("/mnt/data/report.csv")},
            {QStringLiteral("source"), QStringLiteral("user")},
    };

    const ContainerFile file = ContainerFile::fromJson(json);
    QCOMPARE(file.id(), QStringLiteral("cfile_abc"));
    QCOMPARE(file.bytes(), Q_INT64_C(880));
    QCOMPARE(file.containerId(), QStringLiteral("cntr_abc"));
    QCOMPARE(file.path(), QStringLiteral("/mnt/data/report.csv"));
    QCOMPARE(file.source(), QStringLiteral("user"));
}

void TestContainers::containerFileRoundTrip()
{
    ContainerFile file;
    file.setId(QStringLiteral("cfile_1"));
    file.setObject(QStringLiteral("container.file"));
    file.setCreatedAt(1700000000);
    file.setBytes(42);
    file.setContainerId(QStringLiteral("cntr_1"));
    file.setPath(QStringLiteral("/mnt/data/a.txt"));
    file.setSource(QStringLiteral("assistant"));

    QCOMPARE(ContainerFile::fromJson(file.toJson()), file);
}

void TestContainers::parsesContainerFileList()
{
    const QJsonObject json {
            {QStringLiteral("object"), QStringLiteral("list")},
            {QStringLiteral("data"),
             QJsonArray {
                     QJsonObject {{QStringLiteral("id"), QStringLiteral("cfile_1")},
                                  {QStringLiteral("path"), QStringLiteral("/mnt/data/a.txt")}}}},
            {QStringLiteral("has_more"), false},
    };

    const ContainerFileList list = ContainerFileList::fromJson(json);
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.data.first().path(), QStringLiteral("/mnt/data/a.txt"));
    QCOMPARE(ContainerFileList::fromJson(list.toJson()), list);
}

void TestContainers::createRequestJsonBody()
{
    CreateContainerRequest request(QStringLiteral("analysis"),
                                   {QStringLiteral("file-1"), QStringLiteral("file-2")});
    request.setExpiresAfter(QStringLiteral("last_active_at"), 20);

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("name")).toString(), QStringLiteral("analysis"));
    QCOMPARE(json.value(QStringLiteral("file_ids")).toArray().size(), 2);
    QCOMPARE(json.value(QStringLiteral("expires_after"))
                     .toObject()
                     .value(QStringLiteral("minutes"))
                     .toInt(),
             20);
}

void TestContainers::createRequestOmitsUnsetFields()
{
    CreateContainerRequest request(QStringLiteral("bare"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.keys(), QStringList {QStringLiteral("name")});
}

QTEST_MAIN(TestContainers)
#include "tst_containers.moc"
