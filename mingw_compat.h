// MinGW compatibility wrapper
// Suppress MSVC-specific #pragma comment(lib, ...) and #pragma message(...)
// when compiling with MinGW

#ifndef MINGW_COMPAT_H
#define MINGW_COMPAT_H

#ifdef __MINGW32__
// MinGW understands __declspec but not _declspec
#ifndef _declspec
#define _declspec(x) __declspec(x)
#endif

// Suppress #pragma comment(lib, ...) - handled by CMake link_libraries
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif /* __MINGW32__ */

#endif /* MINGW_COMPAT_H */
