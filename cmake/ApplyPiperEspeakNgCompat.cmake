# Applies cmake/piper-espeak-ng-compat.patch to the piper v1.8.0 checkout
# that ExternalProject downloaded for the speak engine (see
# ApplyPiperEspeakNgCompat.cmake usage in cmake/Engines.cmake).
#
# The patch adds one line to the nested espeak-ng project's CMAKE_ARGS so
# Ducktor can hand it the __has_include_next fallback MSVC needs (see
# EspeakNgCompat.cmake). It must apply to the pinned v1.8.0 sources; if the
# piper pin moves, this fails at build time instead of silently keeping the
# broken nested build.
#
# ExternalProject stamp-guards the patch step, so this normally runs once
# per build tree. The forward/reverse checks below keep it safe if the step
# ever re-runs on an already patched tree, and turn a pin mismatch into a
# clear error.

if(NOT DEFINED PATCH_FILE)
    message(FATAL_ERROR "ApplyPiperEspeakNgCompat.cmake requires -DPATCH_FILE=<patch>")
endif()
if(NOT DEFINED PIPER_SOURCE_DIR)
    message(FATAL_ERROR "ApplyPiperEspeakNgCompat.cmake requires -DPIPER_SOURCE_DIR=<piper checkout>")
endif()

execute_process(
    COMMAND git -C ${PIPER_SOURCE_DIR} apply --check ${PATCH_FILE}
    RESULT_VARIABLE _forward
    OUTPUT_QUIET ERROR_QUIET)
if(_forward EQUAL 0)
    execute_process(
        COMMAND git -C ${PIPER_SOURCE_DIR} apply ${PATCH_FILE}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Failed to apply ${PATCH_FILE}:\n${_output}${_error}")
    endif()
    return()
endif()

execute_process(
    COMMAND git -C ${PIPER_SOURCE_DIR} apply -R --check ${PATCH_FILE}
    RESULT_VARIABLE _reverse
    OUTPUT_QUIET ERROR_QUIET)
if(_reverse EQUAL 0)
    # Already applied: the patch step re-ran on a patched tree.
    return()
endif()

message(FATAL_ERROR
    "Cannot apply ${PATCH_FILE} to ${PIPER_SOURCE_DIR} in either direction. "
    "The piper checkout does not match the pinned v1.8.0 sources; delete the "
    "build tree and reconfigure.")
