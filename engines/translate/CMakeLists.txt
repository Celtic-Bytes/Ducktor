cmake_minimum_required(VERSION 3.24)
project(ducktor_engine_translate LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_COMMON OFF CACHE BOOL "" FORCE)
set(LLAMA_CURL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(llama
    GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
    GIT_TAG v0.7.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(llama)

if(DUCKTOR_LICENSE_OUTPUT_DIR)
    configure_file(${llama_SOURCE_DIR}/LICENSE ${DUCKTOR_LICENSE_OUTPUT_DIR}/llama.cpp-LICENSE COPYONLY)
endif()

add_executable(ducktor-translate main.cpp)
target_include_directories(ducktor-translate PRIVATE ${DUCKTOR_COMMON_DIR})
target_link_libraries(ducktor-translate PRIVATE llama)
set_target_properties(ducktor-translate PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DUCKTOR_ENGINE_OUTPUT_DIR})
