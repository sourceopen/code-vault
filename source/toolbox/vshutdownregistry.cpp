/*
Copyright c1997-2014 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 4.1
http://www.bombaydigital.com/
License: MIT. See LICENSE.md in the Vault top level directory.
*/

#include "vshutdownregistry.h"

#include "vmutex.h"
#include "vmutexlocker.h"

VShutdownRegistry* VShutdownRegistry::gInstance = NULL;

// This style of static mutex declaration and access ensures correct
// initialization if accessed during the static initialization phase.
static VMutex* _mutexInstance() {
    static VMutex gMutex("VShutdownRegistry _mutexInstance() gMutex");
    return &gMutex;
}

// static
VShutdownRegistry* VShutdownRegistry::instance() {
    VMutexLocker locker(_mutexInstance(), "VShutdownRegistry::instance()");

    if (gInstance == NULL) {
        gInstance = new VShutdownRegistry();
    }

    return gInstance;
}

// static
void VShutdownRegistry::shutdown() {
    VMutexLocker locker(_mutexInstance(), "VShutdownRegistry::shutdown()");

    if (gInstance != NULL) {
        delete gInstance;
        gInstance = NULL;
    }
}

void VShutdownRegistry::registerHandler(IVShutdownHandler* handler) {
    VMutexLocker locker(_mutexInstance(), "VShutdownRegistry::registerHandler()");

    mHandlers.push_back(handler);
}

void VShutdownRegistry::registerFunction(shutdownFunction func) {
    VMutexLocker locker(_mutexInstance(), "VShutdownRegistry::registerFunction()");

    mFunctions.push_back(func);
}

VShutdownRegistry::~VShutdownRegistry() {
    // Note that this is only called via our static shutdown() function,
    // which takes responsibility for thread-safety. Don't lock, or we'll
    // have a deadlock.

    for (ShutdownFunctionList::iterator i = mFunctions.begin(); i != mFunctions.end(); ++i) {
        shutdownFunction func = (*i);

        func();
    }

    for (ShutdownHandlerList::iterator i = mHandlers.begin(); i != mHandlers.end(); ++i) {
        IVShutdownHandler*    handler = (*i);

        bool deleteHandler = handler->mDeleteAfterShutdown; // save first; _shutdown() could delete handler
        handler->_shutdown();

        if (deleteHandler) {
            delete handler;
        }

        (*i) = NULL;
    }
}

