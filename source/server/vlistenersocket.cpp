/*
Copyright c1997-2014 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 4.1
http://www.bombaydigital.com/
License: MIT. See LICENSE.md in the Vault top level directory.
*/

/** @file */

#include "vlistenersocket.h"
#include "vtypes_internal.h"

#include "vexception.h"
#include "vsocketfactory.h"

VListenerSocket::VListenerSocket(int portNumber, const VString& bindAddress, VSocketFactory* factory, int backlog)
    : VSocket()
    , mBindAddress(bindAddress)
    , mBacklog(backlog)
    , mFactory(factory)
    {
    this->setHostIPAddressAndPort(VSTRING_FORMAT("listener(%s:%d)", bindAddress.chars(), portNumber), portNumber);

    /*
    We need to have our listen() calls timeout if we expect to allow
    other threads (e.g., a thread that is handling a remote server
    management command) to shut us down. Otherwise, we'll be blocked
    on listen() and never get a chance to even check isRunning().
    */
    struct timeval timeout;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    VListenerSocket::setReadTimeOut(timeout);
}

VListenerSocket::~VListenerSocket() {
}

VSocket* VListenerSocket::accept() {
    if (mSocketID == kNoSocketID) {
        throw VStackTraceException("VListenerSocket::accept called before socket is listening.");
    }

    struct sockaddr_in  clientaddr;
    VSocklenT           clientaddrLength = sizeof(clientaddr);
    VSocketID           handlerSockID = kNoSocketID;
    VSocket*            handlerSocket = NULL;
    bool                shouldAccept = true;

    if (mReadTimeOutActive) {
        /* then we need to do a select call */
        struct timeval  timeout = mReadTimeOut;
        fd_set          readset;

        FD_ZERO(&readset);
        //lint -e573 Signed-unsigned mix with divide"
        FD_SET(mSocketID, &readset);

        int result = ::select(static_cast<int>(mSocketID + 1), &readset, NULL, NULL, &timeout);

        if (result == -1) {
            throw VException(VSystemError::getSocketError(), VSTRING_FORMAT("VListenerSocket[%s:%d]::accept select() failed.", mBindAddress.chars(), mPortNumber));
        }

        //lint -e573 Signed-unsigned mix with divide"
        shouldAccept = ((result > 0) && FD_ISSET(mSocketID, &readset));
    }

    if (shouldAccept) {
        ::memset(&clientaddr, 0, static_cast<Vu32>(clientaddrLength));
        handlerSockID = ::accept(mSocketID, (struct sockaddr*) &clientaddr, &clientaddrLength);

        if (handlerSockID == kNoSocketID) {
            throw VException(VSystemError::getSocketError(), VSTRING_FORMAT("VListenerSocket[%s:%d]::accept accept() failed.", mBindAddress.chars(), mPortNumber));
        } else {
            handlerSocket = mFactory->createSocket(handlerSockID);
        }
    }

    return handlerSocket;
}

void VListenerSocket::listen() {
    this->_listen(mBindAddress, mBacklog);
}

