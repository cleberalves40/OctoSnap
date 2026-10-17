cmake_minimum_required(VERSION 3.21)
project(OctoSnap VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets LinguistTools)

qt_standard_project_setup()

# GUI application
qt_add_executable(octosnap
    src/main.cpp
    src/Theme.h
    src/Theme.cpp
    src/FssBackend.h
    src/FssBackend.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/RestoreDialog.h
    src/RestoreDialog.cpp
)

target_compile_options(octosnap PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
)
target_link_libraries(octosnap PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
# Single source of truth for the version: project(... VERSION ...)
target_compile_definitions(octosnap PRIVATE OCTOSNAP_VERSION="${PROJECT_VERSION}")

qt_add_translations(octosnap
    TS_FILES
        translations/octosnap_en.ts
        translations/octosnap_pt_BR.ts
    RESOURCE_PREFIX /i18n
)

# Privileged helper: no Qt, no shell, fixed command set
if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(BUILD_HELPER_DEFAULT ON)
else()
    set(BUILD_HELPER_DEFAULT OFF)
endif()
option(BUILD_HELPER "Build privileged octosnap-helper" ${BUILD_HELPER_DEFAULT})

if(BUILD_HELPER)
    add_executable(octosnap-helper helper/octosnap-helper.cpp)
    target_compile_options(octosnap-helper PRIVATE -Wall -Wextra -Wpedantic)
    install(TARGETS octosnap-helper RUNTIME DESTINATION libexec)
endif()

install(TARGETS octosnap RUNTIME DESTINATION bin)
install(FILES octosnap.desktop DESTINATION share/applications)
install(FILES config/doas.conf.example config/sudoers.d-octosnap
        DESTINATION share/examples/octosnap)
