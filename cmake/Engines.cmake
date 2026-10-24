# Each engine is an independent CMake project: llama.cpp and whisper.cpp both vendor ggml,
# so building them in one project would produce clashing targets.
include(ExternalProject)

set(DUCKTOR_ENGINE_PREFIX ${CMAKE_BINARY_DIR}/engine-prefix)

set(_engine_args
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DDUCKTOR_COMMON_DIR=${CMAKE_SOURCE_DIR}/engines/common
    -DDUCKTOR_ENGINE_OUTPUT_DIR=${DUCKTOR_RUNTIME_DIR}
    -DGGML_NATIVE=${DUCKTOR_NATIVE_CPU})
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    list(APPEND _engine_args -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

if(DUCKTOR_ENGINE_TRANSLATE)
    ExternalProject_Add(ducktor_engine_translate
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/engines/translate
        BINARY_DIR ${CMAKE_BINARY_DIR}/engines/translate
        CMAKE_ARGS ${_engine_args}
        BUILD_ALWAYS ON
        INSTALL_COMMAND "")
    add_dependencies(ducktor ducktor_engine_translate)
endif()

if(DUCKTOR_ENGINE_TRANSCRIBE)
    ExternalProject_Add(ducktor_engine_transcribe
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/engines/transcribe
        BINARY_DIR ${CMAKE_BINARY_DIR}/engines/transcribe
        CMAKE_ARGS ${_engine_args}
        BUILD_ALWAYS ON
        INSTALL_COMMAND "")
    add_dependencies(ducktor ducktor_engine_transcribe)
endif()

if(DUCKTOR_ENGINE_SPEAK)
    ExternalProject_Add(ducktor_libpiper
        GIT_REPOSITORY https://github.com/OHF-Voice/piper1-gpl.git
        GIT_TAG v1.8.0
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR libpiper
        UPDATE_DISCONNECTED TRUE
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_INSTALL_PREFIX=${DUCKTOR_ENGINE_PREFIX}/piper)

    ExternalProject_Add(ducktor_engine_speak
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/engines/speak
        BINARY_DIR ${CMAKE_BINARY_DIR}/engines/speak
        CMAKE_ARGS ${_engine_args} -DPIPER_ROOT=${DUCKTOR_ENGINE_PREFIX}/piper
        DEPENDS ducktor_libpiper
        BUILD_ALWAYS ON
        INSTALL_COMMAND "")
    add_dependencies(ducktor ducktor_engine_speak)
endif()
