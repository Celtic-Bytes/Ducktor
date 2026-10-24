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
    set_target_properties(ducktor-speak PROPERTIES BUILD_RPATH "$ORIGIN")
endif()

# libpiper, ONNX Runtime and the espeak-ng data must sit next to the helper at runtime.
file(GLOB PIPER_RUNTIME_LIBS
    ${PIPER_ROOT}/bin/*.dll
    ${PIPER_ROOT}/lib/*.so*
    ${PIPER_ROOT}/lib/*.dylib)
foreach(lib IN LISTS PIPER_RUNTIME_LIBS)
    add_custom_command(TARGET ducktor-speak POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${lib} ${DUCKTOR_ENGINE_OUTPUT_DIR})
endforeach()
add_custom_command(TARGET ducktor-speak POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PIPER_ROOT}/share/espeak-ng-data ${DUCKTOR_ENGINE_OUTPUT_DIR}/espeak-ng-data)
