/*
Copyright c1997-2008 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 3.0
http://www.bombaydigital.com/
*/

#include "vmessageoutputthread.h"

#include "vexception.h"
#include "vclientsession.h"
#include "vmessagepool.h"
#include "vsocket.h"
#include "vmessageinputthread.h"

// VMessageOutputThread -------------------------------------------------------

VMessageOutputThread::VMessageOutputThread(const VString& name, VSocket* socket, VListenerThread* ownerThread, VServer* server, VClientSession* session, VMessageInputThread* dependentInputThread, VMessagePool* messagePool, int maxQueueSize, Vs64 maxQueueDataSize, const VDuration& maxQueueGracePeriod) :
VSocketThread(name, socket, ownerThread),
mMessagePool(messagePool),
mOutputQueue(), // -> empty
mSocketStream(socket, "VMessageOutputThread"),
mOutputStream(mSocketStream),
mServer(server),
mSessionReference(session),
mDependentInputThread(dependentInputThread),
mMaxQueueSize(maxQueueSize),
mMaxQueueDataSize(maxQueueDataSize),
mMaxQueueGracePeriod(maxQueueGracePeriod),
mWhenMaxQueueSizeWarned(VInstant() - VDuration::MINUTE()), // one minute ago (past warning throttle threshold)
mWasOverLimit(false),
mWhenWentOverLimit(VInstant::NEVER_OCCURRED())
    {
    if (mDependentInputThread != NULL)
        mDependentInputThread->setHasOutputThread(true);
    }

VMessageOutputThread::~VMessageOutputThread()
    {
    mOutputQueue.releaseAllMessages();

    /*
    We share the socket w/ the input thread. We sort of let the input
    thread be the owner. So we don't want our superclass to see
    mSocket and clean it up. Just set it to NULL so that the other
    class will be the one to do so.
    */
    mSocket = NULL;

    mMessagePool = NULL;
    mServer = NULL;
    mDependentInputThread = NULL;
    }

void VMessageOutputThread::run()
    {
    try
        {
        while (this->isRunning())
            this->_processNextOutboundMessage();
        }
    catch (const VSocketClosedException& /*ex*/)
        {
        VLOGGER_MESSAGE_LEVEL(VLogger::kDebug, VString("[%s] VMessageOutputThread: Socket has closed, thread will end.", mName.chars()));
        }
    catch (const VException& ex)
        {
        /*
        Unlike the input threads, we shouldn't normally get an EOF exception to indicate that the
        connection has been closed normally, because we are an output thread. So any exceptions
        that land here uncaught are socket i/o errors and are logged as such. However, if our thread
        has been told to stop -- is no longer in running state -- then exceptions due to the socket
        being closed programmatically are to be expected, so we check that before logging an error.
        */
        if (this->isRunning())
            VLOGGER_MESSAGE_ERROR(VString("[%s] VMessageOutputThread::run: Exiting due to top level exception #%d '%s'.", mName.chars(), ex.getError(), ex.what()));
        else
            VLOGGER_MESSAGE_LEVEL(VLogger::kDebug, VString("[%s] VMessageOutputThread: Socket has closed, thread will end.", mName.chars()));
        }
    catch (const std::exception& ex)
        {
        if (this->isRunning())
            VLOGGER_MESSAGE_ERROR(VString("[%s] VMessageOutputThread: Exiting due to top level exception '%s'.", mName.chars(), ex.what()));
        }
    catch (...)
        {
        if (this->isRunning())
            VLOGGER_MESSAGE_ERROR(VString("[%s] VMessageOutputThread: Exiting due to top level unknown exception.", mName.chars()));
        }

    if (mSessionReference.getSession() != NULL)
        mSessionReference.getSession()->shutdown(this);

    if (mDependentInputThread != NULL)
        mDependentInputThread->setHasOutputThread(false);
    }

void VMessageOutputThread::stop()
    {
    VSocketThread::stop();
    mOutputQueue.wakeUp(); // if it's blocked, this is needed to kick it back to its run loop
    }

void VMessageOutputThread::attachSession(VClientSession* session)
    {
    mSessionReference.setSession(session);
    }

void VMessageOutputThread::postOutputMessage(VMessage* message, bool respectQueueLimits)
    {
    if (respectQueueLimits)
        {
        int currentQueueSize = 0;
        Vs64 currentQueueDataSize = 0;
        if (! this->isOutputQueueOverLimit(currentQueueSize, currentQueueDataSize))
            {
            mWasOverLimit = false;
            }
        else
            {
            VInstant now;
            bool gracePeriodExceeded = false;

            if (mWasOverLimit)
                {
                // Still over limit. Have we exceeded the grace period?
                VDuration howLongOverLimit = now - mWhenWentOverLimit;
                gracePeriodExceeded = (howLongOverLimit > mMaxQueueGracePeriod);
                }
            else
                {
                // We've just gone over the limit.
                // If there is a grace period, note the time.
                if (mMaxQueueGracePeriod == VDuration::ZERO())
                    {
                    gracePeriodExceeded = true;
                    }
                else
                    {
                    mWhenWentOverLimit = now;
                    mWasOverLimit = true;
                    }
                }

            if (gracePeriodExceeded)
                {
                if (this->isRunning()) // Only stop() once; we may land here repeatedly under fast queueing, before stop completes.
                    {
                    VLOGGER_ERROR(VString("[%s] VMessageOutputThread::postOutputMessage: Closing socket to shut down session because output queue size of %d messages and %lld bytes is over limit.",
                        mName.chars(), currentQueueSize, currentQueueDataSize));

                    this->stop();
                    }

                return;
                }
            else
                {
                if (now - mWhenMaxQueueSizeWarned > VDuration::MINUTE()) // Throttle the rate of ongoing warnings.
                    {
                    mWhenMaxQueueSizeWarned = now;
                    VDuration gracePeriodRemaining = (mWhenWentOverLimit + mMaxQueueGracePeriod) - now;
                    VLOGGER_WARN(VString("[%s] VMessageOutputThread::postOutputMessage: Posting to queue with excess size of %d messages and %lld bytes. Remaining grace period %d seconds.",
                        mName.chars(), currentQueueSize, currentQueueDataSize, gracePeriodRemaining.getDurationSeconds()));
                    }
                }
            }
        }

  	// 2009.08.31 JHR ARGO-20463 XPS Server Crash: 08/20/09
	// push_back can throw a std::bad_alloc, which may indicate we have a leak or have
	// run out of space on the hard disk (for paging), etc.
	try {
		mOutputQueue.postMessage(message);
		}
	catch (...) {
        VLOGGER_ERROR(VString("[%s] VMessageOutputThread::postOutputMessage: Closing socket to shut down session because ran out memory.", mName.chars()));

        this->stop();
		}
    }

void VMessageOutputThread::releaseAllQueuedMessages()
    {
    mOutputQueue.releaseAllMessages();
    }

int VMessageOutputThread::getOutputQueueSize() const
    {
    return static_cast<int>(mOutputQueue.getQueueSize());
    }

bool VMessageOutputThread::isOutputQueueOverLimit(int& currentQueueSize, Vs64& currentQueueDataSize) const
    {
    currentQueueSize = static_cast<int>(mOutputQueue.getQueueSize());
    currentQueueDataSize = mOutputQueue.getQueueDataSize();

    return (((mMaxQueueSize != 0) && (currentQueueSize >= mMaxQueueSize)) ||
        ((mMaxQueueDataSize != 0) && (currentQueueDataSize >= mMaxQueueDataSize)));
    }

void VMessageOutputThread::_processNextOutboundMessage()
    {
    VMessage* message = mOutputQueue.blockUntilNextMessage();

    if (message == NULL)
        {
        // OK -- means we were awakened from block but w/o a message actually available
        }
    else
        {
        if (mSessionReference.getSession() != NULL)
            mSessionReference.getSession()->sendMessageToClient(message, mName, mOutputStream);
        else
            {
            // We are just a client. No "session". Just send.
            VLOGGER_CONDITIONAL_MESSAGE_LEVEL(VMessage::kMessageQueueOpsLevel, VString("[%s] VMessageOutputThread::_processNextOutboundMessage: Sending message@0x%08X.", mName.chars(), message));
            message->send(mName, mOutputStream);
            }

        VMessagePool::releaseMessage(message, mMessagePool);
        }
    }

