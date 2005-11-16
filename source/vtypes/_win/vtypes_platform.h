/*
Copyright c1997-2005 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 2.3.2
http://www.bombaydigital.com/
*/

#ifndef vtypes_platform_h
#define vtypes_platform_h

/** @file */

/*
When compiling for Windows, we have a few differences introduced
because of varying compiler quality of Microsoft Visual C++.

We check here to verify that we are compiling with Microsoft Visual
C++, and specifically which version. This will allow us to set a
couple of preprocessor symbols that we can use here and elsewhere
for conditional compilation.

_MSC_VER is defined by MSVC++. The values I've been able to find
documented are:
1020 - version 4.2 (pretty old)
1200 - version 6 SP4 (a little old now but very widely used)
1300 - version 7 (a.k.a. "2003" or ".NET")
1310 - version 7.1 (patch/update to version 7)

Versions prior to 7 (1300) are really awful due to their lack of
support for some basic common standard C++ constructs. Therefore,
we define two symbols based on the _MSC_VER value.

VCOMPILER_MSVC - This is to identify that we are using MSVC and
not some other compiler like CodeWarrior X86 or gcc.

VCOMPILER_MSVC_6_CRIPPLED - This is used to
identify that we are using a version earlier than 7 (1300), and
thus need to include hacks to work around the compiler's deficiencies.

Wherever we need to work around a problem in all VC++ versions,
we can just conditionally compile for VCOMPILER_MSVC. Whenever we
need to work around a problem just for VC++ 6 and earlier, we can
conditionally compile for VCOMPILER_MSVC_6_CRIPPLED. Whenever possible,
we do the workarounds just for VCOMPILER_MSVC_6_CRIPPLED, but some
quirks are still present for later compilers (see DEFINE_V_MINMAXABS
below for example).

Thus we don't reference _MSC_VER elsewhere in the Vault code, we just
set up the other symbols here and reference them only where really
needed.
*/

#ifdef _MSC_VER
#define VCOMPILER_MSVC
    #if _MSC_VER < 1300
    #define VCOMPILER_MSVC_6_CRIPPLED
    #endif
#endif

// The Code Vault does not currently support Unicode strings.
// If we let UNICODE be defined by VC++ it causes problems when
// we call the directory iterator APIs with non-Unicode strings.
#undef UNICODE

#ifdef VCOMPILER_MSVC
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifndef VCOMPILER_MSVC
#include <unistd.h> /* n/a for VC++, needed elsewhere */
#endif

#define VPLATFORM_WIN

#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <direct.h>

/*
The VC++ 6.0 compiler is nonstandard in how it defines 64-bit integers,
so we define them specially here.
*/
#ifdef VCOMPILER_MSVC_6_CRIPPLED
typedef LONGLONG    Vs64;
typedef ULONGLONG    Vu64;
#endif

/*
The VC++ 6.0 compiler is nonstandard in how it denotes 64-bit constants,
so we use a macro and define it specially here. The standard suffix
is LL for signed and ULL for unsigned. VC++ uses i64 (need to research
signed vs. unsigned).
*/
#ifdef VCOMPILER_MSVC_6_CRIPPLED
#define CONST_S64(s) s##i64
#define CONST_U64(s) s##i64
#endif

/*
The VC++ 6.0 compiler does not support the C++ standard for defining static
class constants such as "const static int kFoo = 1;" so we use a macro and
define it specially here.
*/
#ifdef VCOMPILER_MSVC_6_CRIPPLED
#define CLASS_CONST(type, name, init) enum { name = init }
#endif

/*
The VC++ 6.0 compiler is nonstandard in its lack of support for scoped
variables in for loops. This #define gets around that. This should be
#ifdef'd to the compiler version when they fix this bug. And ideally
this should be #ifdef'd to only be in effect for Microsoft's compiler.
*/
#ifdef VCOMPILER_MSVC_6_CRIPPLED
#define for if(false);else for
#endif

/*
VC++ up to and including 7.1 does not like using std::min(), std::max(), std::abs()
so our convenience macros V_MIN, V_MAX, and V_ABS need to be implemented as
old-fashioned C preprocessor macros rather than standard C++ library inlines.
*/
#ifdef VCOMPILER_MSVC
#define DEFINE_V_MINMAXABS 1
#endif

/*
Perhaps we can rely on a Win32 header to determine how these macros should
map as we do in the Unix header file, but for now let's just use X86 behavior
and assume we don't compile for Windows on processors that are not using
Intel byte order.
*/

#define VBYTESWAP_NEEDED

#define V_BYTESWAP_HTONS_GET(x)            VbyteSwap16((Vu16) x)
#define V_BYTESWAP_NTOHS_GET(x)            VbyteSwap16((Vu16) x)
#define V_BYTESWAP_HTONS_IN_PLACE(x)    ((x) = (VbyteSwap16((Vu16) x)))
#define V_BYTESWAP_NTOHS_IN_PLACE(x)    ((x) = (VbyteSwap16((Vu16) x)))

#define V_BYTESWAP_HTONL_GET(x)            VbyteSwap32((Vu32) x)
#define V_BYTESWAP_NTOHL_GET(x)            VbyteSwap32((Vu32) x)
#define V_BYTESWAP_HTONL_IN_PLACE(x)    ((x) = (VbyteSwap32((Vu32) x)))
#define V_BYTESWAP_NTOHL_IN_PLACE(x)    ((x) = (VbyteSwap32((Vu32) x)))

#define V_BYTESWAP_HTON64_GET(x)        VbyteSwap64((Vu64) x)
#define V_BYTESWAP_NTOH64_GET(x)        VbyteSwap64((Vu64) x)
#define V_BYTESWAP_HTON64_IN_PLACE(x)    ((x) = (VbyteSwap64((Vu64) x)))
#define V_BYTESWAP_NTOH64_IN_PLACE(x)    ((x) = (VbyteSwap64((Vu64) x)))

#define V_BYTESWAP_HTONF_GET(x)            VbyteSwapFloat((VFloat) x)
#define V_BYTESWAP_NTOHF_GET(x)            VbyteSwapFloat((VFloat) x)
#define V_BYTESWAP_HTONF_IN_PLACE(x)    ((x) = (VbyteSwapFloat((VFloat) x)))
#define V_BYTESWAP_NTOHF_IN_PLACE(x)    ((x) = (VbyteSwapFloat((VFloat) x)))

typedef size_t ssize_t;
/* #define S_ISLNK(x) ... Would need equivalent for stat.mode on Win32, see VFSNode::isDirectory() */
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND

#ifdef VCOMPILER_MSVC
typedef int mode_t;
#define open _open
#define rmdir _rmdir
#define vsnprintf _vsnprintf
#define S_IRWXO    _S_IREAD | _S_IWRITE
#define S_IRWXG    _S_IREAD | _S_IWRITE
#define S_IRWXU    _S_IREAD | _S_IWRITE
/* #define S_ISDIR(x) ... Would need equivalent for stat.mode on Win32, see VFSNode::isDirectory() */
#endif

// On Windows, we implement snapshot using _ftime64(), which is UTC-based.
#define V_INSTANT_SNAPSHOT_IS_UTC    // platform_snapshot() gives us a UTC time suitable for platform_now()

// We have to implement timegm() because there's no equivalent Win32 function.
extern time_t timegm(struct tm* t);

// The WinSock types fail to define in_addr_t, so to avoid making VSocketBase
// contain conditional code for address resolution, we just define it here.
typedef unsigned long in_addr_t;

#ifdef DEFINE_V_MINMAXABS

#define V_MIN(a, b) ((a) > (b) ? (b) : (a))    ///< Macro for getting min of compatible values when standard functions / templates are not available.
#define V_MAX(a, b) ((a) > (b) ? (a) : (b))    ///< Macro for getting max of compatible values when standard functions / templates are not available.
#define V_ABS(a) ((a) < 0 ? (-(a)) : (a))    ///< Macro for getting abs of an integer value when standard functions / templates are not available.
#define V_FABS(a) ((a) < 0 ? (-(a)) : (a))    ///< Macro for getting abs of a floating point value when standard functions / templates are not available.

#else

#define V_MIN(a, b) std::min(a, b)    ///< Macro for getting min of compatible values using standard function template.
#define V_MAX(a, b) std::max(a, b)    ///< Macro for getting max of compatible values using standard function template.
#define V_ABS(a) std::abs(a)        ///< Macro for getting abs of an integer value using standard function template.
#define V_FABS(a) std::fabs(a)        ///< Macro for getting abs of a floating point value using standard function template.

#endif /* DEFINE_V_MINMAXABS */

#endif /* vtypes_platform_h */