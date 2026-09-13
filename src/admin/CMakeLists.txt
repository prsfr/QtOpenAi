# ---------------------------------------------------------------------------
# QtOpenAi::Admin — the /organization administration surface.
#
# Its own module because it takes its own credential. The administration
# endpoints use an *admin* API key, which can archive a project or revoke a
# colleague's access, and putting them on Client::Client would have meant one
# object type carrying either that key or an ordinary one -- with nothing to
# stop the admin key from being sent to /chat/completions.
#
# Always built: it adds no Qt dependency of its own. It links Client for the
# request path (Client::planRequest/adoptReply) and the reply base types, and
# Core for the value types, and needs nothing else.
# ---------------------------------------------------------------------------

set(QTOPENAI_ADMIN_PUBLIC_HEADERS
    include/QtOpenAi/Admin/GlobalAdmin.h
    include/QtOpenAi/Admin/Organization.h
    include/QtOpenAi/Admin/ProjectReply.h
    include/QtOpenAi/Admin/ProjectServiceAccountReply.h
    include/QtOpenAi/Admin/SpendLimitReply.h
    include/QtOpenAi/Admin/SpendAlertReply.h
    include/QtOpenAi/Admin/AuditLogQuery.h
    include/QtOpenAi/Admin/AuditLogReply.h
    include/QtOpenAi/Admin/AdminApiKeyReply.h
    include/QtOpenAi/Admin/ProjectApiKeyReply.h
    include/QtOpenAi/Admin/ProjectRateLimitReply.h
    include/QtOpenAi/Admin/UsageQuery.h
    include/QtOpenAi/Admin/UsageReply.h
    include/QtOpenAi/Admin/CostsReply.h
    include/QtOpenAi/Admin/UserReply.h
    include/QtOpenAi/Admin/InviteReply.h
    include/QtOpenAi/Admin/RoleScope.h
    include/QtOpenAi/Admin/RoleReply.h
    include/QtOpenAi/Admin/GroupReply.h
    include/QtOpenAi/Admin/CertificateReply.h
    include/QtOpenAi/Admin/ProjectPermissionsReply.h
)

qtopenai_add_module(Admin
    HEADERS
        ${QTOPENAI_ADMIN_PUBLIC_HEADERS}
    SOURCES
        src/Organization.cpp
        src/AuditLogQuery.cpp
        src/UsageQuery.cpp
    LIBS
        QtOpenAi::Client
        QtOpenAi::Core
        Qt6::Core
)
