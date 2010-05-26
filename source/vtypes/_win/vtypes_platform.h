/*
Copyright c1997-2008 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 3.0
http://www.bombaydigital.com/
*/

#ifndef vtypes_platform_h
#define vtypes_platform_h

/** @file */

/*
First, define the 2 fundamental properties:
- platform
- compiler
*/
// Define the platform.
#define VPLATFORM_WIN

// Detect and define the compiler.
// We also need to detect the Metrowerks MSL library for some differences elsewhere.

/*
Visual C++ version 6 and earlier have awful standard C++ compliance,
so for those crippled compilers we have do a few workarounds for basic
C++ work.

_MSC_VER is defined by MSVC++. The values I've been able to find
documented are:
1020 - version 4.2 (pretty old)
1200 - version 6 SP4 (a little old now but very widely used)
1300 - version 7 (a.k.a. "2003" or ".NET")
1310 - version 7.1 (patch/update to version 7)
1400 - version 8.0 (a.k.a. "2005")

Symbols we define conditionally:

VCOMPILER_CODEWARRIOR - Defined if the compiler is CodeWarrior.

VCOMPILER_MSVC - Defined if the compiler is Visual C++.

VCOMPILER_MSVC_6_CRIPPLED - Defined if the compiler is Visual C++
and the version is 6 or earlier, based on _MSC_VER. Workarounds
become required.
*/

#ifdef _MSC_VER
#define VCOMPILER_MSVC

    #if _MSC_VER < 1300
        #define VCOMPILER_MSVC_6_CRIPPLED
    #endif

    // For the moment, turn off the new 8.0 library deprecation stuff.
    // Later, we can change the code to conditionally use the newer
    // function names, depending on the compiler version.
    #if _MSC_VER >= 1400
        #define _CRT_SECURE_NO_DEPRECATE
    #endif

#endif

#ifdef __MWERKS__
    #define VCOMPILER_CODEWARRIOR
    #define VLIBRARY_METROWERKS    /* might want to consider adding a check for using CW but not MSL */
#endif

/*
Second, include the user-editable header file that configures
the desired Vault features/behavior. It can decide based on
the fundamental properties defined above.
*/
#include "vconfigure.h"

/*
Finally, proceed with everything else.
*/

// Boost seems to require being included first.
#ifdef VAULT_BOOST_STRING_FORMATTING_SUPPORT
    // Prevent spurious VC8 warnings about "deprecated"/"unsafe" boost std c++ lib use.
    #if _MSC_VER >= 1400
        #pragma warning(push)
        #pragma warning(disable : 4996)
    #endif

    #include <boost/format.hpp>

    #if _MSC_VER >= 1400
        #pragma warning(pop)
    #endif
#endif

// Minimal includes needed to compile against the Vault header files.

#include <winsock2.h>
#include <windows.h>
#include <algorithm> // std::find
#include <math.h>

#ifdef VCOMPILER_CODEWARRIOR
    #include <time.h>
    #include <stdio.h>
#endif

// Platform-specific definitions for types, byte order, min/max/abs/fabs, etc.

// If Windows globally #defines these as preprocessor macros, they cannot
// be used as method names! Get rid of them.
#undef min
#undef max
#undef abs

/*
Here are the workarounds we need to define if we're compiling under
the crippled VC++ 6 compiler.
*/
#ifdef VCOMPILER_MSVC_6_CRIPPLED
    // 64-bit integer definitions are non-standard. Should be signed/unsigned long long.
    typedef LONGLONG  Vs64;
    typedef ULONGLONG Vu64;
    // 64-bit constants definitions are non-standard. Should be LL and ULL.
    #define CONST_S64(s) s##i64
    #define CONST_U64(s) s##i64
    // Static constants are not supported, so this is a way to create them.
    #define CLASS_CONST(type, name, init) enum { name = init }
    // Scoped variables do not work. This works around that.
    #define for if(false);else for
#endif

/*
Since Windows currently only runs on X86 (well, little-endian) processors, we
can simply define the byte-swapping macros to be unconditionally enabled. If
Windows ever runs on a big-endian processor, these macros can just be conditionally
defined as they are on Unix and Mac, based on the processor type.
*/
#define VBYTESWAP_NEEDED // This means "host order" and "network order" differ.

/*
VC++ up to and including 7.1 does not like using std::min(), std::max(), std::abs()
so our convenience macros V_MIN, V_MAX, and V_ABS need to be implemented as
old-fashioned C preprocessor macros rather than standard C++ library inlines.
*/
#ifdef VCOMPILER_MSVC
    #define DEFINE_V_MINMAXABS 1
#endif

#ifdef DEFINE_V_MINMAXABS

    #define V_MIN(a, b) ((a) > (b) ? (b) : (a)) ///< Macro for getting min of compatible values when standard functions / templates are not available.
    #define V_MAX(a, b) ((a) > (b) ? (a) : (b)) ///< Macro for getting max of compatible values when standard functions / templates are not available.
    #define V_ABS(a) ((a) < 0 ? (-(a)) : (a))   ///< Macro for getting abs of an integer value when standard functions / templates are not available.
    #define V_FABS(a) ((a) < 0 ? (-(a)) : (a))  ///< Macro for getting abs of a floating point value when standard functions / templates are not available.

#else

    #define V_MIN(a, b) std::min(a, b) ///< Macro for getting min of compatible values using standard function template.
    #define V_MAX(a, b) std::max(a, b) ///< Macro for getting max of compatible values using standard function template.
    #define V_ABS(a) std::abs(a)       ///< Macro for getting abs of an integer value using standard function template.
    #define V_FABS(a) std::fabs(a)     ///< Macro for getting abs of a floating point value using standard function template.

#endif /* DEFINE_V_MINMAXABS */

// vsnprintf(NULL, 0, . . .) behavior conforms to IEEE 1003.1 on CW/Win and VC++ 8
// (may need to set conditionally for older versions of VC++).
#define V_EFFICIENT_SPRINTF

// Suppress incorrect warnings from Level 4 VC++ warning level. (/W4 option)
#ifdef VCOMPILER_MSVC
    // VLOGGER_xxx macros properly use "do ... while(false)" but this emits warning 4127.
    #pragma warning(disable: 4127)
#endif

#ifdef VCOMPILER_MSVC
    #ifdef VAULT_WIN32_STRUCTURED_EXCEPTION_TRANSLATION_SUPPORT
        #define V_TRANSLATE_WIN32_STRUCTURED_EXCEPTIONS // This is the private symbol we actually used. Depends on user setting, platform, and compiler.
    #endif
#endif

#endif /* vtypes_platform_h */
