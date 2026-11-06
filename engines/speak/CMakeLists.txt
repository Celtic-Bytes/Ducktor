cmake_minimum_required(VERSION 3.24)
project(ducktor_engine_speak LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

find_path(PIPER_INCLUDE_DIR piper.h PATHS ${PIPER_ROOT}/include NO_DEFAULT_PATH REQUIRED)
find_library(PIPER_LIBRARY NAMES piper libpiper PATHS ${PIPER_ROOT}/lib NO_DEFAULT_PATH REQUIRED)

add_executable(ducktor-speak main.cpp)
target_include_directories(ducktor-speak PRIVATE ${DUCKTOR_COMMON_DIR} ${PIPER_INCLUDE_DIR})
target_link_libraries(ducktor-speak PRIVATE ${PIPER_LIBRARY})
set_target_properties(ducktor-speak PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DUCKTOR_ENGINE_OUTPUT_DIR})
if(APPLE)
    set_target_properties(ducktor-speak PROPERTIES BUILD_RPATH "@loader_path")
elseif(UNIX)
    set_target_properties(ducktor-speak PROPERTIES
        BUILD_RPATH "$ORIGIN"
        # CMake would also add libpiper's absolute build directory to the build
        # tree's rpath, and the install is a plain copy, so that path would
        # ship inside every package (Fedora's check-rpaths rejects it). Using
        # the install rpath in the build tree keeps it exactly $ORIGIN, which
        # is all the helper needs: its libraries are copied next to it.
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE)
    # DT_RPATH (unlike RUNPATH) also lets libpiper find ONNX Runtime next to the helper.
    target_link_options(ducktor-speak PRIVATE "LINKER:--disable-new-dtags")
endif()

# libpiper, ONNX Runtime and the espeak-ng data must sit next to the helper at runtime.
# libpiper installs to lib on most systems and lib64 on RPM-based ones, so take
# the directory of the library that was actually found.
get_filename_component(_piper_libdir ${PIPER_LIBRARY} DIRECTORY)
file(GLOB PIPER_RUNTIME_LIBS
    ${PIPER_ROOT}/bin/*.dll
    ${_piper_libdir}/*.so*
    ${_piper_libdir}/*.dylib)
foreach(lib IN LISTS PIPER_RUNTIME_LIBS)
    add_custom_command(TARGET ducktor-speak POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${lib} ${DUCKTOR_ENGINE_OUTPUT_DIR})
endforeach()
add_custom_command(TARGET ducktor-speak POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PIPER_ROOT}/share/espeak-ng-data ${DUCKTOR_ENGINE_OUTPUT_DIR}/espeak-ng-data)

# Licenses of libpiper and of the eSpeak NG and ONNX Runtime code shipped with it.
if(DUCKTOR_LICENSE_OUTPUT_DIR)
    set(_licenses
        "${PIPER_SOURCE_DIR}/COPYING|piper-COPYING"
        "${PIPER_BINARY_DIR}/espeak_ng/src/espeak_ng_external/COPYING|espeak-ng-COPYING")
    file(GLOB _onnxruntime_dirs LIST_DIRECTORIES true ${PIPER_SOURCE_DIR}/libpiper/lib/onnxruntime-*)
    foreach(dir IN LISTS _onnxruntime_dirs)
        list(APPEND _licenses
            "${dir}/LICENSE|onnxruntime-LICENSE"
            "${dir}/ThirdPartyNotices.txt|onnxruntime-ThirdPartyNotices.txt")
    endforeach()
    foreach(entry IN LISTS _licenses)
        string(REPLACE "|" ";" pair "${entry}")
        list(GET pair 0 source)
        list(GET pair 1 target)
        if(EXISTS "${source}")
            configure_file("${source}" "${DUCKTOR_LICENSE_OUTPUT_DIR}/${target}" COPYONLY)
        else()
            message(WARNING "License file not found: ${source}")
        endif()
    endforeach()
endif()
