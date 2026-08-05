# Standalone consumer project: verifies QtOpenAi is usable via
# find_package(QtOpenAi) with the installed (or exported) package.
# Configured separately from the main build with CMAKE_PREFIX_PATH pointing at
# the QtOpenAi install prefix.
cmake_minimum_required(VERSION 3.21)
project(qtopenai_package_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QtOpenAi REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer
    PRIVATE QtOpenAi::Client QtOpenAi::Chat QtOpenAi::Tools QtOpenAi::Storage QtOpenAi::Admin)

# QtOpenAi::Realtime is optional (it needs Qt6::WebSockets), so the consumer
# exercises it only when the installed package actually carries it.
if(TARGET QtOpenAi::Realtime)
    target_link_libraries(consumer PRIVATE QtOpenAi::Realtime)
    target_compile_definitions(consumer PRIVATE QTOPENAI_CONSUMER_HAS_REALTIME)
endif()

# The same for QtOpenAi::Sql, which needs Qt6::Sql.
if(TARGET QtOpenAi::Sql)
    target_link_libraries(consumer PRIVATE QtOpenAi::Sql)
    target_compile_definitions(consumer PRIVATE QTOPENAI_CONSUMER_HAS_SQL)
endif()

# The other half of the headless guarantee. QtOpenAi must not *depend* on a GUI
# stack -- but "does not depend on" would be worthless if it also meant "does
# not work with", and a Widgets or Qt Quick application is where most callers
# will be. So a second consumer links the library into a real QApplication and
# proves the combination builds, links and runs.
#
# Built only where the GUI packages exist, since the library itself never
# requires them: their absence is not a failure, it is the normal case for a
# headless build.
find_package(Qt6 6.4 QUIET COMPONENTS Widgets)
if(Qt6Widgets_FOUND)
    add_executable(gui_consumer gui_main.cpp)
    target_link_libraries(gui_consumer PRIVATE QtOpenAi::Client QtOpenAi::Chat Qt6::Widgets)

    find_package(Qt6 6.4 QUIET COMPONENTS Qml)
    if(Qt6Qml_FOUND)
        target_link_libraries(gui_consumer PRIVATE Qt6::Qml)
        target_compile_definitions(gui_consumer PRIVATE QTOPENAI_CONSUMER_HAS_QML)
    endif()
else()
    message(STATUS "Qt6::Widgets not found -- skipping the GUI consumer.")
endif()
