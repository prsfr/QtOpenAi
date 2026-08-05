// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Core/CreateInviteRequest.h>
#include <QtOpenAi/Core/Invite.h>
#include <QtOpenAi/Core/OrganizationUser.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;

namespace {

QByteArray userPage()
{
    return R"({"object":"list","data":[
        {"object":"organization.user","id":"user_owner","name":"Ada","email":"ada@example.com",
         "role":"owner","added_at":1711471533},
        {"object":"organization.user","id":"user_reader","name":"Bob","email":"bob@example.com",
         "role":"reader","added_at":1711471999}],
        "first_id":"user_owner","last_id":"user_reader","has_more":true})";
}

QByteArray invitePage()
{
    return R"({"object":"list","data":[
        {"object":"organization.invite","id":"invite_1","email":"new@example.com","role":"reader",
         "status":"pending","invited_at":1711471533,"expires_at":1711731533,"accepted_at":null,
         "projects":[{"id":"proj_a","role":"member"}]}],
        "first_id":"invite_1","last_id":"invite_1","has_more":false})";
}

} // namespace

// Coverage for the administration members and invitations (#100, under #28).
// Offline: every request goes to the local stub server.
class TestUsers : public QObject
{
    Q_OBJECT
private slots:
    void aUserRoundTripsThroughJson();
    void anInviteRoundTripsThroughJson();
    void aPendingInviteHasNoAcceptanceStamp();
    void aCreateInviteRequestSerialisesItsProjects();
    void listUsersPassesPaginationAndTheEmailFilter();
    void listUsersDecodesThePage();
    void modifyUserRoleSendsTheRoleAsAPost();
    void deleteUserSendsADeleteAndDecodesTheAcknowledgement();
    void createInviteSendsTheBodyAndDecodesTheInvite();
    void listAndGetInvitesReachTheirEndpoints();
    void deleteInviteSendsADeleteAndDecodesTheAcknowledgement();
};

void TestUsers::aUserRoundTripsThroughJson()
{
    OrganizationUser user;
    user.setId(QStringLiteral("user_abc"));
    user.setObject(QStringLiteral("organization.user"));
    user.setName(QStringLiteral("Ada"));
    user.setEmail(QStringLiteral("ada@example.com"));
    user.setRole(QStringLiteral("owner"));
    user.setAddedAt(1711471533);

    const OrganizationUser restored = OrganizationUser::fromJson(user.toJson());
    QCOMPARE(restored, user);
    QVERIFY(restored.isOwner());

    // A role this build has never heard of has to survive rather than decay to
    // a guess -- reporting an unknown role as "owner" would be the worst of the
    // available mistakes.
    OrganizationUser future;
    future.setRole(QStringLiteral("billing_manager"));
    QCOMPARE(OrganizationUser::fromJson(future.toJson()).role(), QStringLiteral("billing_manager"));
    QVERIFY(!future.isOwner());

    OrganizationUserList page;
    page.data = {user};
    page.firstId = QStringLiteral("user_abc");
    page.lastId = QStringLiteral("user_abc");
    page.hasMore = true;
    QCOMPARE(OrganizationUserList::fromJson(page.toJson()), page);
}

void TestUsers::anInviteRoundTripsThroughJson()
{
    Invite invite;
    invite.setId(QStringLiteral("invite_1"));
    invite.setObject(QStringLiteral("organization.invite"));
    invite.setEmail(QStringLiteral("new@example.com"));
    invite.setRole(QStringLiteral("reader"));
    invite.setStatus(QStringLiteral("accepted"));
    invite.setInvitedAt(1711471533);
    invite.setExpiresAt(1711731533);
    invite.setAcceptedAt(1711481533);
    invite.setProjects({InviteProject {QStringLiteral("proj_a"), QStringLiteral("member")},
                        InviteProject {QStringLiteral("proj_b"), QStringLiteral("owner")}});

    const Invite restored = Invite::fromJson(invite.toJson());
    QCOMPARE(restored, invite);
    QVERIFY(restored.isAccepted());
    QCOMPARE(restored.projects().size(), 2);
    // The project role is not the organization role: a reader in the
    // organization still owns proj_b.
    QCOMPARE(restored.role(), QStringLiteral("reader"));
    QCOMPARE(restored.projects().at(1).role, QStringLiteral("owner"));

    InviteList page;
    page.data = {invite};
    QCOMPARE(InviteList::fromJson(page.toJson()), page);
}

void TestUsers::aPendingInviteHasNoAcceptanceStamp()
{
    // The API sends `"accepted_at": null` while an invitation is outstanding,
    // which is the normal state of a live invite rather than an error.
    const QJsonObject json = QJsonDocument::fromJson(R"({"id":"invite_1",
        "object":"organization.invite","email":"new@example.com","role":"reader",
        "status":"pending","invited_at":17,"expires_at":99,"accepted_at":null})")
                                     .object();

    const Invite invite = Invite::fromJson(json);
    QCOMPARE(invite.acceptedAt(), qint64(0));
    QVERIFY(!invite.isAccepted());
    QVERIFY(invite.projects().isEmpty());
    // Written as an absent key on the way back out, not as a 1970 timestamp.
    QVERIFY(!invite.toJson().contains(QStringLiteral("accepted_at")));
    QVERIFY(!invite.toJson().contains(QStringLiteral("projects")));
}

void TestUsers::aCreateInviteRequestSerialisesItsProjects()
{
    CreateInviteRequest request(QStringLiteral("new@example.com"), QStringLiteral("reader"));
    request.addProject(QStringLiteral("proj_a"), QStringLiteral("member"));

    const QJsonObject json = request.toJson();
    QCOMPARE(json.value(QStringLiteral("email")).toString(), QStringLiteral("new@example.com"));
    QCOMPARE(json.value(QStringLiteral("role")).toString(), QStringLiteral("reader"));
    QCOMPARE(json.value(QStringLiteral("projects")).toArray().size(), 1);
    QCOMPARE(json.value(QStringLiteral("projects"))
                     .toArray()
                     .at(0)
                     .toObject()
                     .value(QStringLiteral("role"))
                     .toString(),
             QStringLiteral("member"));

    // The two required fields go out even when empty, so a missing one comes
    // back as the server's error rather than as a silently different invite.
    const QJsonObject empty = CreateInviteRequest().toJson();
    QVERIFY(empty.contains(QStringLiteral("email")));
    QVERIFY(empty.contains(QStringLiteral("role")));
    QVERIFY(!empty.contains(QStringLiteral("projects")));
}

void TestUsers::listUsersPassesPaginationAndTheEmailFilter()
{
    StubServer server(userPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    QtOpenAi::Client::ListParams params;
    params.after = QStringLiteral("user_owner");
    params.limit = 20;

    QVERIFY(awaited(organization.listUsers(
            params, {QStringLiteral("ada@example.com"), QStringLiteral("bob@example.com")})));

    const QByteArray line = server.requestLine();
    QVERIFY2(line.startsWith("GET /v1/organization/users?"), line.constData());
    QVERIFY(line.contains("after=user_owner"));
    QVERIFY(line.contains("limit=20"));
    // Repeated rather than comma-joined, as the array parameters elsewhere are.
    QVERIFY2(line.contains("emails=ada@example.com&emails=bob@example.com"), line.constData());
}

void TestUsers::listUsersDecodesThePage()
{
    StubServer server(userPage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.listUsers());
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-test"));

    const OrganizationUserList page = reply->users();
    QCOMPARE(page.size(), 2);
    QVERIFY(page.hasMore);
    QCOMPARE(page.lastId, QStringLiteral("user_reader"));
    QCOMPARE(page.data.at(0).email(), QStringLiteral("ada@example.com"));
    QVERIFY(page.data.at(0).isOwner());
    QVERIFY(!page.data.at(1).isOwner());
    QCOMPARE(page.data.at(1).addedAt(), qint64(1711471999));
}

void TestUsers::modifyUserRoleSendsTheRoleAsAPost()
{
    StubServer server(R"({"object":"organization.user","id":"user_reader","name":"Bob",
        "email":"bob@example.com","role":"owner","added_at":1711471999})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(
            organization.modifyUserRole(QStringLiteral("user_reader"), QStringLiteral("owner")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QCOMPARE(server.requestLine(), "POST /v1/organization/users/user_reader HTTP/1.1");
    QCOMPARE(server.requestBody(), R"({"role":"owner"})");
    QVERIFY(reply->user().isOwner());
}

void TestUsers::deleteUserSendsADeleteAndDecodesTheAcknowledgement()
{
    StubServer server(R"({"object":"organization.user.deleted","id":"user_reader",
        "deleted":true})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.deleteUser(QStringLiteral("user_reader")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QCOMPARE(server.requestLine(), "DELETE /v1/organization/users/user_reader HTTP/1.1");
    // A DELETE carries no body, so the chain must not have been shown one.
    QVERIFY(server.requestBody().isEmpty());
    // The acknowledgement decodes into the same type, which is what keeps the
    // id in hand after the member is gone.
    QCOMPARE(reply->user().id(), QStringLiteral("user_reader"));
    QCOMPARE(reply->user().object(), QStringLiteral("organization.user.deleted"));
}

void TestUsers::createInviteSendsTheBodyAndDecodesTheInvite()
{
    StubServer server(R"({"object":"organization.invite","id":"invite_1",
        "email":"new@example.com","role":"reader","status":"pending","invited_at":1711471533,
        "expires_at":1711731533,"accepted_at":null,
        "projects":[{"id":"proj_a","role":"member"}]})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    CreateInviteRequest request(QStringLiteral("new@example.com"), QStringLiteral("reader"));
    request.addProject(QStringLiteral("proj_a"), QStringLiteral("member"));

    const auto reply = awaited(organization.createInvite(request));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QCOMPARE(server.requestLine(), "POST /v1/organization/invites HTTP/1.1");
    QCOMPARE(server.requestBody(),
             R"({"email":"new@example.com","projects":[{"id":"proj_a","role":"member"}],)"
             R"("role":"reader"})");

    const Invite invite = reply->invite();
    QCOMPARE(invite.id(), QStringLiteral("invite_1"));
    QCOMPARE(invite.status(), QStringLiteral("pending"));
    QVERIFY(!invite.isAccepted());
    QCOMPARE(invite.acceptedAt(), qint64(0));
    QCOMPARE(invite.projects().size(), 1);
    QCOMPARE(invite.projects().at(0).id, QStringLiteral("proj_a"));
}

void TestUsers::listAndGetInvitesReachTheirEndpoints()
{
    StubServer server(invitePage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    QtOpenAi::Client::ListParams params;
    params.limit = 5;
    const auto list = awaited(organization.listInvites(params));
    QVERIFY(list);
    QVERIFY(server.requestLine().startsWith("GET /v1/organization/invites?limit=5"));
    QCOMPARE(list->invites().size(), 1);
    QCOMPARE(list->invites().data.at(0).expiresAt(), qint64(1711731533));

    StubServer single(invitePage());
    Organization other(single.baseUrl(), QStringLiteral("sk-admin-test"));
    QVERIFY(awaited(other.getInvite(QStringLiteral("invite_1"))));
    QCOMPARE(single.requestLine(), "GET /v1/organization/invites/invite_1 HTTP/1.1");
}

void TestUsers::deleteInviteSendsADeleteAndDecodesTheAcknowledgement()
{
    StubServer server(R"({"object":"organization.invite.deleted","id":"invite_1",
        "deleted":true})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.deleteInvite(QStringLiteral("invite_1")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QCOMPARE(server.requestLine(), "DELETE /v1/organization/invites/invite_1 HTTP/1.1");
    QCOMPARE(reply->invite().id(), QStringLiteral("invite_1"));
    QCOMPARE(reply->invite().object(), QStringLiteral("organization.invite.deleted"));
}

QTEST_MAIN(TestUsers)
#include "tst_users.moc"
