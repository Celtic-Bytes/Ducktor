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
    -DDUCKTOR_LICENSE_OUTPUT_DIR=${DUCKTOR_LICENSE_DIR}
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
endif()

if(DUCKTOR_ENGINE_TRANSCRIBE)
    ExternalProject_Add(ducktor_engine_transcribe
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/engines/transcribe
        BINARY_DIR ${CMAKE_BINARY_DIR}/engines/transcribe
        CMAKE_ARGS ${_engine_args}
        BUILD_ALWAYS ON
        INSTALL_COMMAND "")
endif()

if(DUCKTOR_ENGINE_SPELLCHECK)
    ExternalProject_Add(ducktor_engine_spellcheck
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/engines/spellcheck
        BINARY_DIR ${CMAKE_BINARY_DIR}/engines/spellcheck
        CMAKE_ARGS ${_engine_args}
        BUILD_ALWAYS ON
        INSTALL_COMMAND "")
    # The bundled dictionaries sit next to the helper at runtime, in the dev tree and in packages.
    add_custom_command(TARGET ducktor_engine_spellcheck POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources/spell ${DUCKTOR_RUNTIME_DIR}/spell)
endif()

if(DUCKTOR_ENGINE_SPEAK)
    # libpiper downloads ONNX Runtime from a GitHub release without checking whether the
    # download succeeded. Microsoft removed the microsoft/onnxruntime repository and its
    # release assets, so that download now yields a 404 page. Use the official NuGet
    # package instead: it is immutable, covers every platform we package and can be
    # verified with a hash. The prepared directory mirrors the layout libpiper expects
    # from its own download.
    set(DUCKTOR_ONNXRUNTIME_VERSION 1.22.0)
    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(_onnxruntime_rid win-x86)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
            set(_onnxruntime_rid win-arm64)
        else()
            set(_onnxruntime_rid win-x64)
        endif()
    elseif(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
            set(_onnxruntime_rid osx-arm64)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
            set(_onnxruntime_rid osx-x64)
        else()
            message(FATAL_ERROR
                "Unsupported architecture for ONNX Runtime: ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
    else()
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
            set(_onnxruntime_rid linux-arm64)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
            set(_onnxruntime_rid linux-x64)
        else()
            message(FATAL_ERROR
                "Unsupported architecture for ONNX Runtime: ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
    endif()

    set(DUCKTOR_ONNXRUNTIME_SOURCE_DIR ${CMAKE_BINARY_DIR}/onnxruntime-src)
    set(DUCKTOR_ONNXRUNTIME_DIR ${CMAKE_BINARY_DIR}/onnxruntime)

    # The NuGet binaries keep the SONAME / install name of the official archives but do
    # not ship the versioned file names the loader asks for. Link against the plain name
    # and expose the name recorded in libpiper: libonnxruntime.so.1 on Linux and
    # @rpath/libonnxruntime.<version>.dylib on macOS.
    set(_onnxruntime_versioned_name)
    if(APPLE)
        set(_onnxruntime_dylib libonnxruntime.${DUCKTOR_ONNXRUNTIME_VERSION}.dylib)
        list(APPEND _onnxruntime_versioned_name
            COMMAND ${CMAKE_COMMAND} -E create_symlink libonnxruntime.dylib
                ${DUCKTOR_ONNXRUNTIME_DIR}/lib/${_onnxruntime_dylib})
    elseif(UNIX)
        list(APPEND _onnxruntime_versioned_name
            COMMAND ${CMAKE_COMMAND} -E create_symlink libonnxruntime.so
                ${DUCKTOR_ONNXRUNTIME_DIR}/lib/libonnxruntime.so.1)
    endif()

    # Give libpiper the library path explicitly so a system ONNX Runtime of another
    # version can never be picked up instead of the one prepared here.
    if(WIN32)
        set(_onnxruntime_library ${DUCKTOR_ONNXRUNTIME_DIR}/lib/onnxruntime.lib)
    elseif(APPLE)
        set(_onnxruntime_library ${DUCKTOR_ONNXRUNTIME_DIR}/lib/libonnxruntime.dylib)
    else()
        set(_onnxruntime_library ${DUCKTOR_ONNXRUNTIME_DIR}/lib/libonnxruntime.so)
    endif()

    ExternalProject_Add(ducktor_onnxruntime
        URL https://www.nuget.org/api/v2/package/Microsoft.ML.OnnxRuntime/${DUCKTOR_ONNXRUNTIME_VERSION}
        DOWNLOAD_NAME Microsoft.ML.OnnxRuntime.${DUCKTOR_ONNXRUNTIME_VERSION}.zip
        URL_HASH SHA256=d571e63a2329baacb713f441e65ad75284de354db6e1ac435fe4bebbb417986a
        DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
        SOURCE_DIR ${DUCKTOR_ONNXRUNTIME_SOURCE_DIR}
        CONFIGURE_COMMAND
            ${CMAKE_COMMAND} -E rm -rf ${DUCKTOR_ONNXRUNTIME_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory
                ${DUCKTOR_ONNXRUNTIME_DIR}/include
                ${DUCKTOR_ONNXRUNTIME_DIR}/lib
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${DUCKTOR_ONNXRUNTIME_SOURCE_DIR}/build/native/include
                ${DUCKTOR_ONNXRUNTIME_DIR}/include
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${DUCKTOR_ONNXRUNTIME_SOURCE_DIR}/runtimes/${_onnxruntime_rid}/native
                ${DUCKTOR_ONNXRUNTIME_DIR}/lib
            ${_onnxruntime_versioned_name}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DUCKTOR_ONNXRUNTIME_SOURCE_DIR}/LICENSE
                ${DUCKTOR_ONNXRUNTIME_DIR}/LICENSE
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DUCKTOR_ONNXRUNTIME_SOURCE_DIR}/ThirdPartyNotices.txt
                ${DUCKTOR_ONNXRUNTIME_DIR}/ThirdPartyNotices.txt
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_COMMAND "")

    # Short paths: eSpeak NG truncates long file paths while compiling its phoneme data.
    ExternalProject_Add(ducktor_libpiper
        GIT_REPOSITORY https://github.com/OHF-Voice/piper1-gpl.git
        GIT_TAG v1.8.0
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR libpiper
        UPDATE_DISCONNECTED TRUE
        PREFIX ${CMAKE_BINARY_DIR}/pp
        SOURCE_DIR ${CMAKE_BINARY_DIR}/pp/s
        BINARY_DIR ${CMAKE_BINARY_DIR}/pp/b
        DEPENDS ducktor_onnxruntime
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_INSTALL_PREFIX=${DUCKTOR_ENGINE_PREFIX}/piper
            -DCMAKE_DISABLE_FIND_PACKAGE_onnxruntime=TRUE
            -DONNXRUNTIME_DIR=${DUCKTOR_ONNXRUNTIME_DIR}
            -DONNXRUNTIME_LIB=${_onnxruntime_library})

    ExternalProject_Add(ducktor_engine_speak
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/engines/speak
        BINARY_DIR ${CMAKE_BINARY_DIR}/engines/speak
        CMAKE_ARGS ${_engine_args}
            -DPIPER_ROOT=${DUCKTOR_ENGINE_PREFIX}/piper
            -DPIPER_SOURCE_DIR=${CMAKE_BINARY_DIR}/pp/s
            -DPIPER_BINARY_DIR=${CMAKE_BINARY_DIR}/pp/b
            -DONNXRUNTIME_DIR=${DUCKTOR_ONNXRUNTIME_DIR}
        DEPENDS ducktor_libpiper
        BUILD_ALWAYS ON
        INSTALL_COMMAND "")
endif()
