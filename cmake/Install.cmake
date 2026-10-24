# Install layout. Linux follows the FHS (bin, libexec, share); the Windows ZIP keeps everything
# next to ducktor.exe; macOS puts the engines inside Ducktor.app. src/core/Paths.cpp looks in these places.
include(GNUInstallDirs)

if(WIN32)
    set(CMAKE_INSTALL_BINDIR .)
    set(DUCKTOR_INSTALL_ENGINES .)
    set(DUCKTOR_INSTALL_DOCS .)
elseif(APPLE)
    set(DUCKTOR_INSTALL_ENGINES Ducktor.app/Contents/MacOS)
    set(DUCKTOR_INSTALL_DOCS .)
else()
    set(DUCKTOR_INSTALL_ENGINES ${CMAKE_INSTALL_LIBEXECDIR}/ducktor)
    set(DUCKTOR_INSTALL_DOCS ${CMAKE_INSTALL_DATAROOTDIR}/doc/ducktor)
endif()

install(TARGETS ducktor
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(DUCKTOR_BUILD_ENGINES)
    foreach(engine IN ITEMS TRANSLATE TRANSCRIBE SPEAK)
        if(DUCKTOR_ENGINE_${engine})
            string(TOLOWER ${engine} name)
            install(PROGRAMS ${DUCKTOR_RUNTIME_DIR}/ducktor-${name}${CMAKE_EXECUTABLE_SUFFIX}
                DESTINATION ${DUCKTOR_INSTALL_ENGINES})
        endif()
    endforeach()
    if(DUCKTOR_ENGINE_SPEAK)
        # libpiper and ONNX Runtime, found by ducktor-speak through its $ORIGIN / @loader_path rpath.
        install(DIRECTORY ${DUCKTOR_RUNTIME_DIR}/
            DESTINATION ${DUCKTOR_INSTALL_ENGINES}
            USE_SOURCE_PERMISSIONS
            FILES_MATCHING
                PATTERN "*.so*"
                PATTERN "*.dylib"
                PATTERN "*.dll"
                PATTERN "espeak-ng-data" EXCLUDE)
        install(DIRECTORY ${DUCKTOR_RUNTIME_DIR}/espeak-ng-data DESTINATION ${DUCKTOR_INSTALL_ENGINES})
    endif()
endif()

install(FILES LICENSE PRIVACY.md THIRD_PARTY_NOTICES.md DESTINATION ${DUCKTOR_INSTALL_DOCS})
install(FILES resources/flags/LICENSE DESTINATION ${DUCKTOR_INSTALL_DOCS}/licenses RENAME flag-icons-LICENSE)
install(FILES packaging/licenses/LGPL-3.0-only.txt packaging/licenses/LGPL-2.1-or-later.txt
    DESTINATION ${DUCKTOR_INSTALL_DOCS}/licenses)
install(DIRECTORY ${DUCKTOR_LICENSE_DIR}/ DESTINATION ${DUCKTOR_INSTALL_DOCS}/licenses)

if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/com.celticbytes.Ducktor.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES packaging/linux/com.celticbytes.Ducktor.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)
    install(FILES resources/icons/ducktor.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
        RENAME com.celticbytes.Ducktor.svg)
endif()

# Windows and macOS packages carry their own Qt; Linux uses the system Qt, the AppImage tools or the Flatpak runtime.
if(WIN32 OR APPLE)
    qt_generate_deploy_qml_app_script(
        TARGET ducktor
        OUTPUT_SCRIPT ducktor_deploy_script)
    install(SCRIPT ${ducktor_deploy_script})
endif()

if(WIN32)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    include(InstallRequiredSystemLibraries)
    install(FILES packaging/windows/portable.txt DESTINATION .)
endif()

if(APPLE)
    # Deployment rewrites the binaries, so Apple Silicon needs a fresh (ad hoc) signature to run them.
    install(CODE "execute_process(COMMAND codesign --force --deep --sign - \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/Ducktor.app\" COMMAND_ERROR_IS_FATAL ANY)")
endif()
