// SPDX-License-Identifier: MIT
#include <QtOpenAi/Admin/Organization.h>
#include <QtOpenAi/Core/Group.h>
#include <QtOpenAi/Core/OrganizationRole.h>
#include <QtOpenAi/Core/RoleAssignment.h>
#include <QtOpenAi/Core/RoleRequest.h>

#include <QtTest/QtTest>

#include "support/AwaitedReply.h"
#include "support/StubServer.h"

using namespace QtOpenAi::Core;
using namespace QtOpenAi::Admin;

namespace {

QByteArray rolePage()
{
    return R"({"object":"list","data":[
        {"object":"role","id":"role_manager","name":"API Group Manager",
         "description":"Allows managing organization groups",
         "permissions":["api.groups.read","api.groups.write"],
         "resource_type":"api.organization","predefined_role":false},
        {"object":"role","id":"role_owner","name":"Owner","description":null,
         "permissions":["*"],"resource_type":"api.organization","predefined_role":true}],
        "has_more":true,"next":"role_owner"})";
}

QByteArray groupPage()
{
    return R"({"object":"list","data":[
        {"id":"group_support","name":"Support Team","created_at":1711471533,
         "is_scim_managed":false,"group_type":"group"},
        {"id":"group_sales","name":"Sales","created_at":1711472599,
         "is_scim_managed":true,"group_type":"group"}],
        "has_more":false,"next":null})";
}

// A role as an assignment listing reports it: the same fields plus provenance,
// and this one is held through a group rather than directly.
QByteArray inheritedRolePage()
{
    return R"({"object":"list","data":[
        {"id":"role_manager","name":"API Group Manager","permissions":["api.groups.read"],
         "resource_type":"api.organization","predefined_role":false,
         "description":"Allows managing organization groups",
         "created_at":1711471533,"updated_at":1711472599,"created_by":"user_ada",
         "created_by_user_obj":{"id":"user_ada","name":"Ada","email":"ada@example.com"},
         "metadata":{},"assignment_sources":[
             {"principal_id":"group_support","principal_type":"group"}]}],
        "has_more":false,"next":null})";
}

} // namespace

// Coverage for the administration roles and groups (#102, under #28). Offline:
// every request goes to the local stub server.
class TestRoles : public QObject
{
    Q_OBJECT
private slots:
    void aRoleRoundTripsThroughJson();
    void aGroupRoundTripsThroughJson();
    void aGroupReadsBothSpellingsOfTheScimFlag();
    void aRoleRequestSendsOnlyWhatWasSet();
    void listRolesDecodesTheCursorPage();
    void rolePathsAreComposedFromTheScope_data();
    void rolePathsAreComposedFromTheScope();
    void createAGroupAndDeleteIt();
    void addingAGroupMemberIsAcknowledgedNotReturned();
    void assignARoleAtBothScopes_data();
    void assignARoleAtBothScopes();
    void anInheritedRoleReportsWhereItCameFrom();
    void unassigningAnswersWithAlmostNothing();
    void grantingAGroupAccessToAProjectPutsBothIdsInTheBody();
};

void TestRoles::aRoleRoundTripsThroughJson()
{
    OrganizationRole role;
    role.setId(QStringLiteral("role_manager"));
    role.setObject(QStringLiteral("role"));
    role.setName(QStringLiteral("API Group Manager"));
    role.setDescription(QStringLiteral("Allows managing organization groups"));
    role.setPermissions({QStringLiteral("api.groups.read"), QStringLiteral("api.groups.write")});
    role.setResourceType(QStringLiteral("api.organization"));

    QCOMPARE(OrganizationRole::fromJson(role.toJson()), role);
    QVERIFY(!role.isProjectScoped());
    QVERIFY(!role.predefinedRole());
    // A role read out of the catalogue carries no provenance, which is not the
    // same as a role created at the epoch by nobody.
    QVERIFY(!role.toJson().contains(QStringLiteral("created_at")));
    QVERIFY(!role.isInherited());

    // The same class carries the assignment listing's extra fields.
    role.setCreatedAt(1711471533);
    role.setUpdatedAt(1711472599);
    role.setCreatedBy(QStringLiteral("user_ada"));
    role.setAssignmentSources({{QStringLiteral("group_support"), QStringLiteral("group")}});

    const OrganizationRole assigned = OrganizationRole::fromJson(role.toJson());
    QCOMPARE(assigned, role);
    QVERIFY(assigned.isInherited());
    QCOMPARE(assigned.assignmentSources().size(), 1);
    QVERIFY(assigned.assignmentSources().first().isGroup());

    // A project role differs from an organization one in this field and in
    // nothing else -- which is why one class serves both scopes.
    OrganizationRole projectRole = role;
    projectRole.setResourceType(QStringLiteral("api.project"));
    QVERIFY(projectRole.isProjectScoped());
    QCOMPARE(OrganizationRole::fromJson(projectRole.toJson()), projectRole);
}

void TestRoles::aGroupRoundTripsThroughJson()
{
    Group group;
    group.setId(QStringLiteral("group_support"));
    group.setObject(QStringLiteral("group"));
    group.setName(QStringLiteral("Support Team"));
    group.setCreatedAt(1711471533);
    group.setGroupType(QStringLiteral("group"));

    QCOMPARE(Group::fromJson(group.toJson()), group);
    QVERIFY(!group.isScimManaged());
    QVERIFY(!group.isDeleted());

    group.setScimManaged(true);
    const Group managed = Group::fromJson(group.toJson());
    QCOMPARE(managed, group);
    QVERIFY(managed.isScimManaged());
    // One spelling goes out, whichever came in.
    QVERIFY(group.toJson().contains(QStringLiteral("is_scim_managed")));
    QVERIFY(!group.toJson().contains(QStringLiteral("scim_managed")));

    GroupMember member;
    member.setId(QStringLiteral("user_ada"));
    member.setName(QStringLiteral("Ada"));
    member.setEmail(QStringLiteral("ada@example.com"));
    member.setUserType(QStringLiteral("user"));
    QCOMPARE(GroupMember::fromJson(member.toJson()), member);
    QVERIFY(!member.isServiceAccount());
}

void TestRoles::aGroupReadsBothSpellingsOfTheScimFlag()
{
    // The groups endpoints send `is_scim_managed`; the same group embedded in a
    // role assignment sends `scim_managed`. Reading only one would report a
    // synchronised group as editable in exactly one of the two places -- and
    // that is the place where the edit is silently undone by the next sync.
    const Group fromEndpoint = Group::fromJson(
            QJsonDocument::fromJson(R"({"id":"group_1","is_scim_managed":true})").object());
    QVERIFY(fromEndpoint.isScimManaged());

    const Group fromAssignment = Group::fromJson(
            QJsonDocument::fromJson(R"({"id":"group_1","scim_managed":true})").object());
    QVERIFY(fromAssignment.isScimManaged());

    // An explicit false in the endpoint spelling is not overridden by the
    // fallback's absence, and neither spelling invents a true.
    const Group unmanaged = Group::fromJson(
            QJsonDocument::fromJson(R"({"id":"group_1","is_scim_managed":false})").object());
    QVERIFY(!unmanaged.isScimManaged());
}

void TestRoles::aRoleRequestSendsOnlyWhatWasSet()
{
    RoleRequest request;
    QVERIFY(request.isEmpty());
    QVERIFY(request.toJson().isEmpty());

    request.setDescription(QStringLiteral("Now with fewer powers"));
    QVERIFY(!request.isEmpty());
    // The name and the permissions were not mentioned, so they are left alone
    // rather than cleared -- on this type that is the difference between a role
    // that works and a role that grants nothing.
    QVERIFY(!request.toJson().contains(QStringLiteral("role_name")));
    QVERIFY(!request.toJson().contains(QStringLiteral("permissions")));
    QCOMPARE(RoleRequest::fromJson(request.toJson()), request);

    // `role_name` on the way in, `name` on the way out. The API's spelling.
    request.setRoleName(QStringLiteral("API Group Manager"));
    QCOMPARE(request.toJson().value(QStringLiteral("role_name")).toString(),
             QStringLiteral("API Group Manager"));
    QVERIFY(!request.toJson().contains(QStringLiteral("name")));

    // An explicitly empty permission list is a real request -- revoke
    // everything -- and survives as one rather than being dropped as "empty".
    RoleRequest revoke;
    revoke.setPermissions({});
    QVERIFY(!revoke.isEmpty());
    QVERIFY(revoke.toJson().contains(QStringLiteral("permissions")));
    QVERIFY(revoke.toJson().value(QStringLiteral("permissions")).toArray().isEmpty());
    QCOMPARE(RoleRequest::fromJson(revoke.toJson()), revoke);
}

void TestRoles::listRolesDecodesTheCursorPage()
{
    StubServer server(rolePage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    QtOpenAi::Client::ListParams params;
    params.limit = 20;
    const auto reply = awaited(organization.listRoles(RoleScope::organization(), params));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(), "GET /v1/organization/roles?limit=20 HTTP/1.1");
    QVERIFY(server.requestHeaders().contains("Authorization: Bearer sk-admin-test"));

    const OrganizationRoleList page = reply->roles();
    QCOMPARE(page.size(), 2);
    QVERIFY(page.hasMore);
    // Paginated by an opaque cursor rather than by item ids: `next` is what goes
    // back as `after`, and there is no first_id/last_id to mistake it for.
    QCOMPARE(page.next, QStringLiteral("role_owner"));

    QCOMPARE(page.data.at(0).permissions().size(), 2);
    QVERIFY(!page.data.at(0).predefinedRole());
    // A predefined role cannot be edited or deleted, which is worth knowing
    // before offering the button.
    QVERIFY(page.data.at(1).predefinedRole());
    // `description: null` reads back as empty rather than as the string "null".
    QVERIFY(page.data.at(1).description().isEmpty());
}

void TestRoles::rolePathsAreComposedFromTheScope_data()
{
    QTest::addColumn<int>("endpoint");
    QTest::addColumn<QByteArray>("line");

    // The point of the table: **a project's roles are not under
    // /organization/projects**, where its groups and users are. One composition
    // rule serves both scopes and both principals, and a typo in it is a 404
    // rather than a wrong answer.
    QTest::newRow("org roles") << 0 << QByteArray("GET /v1/organization/roles HTTP/1.1");
    QTest::newRow("project roles") << 1 << QByteArray("GET /v1/projects/proj_1/roles HTTP/1.1");
    QTest::newRow("org role") << 2 << QByteArray("GET /v1/organization/roles/role_1 HTTP/1.1");
    QTest::newRow("project role") << 3
                                  << QByteArray("GET /v1/projects/proj_1/roles/role_1 HTTP/1.1");
    QTest::newRow("org group roles")
            << 4 << QByteArray("GET /v1/organization/groups/group_1/roles HTTP/1.1");
    QTest::newRow("project group roles")
            << 5 << QByteArray("GET /v1/projects/proj_1/groups/group_1/roles HTTP/1.1");
    QTest::newRow("org user roles")
            << 6 << QByteArray("GET /v1/organization/users/user_1/roles HTTP/1.1");
    QTest::newRow("project user role")
            << 7 << QByteArray("GET /v1/projects/proj_1/users/user_1/roles/role_1 HTTP/1.1");
    // The groups themselves stay where the rest of the administration surface
    // is, project groups included -- which is exactly the inconsistency the
    // rows above exist to pin down.
    QTest::newRow("groups") << 8 << QByteArray("GET /v1/organization/groups HTTP/1.1");
    QTest::newRow("group users") << 9
                                 << QByteArray(
                                            "GET /v1/organization/groups/group_1/users HTTP/1.1");
    QTest::newRow("group user")
            << 10 << QByteArray("GET /v1/organization/groups/group_1/users/user_1 HTTP/1.1");
    QTest::newRow("project groups")
            << 11 << QByteArray("GET /v1/organization/projects/proj_1/groups HTTP/1.1");
    QTest::newRow("project group")
            << 12 << QByteArray("GET /v1/organization/projects/proj_1/groups/group_1 HTTP/1.1");
}

void TestRoles::rolePathsAreComposedFromTheScope()
{
    QFETCH(int, endpoint);
    QFETCH(QByteArray, line);

    StubServer server(QByteArray("{}"));
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));
    const RoleScope project = RoleScope::project(QStringLiteral("proj_1"));
    const QString projectId = QStringLiteral("proj_1");
    const QString groupId = QStringLiteral("group_1");
    const QString userId = QStringLiteral("user_1");
    const QString roleId = QStringLiteral("role_1");

    switch (endpoint) {
    case 0:
        QVERIFY(awaited(organization.listRoles()));
        break;
    case 1:
        QVERIFY(awaited(organization.listRoles(project)));
        break;
    case 2:
        QVERIFY(awaited(organization.getRole(roleId)));
        break;
    case 3:
        QVERIFY(awaited(organization.getRole(roleId, project)));
        break;
    case 4:
        QVERIFY(awaited(organization.listGroupRoles(groupId)));
        break;
    case 5:
        QVERIFY(awaited(organization.listGroupRoles(groupId, project)));
        break;
    case 6:
        QVERIFY(awaited(organization.listUserRoles(userId)));
        break;
    case 7:
        QVERIFY(awaited(organization.getUserRole(userId, roleId, project)));
        break;
    case 8:
        QVERIFY(awaited(organization.listGroups()));
        break;
    case 9:
        QVERIFY(awaited(organization.listGroupUsers(groupId)));
        break;
    case 10:
        QVERIFY(awaited(organization.getGroupUser(groupId, userId)));
        break;
    case 11:
        QVERIFY(awaited(organization.listProjectGroups(projectId)));
        break;
    default:
        QVERIFY(awaited(organization.getProjectGroup(projectId, groupId)));
        break;
    }

    QCOMPARE(server.requestLine(), line);
}

void TestRoles::createAGroupAndDeleteIt()
{
    StubServer list(groupPage());
    Organization reader(list.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto page = awaited(reader.listGroups());
    QVERIFY(page);
    QVERIFY2(page->isSuccess(), qPrintable(page->error().message()));
    QCOMPARE(page->groups().size(), 2);
    // The last page: `next` is null, which reads back as empty.
    QVERIFY(!page->groups().hasMore);
    QVERIFY(page->groups().next.isEmpty());
    QVERIFY(!page->groups().data.at(0).isScimManaged());
    // A group the identity provider owns: a membership change here is undone by
    // the next sync.
    QVERIFY(page->groups().data.at(1).isScimManaged());

    StubServer created(R"({"object":"group","id":"group_new","name":"Escalations",
        "created_at":1711471533,"is_scim_managed":false})");
    Organization organization(created.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.createGroup(QStringLiteral("Escalations")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(created.requestLine(), "POST /v1/organization/groups HTTP/1.1");
    QCOMPARE(created.requestBody(), R"({"name":"Escalations"})");
    QCOMPARE(reply->group().name(), QStringLiteral("Escalations"));

    // Unlike a project, a group really is deleted -- and the acknowledgement is
    // the same value type, as a project API key's is.
    StubServer removed(R"({"object":"group.deleted","id":"group_new","deleted":true})");
    Organization other(removed.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto gone = awaited(other.deleteGroup(QStringLiteral("group_new")));
    QVERIFY(gone);
    QVERIFY2(gone->isSuccess(), qPrintable(gone->error().message()));
    QCOMPARE(removed.requestLine(), "DELETE /v1/organization/groups/group_new HTTP/1.1");
    QVERIFY(gone->group().isDeleted());
    QCOMPARE(gone->group().id(), QStringLiteral("group_new"));
}

void TestRoles::addingAGroupMemberIsAcknowledgedNotReturned()
{
    StubServer server(R"({"object":"group.user","user_id":"user_ada","group_id":"group_support"})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(
            organization.addGroupUser(QStringLiteral("group_support"), QStringLiteral("user_ada")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    // The id goes in the body, not the path: this adds an existing organization
    // member to the group rather than creating a person -- the same shape as
    // adding someone to a project.
    QCOMPARE(server.requestLine(), "POST /v1/organization/groups/group_support/users HTTP/1.1");
    QCOMPARE(server.requestBody(), R"({"user_id":"user_ada"})");

    // And the answer is an acknowledgement, not the member: two ids and nothing
    // else, which is why it does not decode into a GroupMember with a missing
    // name.
    const GroupMembership membership = reply->membership();
    QCOMPARE(membership.userId(), QStringLiteral("user_ada"));
    QCOMPARE(membership.groupId(), QStringLiteral("group_support"));
    QVERIFY(!membership.isDeleted());

    StubServer removed(R"({"object":"group.user.deleted","deleted":true})");
    Organization other(removed.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto gone = awaited(
            other.removeGroupUser(QStringLiteral("group_support"), QStringLiteral("user_ada")));
    QVERIFY(gone);
    QVERIFY2(gone->isSuccess(), qPrintable(gone->error().message()));
    QCOMPARE(removed.requestLine(),
             "DELETE /v1/organization/groups/group_support/users/user_ada HTTP/1.1");
    QVERIFY(gone->membership().isDeleted());
    // Removal echoes no ids at all; the caller already has them.
    QVERIFY(gone->membership().userId().isEmpty());
}

void TestRoles::assignARoleAtBothScopes_data()
{
    QTest::addColumn<bool>("projectScoped");
    QTest::addColumn<QByteArray>("line");

    QTest::newRow("organization")
            << false << QByteArray("POST /v1/organization/groups/group_support/roles HTTP/1.1");
    QTest::newRow("project")
            << true << QByteArray("POST /v1/projects/proj_1/groups/group_support/roles HTTP/1.1");
}

void TestRoles::assignARoleAtBothScopes()
{
    QFETCH(bool, projectScoped);
    QFETCH(QByteArray, line);

    // The same request body and the same reply at both scopes -- which is the
    // whole reason RoleScope is a parameter rather than a second set of methods.
    StubServer server(R"({"object":"group.role",
        "group":{"object":"group","id":"group_support","name":"Support Team",
                 "created_at":1711471533,"scim_managed":false},
        "role":{"object":"role","id":"role_manager","name":"API Group Manager",
                "description":"Allows managing organization groups",
                "permissions":["api.groups.read"],"resource_type":"api.organization",
                "predefined_role":false}})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const RoleScope scope = projectScoped ? RoleScope::project(QStringLiteral("proj_1"))
                                          : RoleScope::organization();
    const auto reply = awaited(organization.assignGroupRole(QStringLiteral("group_support"),
                                                            QStringLiteral("role_manager"), scope));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    QCOMPARE(server.requestLine(), line);
    // The role id goes in the body; the path names the principal it is for.
    QCOMPARE(server.requestBody(), R"({"role_id":"role_manager"})");

    const RoleAssignment assignment = reply->assignment();
    QVERIFY(assignment.isGroup());
    QVERIFY(!assignment.isUser());
    QCOMPARE(assignment.principalName(), QStringLiteral("Support Team"));
    QCOMPARE(assignment.role().id(), QStringLiteral("role_manager"));
    // The embedded group spells the SCIM flag the short way; it still decodes.
    QVERIFY(!assignment.group().isScimManaged());
}

void TestRoles::anInheritedRoleReportsWhereItCameFrom()
{
    StubServer server(inheritedRolePage());
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.listUserRoles(QStringLiteral("user_ada")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(), "GET /v1/organization/users/user_ada/roles HTTP/1.1");

    const OrganizationRole role = reply->roles().data.value(0);
    QCOMPARE(role.name(), QStringLiteral("API Group Manager"));
    QCOMPARE(role.createdBy(), QStringLiteral("user_ada"));
    QCOMPARE(role.createdByUser().email(), QStringLiteral("ada@example.com"));

    // The field that answers the only interesting question about this role:
    // revoking it from the user does nothing, because the user never had it --
    // the group does.
    QVERIFY(role.isInherited());
    QCOMPARE(role.assignmentSources().size(), 1);
    QCOMPARE(role.assignmentSources().first().principalId, QStringLiteral("group_support"));
    QVERIFY(role.assignmentSources().first().isGroup());
}

void TestRoles::unassigningAnswersWithAlmostNothing()
{
    StubServer server(R"({"object":"user.role.deleted","deleted":true})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.unassignUserRole(
            QStringLiteral("user_ada"), QStringLiteral("role_manager"),
            RoleScope::project(QStringLiteral("proj_1"))));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));
    QCOMPARE(server.requestLine(),
             "DELETE /v1/projects/proj_1/users/user_ada/roles/role_manager HTTP/1.1");

    // No ids come back, not even the role's -- so the reply is the flag and the
    // object name, and both are readable rather than inferred from an empty
    // struct.
    const RoleAssignment assignment = reply->assignment();
    QVERIFY(assignment.isDeleted());
    QVERIFY(assignment.isUser());
    QVERIFY(assignment.role().id().isEmpty());
}

void TestRoles::grantingAGroupAccessToAProjectPutsBothIdsInTheBody()
{
    StubServer server(R"({"object":"project.group","project_id":"proj_1",
        "group_id":"group_support","group_name":"Support Team","group_type":"group",
        "created_at":1711471533})");
    Organization organization(server.baseUrl(), QStringLiteral("sk-admin-test"));

    const auto reply = awaited(organization.addProjectGroup(QStringLiteral("proj_1"),
                                                            QStringLiteral("group_support"),
                                                            QStringLiteral("role_project_member")));
    QVERIFY(reply);
    QVERIFY2(reply->isSuccess(), qPrintable(reply->error().message()));

    // Project groups sit under /organization/projects, where a project's roles
    // do not.
    QCOMPARE(server.requestLine(), "POST /v1/organization/projects/proj_1/groups HTTP/1.1");
    // `role` carries a role id rather than a role name, despite the short name.
    QCOMPARE(server.requestBody(), R"({"group_id":"group_support","role":"role_project_member"})");

    const ProjectGroup granted = reply->projectGroup();
    QCOMPARE(granted.groupName(), QStringLiteral("Support Team"));
    QCOMPARE(granted.projectId(), QStringLiteral("proj_1"));
    // The moment access was granted, not when the group was made.
    QCOMPARE(granted.createdAt(), qint64(1711471533));
    QCOMPARE(ProjectGroup::fromJson(granted.toJson()), granted);
}

QTEST_MAIN(TestRoles)
#include "tst_roles.moc"
