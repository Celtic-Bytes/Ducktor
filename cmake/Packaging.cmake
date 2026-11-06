# Package formats made with CPack: .deb and .tar.xz on Linux (an RPM instead,
# with DUCKTOR_PACKAGE_RPM), ZIP on Windows and macOS. AppImage and Flatpak
# have their own tools; see packaging/ and the README.
set(CPACK_PACKAGE_NAME ducktor)
set(CPACK_PACKAGE_VENDOR celticbytes)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Offline translator for text and voice")
set(CPACK_PACKAGE_HOMEPAGE_URL https://celticbytes.com)
set(CPACK_PACKAGE_CONTACT "celticbytes <info@celticbytes.com>")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CPACK_STRIP_FILES ON)
set(CPACK_VERBATIM_VARIABLES ON)

if(WIN32)
    set(CPACK_GENERATOR ZIP)
    set(CPACK_PACKAGE_FILE_NAME Ducktor-${PROJECT_VERSION}-windows-x64)
elseif(APPLE)
    set(CPACK_GENERATOR ZIP)
    set(CPACK_PACKAGE_FILE_NAME Ducktor-${PROJECT_VERSION}-macos-${CMAKE_SYSTEM_PROCESSOR})
else()
    set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
    if(DUCKTOR_PACKAGE_RPM)
        # The RPM targets the Fedora family: the system Qt comes from the standard
        # qt6-* packages, and the engines' own libraries (ONNX Runtime, Piper,
        # espeak-ng) are private to libexec/ducktor, so their dependencies are kept
        # out of the automatic requires.
        set(CPACK_GENERATOR RPM)
        set(CPACK_RPM_PACKAGE_LICENSE GPL-3.0-or-later)
        set(CPACK_RPM_REQUIRES_EXCLUDE_FROM
            ${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/ducktor/.* )
        include(${CMAKE_CURRENT_LIST_DIR}/FedoraQtDepends.cmake)
    else()
        set(CPACK_GENERATOR DEB TXZ)
        # Name of the tarball; the .deb keeps the standard Debian name (DEB-DEFAULT).
        set(CPACK_PACKAGE_FILE_NAME ducktor-${PROJECT_VERSION}-linux-x86_64)
        set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
        set(CPACK_DEBIAN_PACKAGE_SECTION utils)
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
            "Offline translator for text and voice\n Translates, transcribes and reads aloud on this computer with local AI\n models. It has no account, telemetry or history.")
        # Libraries dpkg-shlibdeps finds for the executables (Qt, libc...) become dependencies automatically.
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS ${DUCKTOR_RUNTIME_DIR})
        include(${CMAKE_CURRENT_LIST_DIR}/DebianQtDepends.cmake)
    endif()
endif()

include(CPack)
