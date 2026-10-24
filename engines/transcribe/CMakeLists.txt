cmake_minimum_required(VERSION 3.24)
project(ducktor_engine_transcribe LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(WHISPER_CURL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(whisper
    GIT_REPOSITORY https://github.com/ggml-org/whisper.cpp.git
    GIT_TAG v1.9.5
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(whisper)

if(DUCKTOR_LICENSE_OUTPUT_DIR)
    configure_file(${whisper_SOURCE_DIR}/LICENSE ${DUCKTOR_LICENSE_OUTPUT_DIR}/whisper.cpp-LICENSE COPYONLY)
endif()

add_executable(ducktor-transcribe main.cpp)
target_include_directories(ducktor-transcribe PRIVATE ${DUCKTOR_COMMON_DIR})
target_link_libraries(ducktor-transcribe PRIVATE whisper)
set_target_properties(ducktor-transcribe PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DUCKTOR_ENGINE_OUTPUT_DIR})
