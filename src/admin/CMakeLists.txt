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
    include/QtOpenAi/Admin/ProjectListReply.h
)

add_library(QtOpenAiAdmin
    ${QTOPENAI_ADMIN_PUBLIC_HEADERS}
    src/Organization.cpp
)

add_library(QtOpenAi::Admin ALIAS QtOpenAiAdmin)

target_link_libraries(QtOpenAiAdmin
    PUBLIC
        QtOpenAi::Client
        QtOpenAi::Core
        Qt6::Core
)

target_include_directories(QtOpenAiAdmin
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(QtOpenAiAdmin PRIVATE QTOPENAI_ADMIN_LIBRARY)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(QtOpenAiAdmin PUBLIC QTOPENAI_ADMIN_STATIC)
endif()

set_target_properties(QtOpenAiAdmin PROPERTIES
    OUTPUT_NAME QtOpenAiAdmin
    EXPORT_NAME Admin
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(QTOPENAI_INSTALL)
    install(TARGETS QtOpenAiAdmin
        EXPORT QtOpenAiTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    # Preserves the QtOpenAi/Admin/... include layout.
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
