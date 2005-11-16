/*
Copyright c1997-2005 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 2.3.2
http://www.bombaydigital.com/
*/

#ifndef vthread_platform_h
#define vthread_platform_h

/** @file */

// Win32 threads data types.

#ifndef _WIN32
#define _WIN32
#endif

#include "vtypes_platform.h"

#ifdef __MWERKS__
typedef ULONG                VThreadID_Type;
#else
typedef uintptr_t            VThreadID_Type;
#endif
typedef HANDLE                VSemaphore_Type;
typedef CRITICAL_SECTION    VMutex_Type;
typedef long                VTimeout_Type;

typedef void (*Win32ThreadMainFunction)(void* arg);
typedef unsigned (__stdcall *Win32ThreadMainFunctionEx)(void* arg);

#endif /* vthread_platform_h */
