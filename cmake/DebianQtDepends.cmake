# Qt plugins and QML modules are loaded at runtime, so dpkg-shlibdeps cannot see them. Ask dpkg which
# installed packages provide the ones Ducktor needs and add those packages to the .deb dependencies.
find_program(DUCKTOR_DPKG dpkg)
if(NOT DUCKTOR_DPKG OR NOT QT6_INSTALL_PREFIX)
    return()
endif()

set(_plugins ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS})
set(_qml ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML})
file(GLOB _runtime_files
    ${_plugins}/imageformats/libqsvg.so
    ${_plugins}/multimedia/*.so
    ${_plugins}/platforms/libqxcb.so
    ${_plugins}/platforms/libqwayland*.so)
foreach(module IN ITEMS
        QtQuick QtQuick/Controls QtQuick/Controls/Fusion QtQuick/Layouts QtQuick/Templates QtQuick/Window
        QtQuick/Dialogs QtQuick/Dialogs/quickimpl QtQml/WorkerScript Qt/labs/folderlistmodel)
    if(EXISTS ${_qml}/${module}/qmldir)
        list(APPEND _runtime_files ${_qml}/${module}/qmldir)
    endif()
endforeach()

set(_packages)
foreach(path IN LISTS _runtime_files)
    execute_process(COMMAND ${DUCKTOR_DPKG} -S ${path}
        OUTPUT_VARIABLE _owner RESULT_VARIABLE _result ERROR_QUIET)
    if(_result EQUAL 0)
        string(REGEX MATCH "^[^:,]+" _package "${_owner}")
        list(APPEND _packages ${_package})
    endif()
endforeach()

if(_packages)
    list(REMOVE_DUPLICATES _packages)
    list(SORT _packages)
    list(JOIN _packages ", " CPACK_DEBIAN_PACKAGE_DEPENDS)
    message(STATUS "Extra .deb dependencies: ${CPACK_DEBIAN_PACKAGE_DEPENDS}")
else()
    message(STATUS "Qt is not from dpkg packages: the .deb will only list the libraries found by dpkg-shlibdeps.")
endif()
