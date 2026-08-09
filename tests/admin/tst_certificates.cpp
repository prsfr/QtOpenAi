// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Core/Certificate.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;

namespace {

// A PEM body short enough to read in a diff. The real ones are a few kilobytes,
// which is the reason the listings leave them out.
QByteArray pem()
{
    return "-----BEGIN CERTIFICATE----- MIIGAjCCA...6znFlOW+ -----END CERTIFICATE-----";
}

QByteArray certificatePage()
{
    return R"({"object":"list","data":[
        {"object":"organization.certificate","id":"cert_active","name":"Production",
         "created_at":1234567,"certificate_details":{"valid_at":1234567,"expires_at":12345678},
         "active":true},
        {"object":"organization.certificate","id":"cert_idle","name":null,
         "created_at":1234999,"certificate_details":{"valid_at":1234999,"expires_at":12345999},
         "active":false}],
        "first_id":"cert_active","last_id":"cert_idle","has_more":false})";
}

} // namespace

// Coverage for the administration certificates (#103, under #28). Offline:
// every request goes to the local stub server.
class TestCertificates : public QObject
{
    Q_OBJECT
private slots:
    void aCertificateRoundTripsThroughJson();
    void activeIsUnsetRatherThanFalseWhenTheApiOmitsIt();
    void uploadACertificate();
    void listCertificatesDecodesBothScopes_data();
    void listCertificatesDecodesBothScopes();
    void thePemContentIsAskedForRatherThanAssumed();
    void activatingTakesABatchNotACertificate_data();
    void activatingTakesABatchNotACertificate();
    void anActivationAnswersWithTheCertificatesItChanged();
    void renameAndDeleteACertificate();
};

void TestCertificates::aCertificateRoundTripsThroughJson()
{
    Certificate certificate;
    certificate.setId(QStringLiteral("cert_abc"));
    certificate.setObject(QStringLiteral("certificate"));
    certificate.setName(QStringLiteral("My Certificate"));
    certificate.setCreatedAt(1234567);
    certificate.setValidAt(1234567);
    certificate.setExpiresAt(12345678);
    certificate.setPemContent(QString::fromLatin1(pem()));

    QCOMPARE(Certificate::fromJson(certificate.toJson()), certificate);
    QVERIFY(!certificate.isProjectScoped());
    QVERIFY(!certificate.isDeleted());

    // The validity window is flattened for the caller but goes back on the wire
    // nested, where the API keeps it.
    const QJsonObject json = certificate.toJson();
    QVERIFY(!json.contains(QStringLiteral("valid_at")));
    const QJsonObject details = json.value(QStringLiteral("certificate_details")).toObject();
    QCOMPARE(details.value(QStringLiteral("valid_at")).toInteger(), qint64(1234567));
    QCOMPARE(details.value(QStringLiteral("expires_at")).toInteger(), qint64(12345678));

    // A certificate with no window at all writes no `certificate_details`,
    // rather than an empty object claiming one.
    Certificate bare;
    bare.setId(QStringLiteral("cert_bare"));
    QVERIFY(!bare.toJson().contains(QStringLiteral("certificate_details")));
    QCOMPARE(Certificate::fromJson(bare.toJson()), bare);

    Certificate scoped = certificate;
    scoped.setObject(QStringLiteral("organization.project.certificate"));
    scoped.setActive(true);
    QVERIFY(scoped.isProjectScoped());
    QCOMPARE(Certificate::fromJson(scoped.toJson()), scoped);
}

void TestCertificates::activeIsUnsetRatherThanFalseWhenTheApiOmitsIt()
{
    // Activation belongs to a scope, so a certificate read by id carries no
    // `active` at all. Defaulting that to false would report every certificate
    // fetched this way as switched off.
    const Certificate byId = Certificate::fromJson(
            QJsonDocument::fromJson(R"({"object":"certificate","id":"cert_abc"})").object());
    QVERIFY(!byId.active().has_value());
    QVERIFY(!byId.toJson().contains(QStringLiteral("active")));

    // And a certificate the server really reported as inactive survives as
    // false rather than collapsing into the same "unknown".
    const Certificate inactive = Certificate::fromJson(
            QJsonDocument::fromJson(
                    R"({"object":"organization.certificate","id":"cert_abc","active":false})")
                    .object());
    QCOMPARE(inactive.active(), std::optional<bool>(false));
    QCOMPARE(inactive.toJson().value(QStringLiteral("active")).toBool(), false);
    QCOMPARE(Certificate::fromJson(inactive.toJson()), inactive);
}

void TestCertificates::uploadACertificate()
{
    StubServer server(R"({"object":"certificate","id":"cert_abc","name":"My Certificate",
        "created_at":1234567,"certificate_details":{"valid_at":1234567,"expires_at":12345678,
        "content":"-----BEGIN CERTIFICATE----- MIIGAjCCA...6znFlOW+ -----END CERTIFICATE-----"}})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.uploadCertificate(QString::fromLatin1(pem()),
                                                              QStringLiteral("My Certificate")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QCOMPARE(server.requestLine(), "POST /v1/organization/certificates HTTP/1.1");
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-test"));
    QVERIFY(server.requestBody().contains("\"certificate\":\"-----BEGIN CERTIFICATE-----"));
    QVERIFY(server.requestBody().contains("\"name\":\"My Certificate\""));

    // The upload is one of the two responses that carries the PEM back.
    QCOMPARE(reply->certificate().pemContent(), QString::fromLatin1(pem()));
    QCOMPARE(reply->certificate().expiresAt(), qint64(12345678));
    // Freshly uploaded is not the same as switched on anywhere.
    QVERIFY(!reply->certificate().active().has_value());

    // A certificate may be uploaded without a name, and then the field is
    // absent rather than empty.
    StubServer unnamed(R"({"object":"certificate","id":"cert_x","name":null,"created_at":1})");
    Organization other(unnamed.baseUrl(), QStringLiteral("sk-admin-test"));
    QVERIFY(awaited(other.uploadCertificate(QString::fromLatin1(pem()))));
    QVERIFY(!unnamed.requestBody().contains("\"name\""));
}

void TestCertificates::listCertificatesDecodesBothScopes_data()
{
    QTest::addColumn<bool>("projectScoped");
    QTest::addColumn<QByteArray>("line");

    QTest::newRow("organization") << false
                                  << QByteArray(
                                             "GET /v1/organization/certificates?limit=20 HTTP/1.1");
    // A project's certificates *are* under /organization/projects, unlike its
    // roles -- see Admin::RoleScope.
    QTest::newRow("project")
            << true
            << QByteArray("GET /v1/organization/projects/proj_1/certificates?limit=20 HTTP/1.1");
}

void TestCertificates::listCertificatesDecodesBothScopes()
{
    QFETCH(bool, projectScoped);
    QFETCH(QByteArray, line);

    StubServer server(certificatePage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    QtOpenAi::Client::ListParams params;
    params.limit = 20;
    const auto reply = awaited(
            projectScoped ? organization.listProjectCertificates(QStringLiteral("proj_1"), params)
                          : organization.listCertificates(params));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(), line);

    const CertificateList page = reply->certificates();
    QCOMPARE(page.size(), 2);
    QVERIFY(!page.hasMore);
    // Paginated by item ids, unlike roles and groups -- this family really does
    // send first_id/last_id.
    QCOMPARE(page.firstId, QStringLiteral("cert_active"));
    QCOMPARE(page.lastId, QStringLiteral("cert_idle"));

    QCOMPARE(page.data.at(0).active(), std::optional<bool>(true));
    QCOMPARE(page.data.at(1).active(), std::optional<bool>(false));
    // `name: null` reads back as empty rather than as the string "null".
    QVERIFY(page.data.at(1).name().isEmpty());
    // A listing never carries the PEM, whichever scope it came from.
    QVERIFY(page.data.at(0).pemContent().isEmpty());
}

void TestCertificates::thePemContentIsAskedForRatherThanAssumed()
{
    StubServer plain(R"({"object":"certificate","id":"cert_abc","created_at":1234567,
        "certificate_details":{"valid_at":1234567,"expires_at":12345678}})");
    Organization organization(plain.baseUrl(), QStringLiteral("sk-admin-test"));

    QVERIFY(awaited(organization.getCertificate(QStringLiteral("cert_abc"))));
    // No `include` at all by default: the query string says nothing extra.
    QCOMPARE(plain.requestLine(), "GET /v1/organization/certificates/cert_abc HTTP/1.1");

    StubServer withContent(R"({"object":"certificate","id":"cert_abc","created_at":1234567,
        "certificate_details":{"valid_at":1234567,"expires_at":12345678,
        "content":"-----BEGIN CERTIFICATE----- MIIGAjCCA...6znFlOW+ -----END CERTIFICATE-----"}})");
    Organization other(withContent.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(other.getCertificate(QStringLiteral("cert_abc"), true));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(withContent.requestLine(),
             "GET /v1/organization/certificates/cert_abc?include[]=content HTTP/1.1");
    QCOMPARE(reply->certificate().pemContent(), QString::fromLatin1(pem()));
}

void TestCertificates::activatingTakesABatchNotACertificate_data()
{
    QTest::addColumn<int>("endpoint");
    QTest::addColumn<QByteArray>("line");

    // The verb is a segment on the *collection*. There is no
    // POST /organization/certificates/{id}/activate, which is the reading the
    // method names invite -- so all four paths are pinned here.
    QTest::newRow("activate") << 0
                              << QByteArray("POST /v1/organization/certificates/activate "
                                            "HTTP/1.1");
    QTest::newRow("deactivate") << 1
                                << QByteArray("POST /v1/organization/certificates/deactivate "
                                              "HTTP/1.1");
    QTest::newRow("project activate")
            << 2
            << QByteArray("POST /v1/organization/projects/proj_1/certificates/activate HTTP/1.1");
    QTest::newRow("project deactivate")
            << 3
            << QByteArray("POST /v1/organization/projects/proj_1/certificates/deactivate "
                          "HTTP/1.1");
}

void TestCertificates::activatingTakesABatchNotACertificate()
{
    QFETCH(int, endpoint);
    QFETCH(QByteArray, line);

    StubServer server(QByteArray(R"({"object":"organization.certificate.activation","data":[]})"));
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));
    const QString project = QStringLiteral("proj_1");
    const QStringList ids {QStringLiteral("cert_abc"), QStringLiteral("cert_def")};

    switch (endpoint) {
    case 0:
        QVERIFY(awaited(organization.activateCertificates(ids)));
        break;
    case 1:
        QVERIFY(awaited(organization.deactivateCertificates(ids)));
        break;
    case 2:
        QVERIFY(awaited(organization.activateProjectCertificates(project, ids)));
        break;
    default:
        QVERIFY(awaited(organization.deactivateProjectCertificates(project, ids)));
        break;
    }

    QCOMPARE(server.requestLine(), line);
    // The ids travel in the body as a list, at every one of the four endpoints.
    QCOMPARE(server.requestBody(), R"({"certificate_ids":["cert_abc","cert_def"]})");
}

void TestCertificates::anActivationAnswersWithTheCertificatesItChanged()
{
    StubServer server(R"({"object":"organization.certificate.activation","data":[
        {"object":"organization.certificate","id":"cert_abc","name":"Production",
         "created_at":1234567,"certificate_details":{"valid_at":1234567,"expires_at":12345678},
         "active":true}]})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    // A batch of one is still a batch.
    const auto reply = awaited(organization.activateCertificates({QStringLiteral("cert_abc")}));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestBody(), R"({"certificate_ids":["cert_abc"]})");

    const CertificateList changed = reply->certificates();
    QCOMPARE(changed.size(), 1);
    QCOMPARE(changed.data.at(0).active(), std::optional<bool>(true));
    // The reply is not a page and does not pretend to be one: no cursors, and
    // nothing further to fetch.
    QVERIFY(!changed.hasMore);
    QVERIFY(changed.firstId.isEmpty());
}

void TestCertificates::renameAndDeleteACertificate()
{
    StubServer renamed(R"({"object":"certificate","id":"cert_abc","name":"Renamed",
        "created_at":1234567,"certificate_details":{"valid_at":1234567,"expires_at":12345678}})");
    Organization organization(renamed.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(
            organization.modifyCertificate(QStringLiteral("cert_abc"), QStringLiteral("Renamed")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(renamed.requestLine(), "POST /v1/organization/certificates/cert_abc HTTP/1.1");
    QCOMPARE(renamed.requestBody(), R"({"name":"Renamed"})");
    QCOMPARE(reply->certificate().name(), QStringLiteral("Renamed"));

    StubServer removed(R"({"object":"certificate.deleted","id":"cert_abc"})");
    Organization other(removed.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto gone = awaited(other.deleteCertificate(QStringLiteral("cert_abc")));
    QVERIFY(gone);
    QVERIFY2(gone->isSuccess(), qPrintable(gone->error().message()));
    QCOMPARE(removed.requestLine(), "DELETE /v1/organization/certificates/cert_abc HTTP/1.1");
    // The acknowledgement is the same value type, keeping the id and saying so
    // in `object` -- which is the only signal it gives, because unlike a role's
    // or a group's it carries no `deleted` boolean.
    QCOMPARE(gone->certificate().id(), QStringLiteral("cert_abc"));
    QVERIFY(gone->certificate().isDeleted());
    QVERIFY(!reply->certificate().isDeleted());
}

QTEST_MAIN(TestCertificates)
#include "tst_certificates.moc"
