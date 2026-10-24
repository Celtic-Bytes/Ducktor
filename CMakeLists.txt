cmake_minimum_required(VERSION 3.24)

project(Ducktor VERSION 0.1.0 DESCRIPTION "Offline desktop translator" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

option(DUCKTOR_BUILD_ENGINES "Build the local inference helper programs" ON)
option(DUCKTOR_ENGINE_TRANSLATE "Build the llama.cpp translation engine" ON)
option(DUCKTOR_ENGINE_TRANSCRIBE "Build the whisper.cpp speech recognition engine" ON)
option(DUCKTOR_ENGINE_SPEAK "Build the Piper speech synthesis engine" ON)
option(DUCKTOR_NATIVE_CPU "Optimize engines for this machine only (disable for distributable builds)" ON)
option(DUCKTOR_BUILD_TESTS "Build the test suite" ON)

set(DUCKTOR_RUNTIME_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DUCKTOR_RUNTIME_DIR})

set(_qt_components Core Gui Network Multimedia Qml Quick QuickControls2 Svg)
if(DUCKTOR_BUILD_TESTS)
    list(APPEND _qt_components Test)
endif()
find_package(Qt6 6.9 REQUIRED COMPONENTS ${_qt_components})
find_package(Qt6 6.9 QUIET OPTIONAL_COMPONENTS LinguistTools)
qt_standard_project_setup(REQUIRES 6.9)

add_subdirectory(src)

qt_add_executable(ducktor WIN32 MACOSX_BUNDLE src/main.cpp)
target_compile_definitions(ducktor PRIVATE DUCKTOR_VERSION="${PROJECT_VERSION}")
target_link_libraries(ducktor PRIVATE ducktor_core Qt6::Quick Qt6::QuickControls2 Qt6::Svg)
set_target_properties(ducktor PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME Ducktor
    MACOSX_BUNDLE_GUI_IDENTIFIER com.celticbytes.Ducktor
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})

qt_add_qml_module(ducktor
    URI Ducktor
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/TranslatorPage.qml
        qml/LimitedTextArea.qml
        qml/SettingsPage.qml
        qml/ModelSection.qml
        qml/ModelCard.qml
        qml/AboutPage.qml)

qt_add_resources(ducktor ducktor_assets
    PREFIX /
    FILES
        resources/models.json
        resources/icons/ducktor.svg)

# The repository documents are the single source shown in the About page.
qt_add_resources(ducktor ducktor_docs
    PREFIX /docs
    FILES
        LICENSE
        PRIVACY.md
        THIRD_PARTY_NOTICES.md)

if(Qt6LinguistTools_FOUND)
    qt_add_translations(ducktor TS_FILES i18n/ducktor_es.ts)
endif()

if(DUCKTOR_BUILD_ENGINES)
    include(cmake/Engines.cmake)
endif()

if(DUCKTOR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS ducktor
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
