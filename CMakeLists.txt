cmake_minimum_required(VERSION 3.16)

project(clipboard_toolbox VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Network Concurrent)

qt_standard_project_setup()

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

qt_add_executable(appclipboard_toolbox
    ${SOURCES}
    resources.qrc
    clipboard_toolbox.rc
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appclipboard_toolbox PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appclipboard_toolbox
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appclipboard_toolbox
    PRIVATE Qt6::Widgets Qt6::Network Qt6::Concurrent
)

target_include_directories(appclipboard_toolbox PRIVATE src)

include(GNUInstallDirs)
install(TARGETS appclipboard_toolbox
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
