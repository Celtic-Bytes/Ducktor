cmake_minimum_required(VERSION 3.24)
project(ducktor_engine_spellcheck LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Pinned release; the official CMake builds the library only (no command-line tools).
FetchContent_Declare(hunspell
    GIT_REPOSITORY https://github.com/hunspell/hunspell.git
    GIT_TAG v1.7.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(hunspell)

# Tri-licensed MPL-1.1/GPL-2.0+/LGPL-2.1+; Ducktor takes the LGPL branch.
if(DUCKTOR_LICENSE_OUTPUT_DIR)
    configure_file(${hunspell_SOURCE_DIR}/COPYING.LESSER ${DUCKTOR_LICENSE_OUTPUT_DIR}/hunspell-LICENSE COPYONLY)
endif()

add_executable(ducktor-spellcheck main.cpp)
target_include_directories(ducktor-spellcheck PRIVATE ${DUCKTOR_COMMON_DIR})
target_link_libraries(ducktor-spellcheck PRIVATE hunspell)
set_target_properties(ducktor-spellcheck PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DUCKTOR_ENGINE_OUTPUT_DIR})
