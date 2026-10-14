cmake_minimum_required(VERSION 3.21)
project(OctoSnap VERSION 0.1.0 LANGUAGES CXX)

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
)

target_link_libraries(octosnap PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

qt_add_translations(octosnap
    TS_FILES
        translations/octosnap_en.ts
        translations/octosnap_pt_BR.ts
    RESOURCE_PREFIX /i18n
)

# Privileged helper: no Qt, no shell, fixed command set
add_executable(octosnap-helper helper/octosnap-helper.cpp)
target_compile_options(octosnap-helper PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS octosnap RUNTIME DESTINATION bin)
install(TARGETS octosnap-helper RUNTIME DESTINATION libexec)
install(FILES octosnap.desktop DESTINATION share/applications)
install(FILES config/doas.conf.example config/sudoers.d-octosnap
        DESTINATION share/examples/octosnap)
