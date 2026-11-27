/* Forced include for MSVC builds of the nested espeak-ng project.
 *
 * The espeak-ng compat shims (src/include/compat/unistd.h, endian.h and
 * getopt.h) probe for real system headers with the C++23 __has_include_next
 * operator:
 *
 *     #if __has_include_next(<unistd.h>)
 *
 * MSVC does not implement that operator. MSVC 14.44 (Visual Studio 2022)
 * silently evaluated the undefined function-like name in the conditional as
 * 0, but MSVC 14.52 (Visual Studio 2026) rejects the construct with errors
 * C7780 and C1017. Upstream espeak-ng fixed the shims in commit 021e4653
 * ("fix(compat): fall back when the compiler has no __has_include_next"),
 * but piper v1.8.0 pins the earlier commit 212928b3 and no piper release
 * carries the fix.
 *
 * This header installs that same fallback: a function-like
 * __has_include_next that expands to 0, so the shims take the same path
 * MSVC 14.44 produced (the unistd.h shim stays empty, endian.h uses its
 * _WIN32 fallback and getopt.h provides its own declarations). It must be
 * force-included with /FI rather than passed as a /D definition, because
 * MSVC's /D switch does not support function-like macro definitions.
 *
 * The guard keeps the header harmless on compilers that implement the
 * operator: GCC and Clang count __has_include_next as defined, so the
 * fallback macro is never defined there. EspeakNgCompat.cmake only
 * force-includes this header on MSVC, for the nested espeak-ng project
 * alone.
 */
#ifndef __has_include_next
#define __has_include_next(x) 0
#endif
