# CMAKE_PROJECT_INCLUDE_BEFORE file for the nested espeak-ng project that
# piper builds. It is injected into that project's CMAKE_ARGS by
# cmake/piper-espeak-ng-compat.patch (applied to piper v1.8.0 at build time)
# and runs before project(), so the compile options below reach every target
# of the nested project and nothing else.
#
# On MSVC the nested project force-includes espeak-ng-msvc-compat.h: the
# espeak-ng shims of the pinned commit probe the C++23 __has_include_next
# operator, which MSVC does not implement and MSVC 14.52 (Visual Studio
# 2026) rejects with errors C7780 and C1017 (see that header for the full
# rationale). The fallback macro must come from a forced-include header
# because MSVC's /D switch does not support function-like macro
# definitions.
#
# The compiler ids are evaluated at generation time, which is why this file
# works although CMAKE_PROJECT_INCLUDE_BEFORE runs before project() has
# detected the compilers. Compilers that implement the operator (GCC,
# Clang) match neither branch, so their builds are unchanged.
add_compile_options(
    "$<$<AND:$<COMPILE_LANGUAGE:C>,$<C_COMPILER_ID:MSVC>>:/FI${CMAKE_CURRENT_LIST_DIR}/espeak-ng-msvc-compat.h>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/FI${CMAKE_CURRENT_LIST_DIR}/espeak-ng-msvc-compat.h>")
