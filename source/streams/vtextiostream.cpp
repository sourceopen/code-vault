/*
Copyright c1997-2005 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 2.3.2
http://www.bombaydigital.com/
*/

/** @file */

#include "vtextiostream.h"

#include "vchar.h"
#include "vexception.h"

VTextIOStream::VTextIOStream(VStream& rawStream, int lineEndingsWriteKind)
: VIOStream(rawStream)
    {
    mLineBuffer.preflight(80);
    mLineEndingsReadKind = kLineEndingsUnknown;
    mLineEndingsWriteKind = lineEndingsWriteKind;
    mPendingCharacter = 0;
    mReadState = kReadStateReady;
    
    ASSERT_INVARIANT();
    }

void VTextIOStream::readLine(VString& s, bool includeLineEnding)
    {
    ASSERT_INVARIANT();

    // FIXME: optimize (perhaps using a temporary stack-based VMemoryStream
    // instead of the return VString) to allocate memory for the read
    // as gracefully and minimally as possible. This implementation simply
    // appends each char to the VString. This means its buffer is reallocated for
    // each character beyond the initial buffer size we specified in the
    // constructor (80).
    
    mLineBuffer = VString::kEmptyString;

    bool    readFirstByteOfLine = false;
    Vs64    numBytesToRead = 1;
    Vs64    numBytesRead;
    char    c = 0;
    bool    done = false;
    
    do
        {
        if (mPendingCharacter != 0)
            {
            c = mPendingCharacter;
            mPendingCharacter = 0;
            numBytesRead = 1;
            }
        else
            {
            numBytesRead = this->read(reinterpret_cast<Vu8*> (&c), 1);
            
            // Throw EOF if we fail reading very first byte of line.
            // Otherwise, we'll return whatever we read, and throw next time.
            if ((numBytesRead == 0) && ! readFirstByteOfLine)
                throw VEOFException("EOF");
            
            readFirstByteOfLine = true;
            }
        
        switch (mReadState)
            {
            case kReadStateReady:

                if (c == 0x0A)    // found a Unix line end
                    {
                    if (includeLineEnding)
                        mLineBuffer += c;
                    
                    done = true;    // done, bail out and return the string
                    
                    this->updateLineEndingsReadKind(kLineEndingsUnix);
                    }
                else if (c == 0x0D)    // found a Mac line end, or 1st byte of a DOS line end
                    {
                    mReadState = kReadStateGot0x0D;
                    }
                else    // found a normal character
                    {
                    mLineBuffer += c;
                    }

                break;
            
            case kReadStateGot0x0D:

                if (c == 0x0A)    // found a DOS line end
                    {
                    if (includeLineEnding)
                        {
                        mLineBuffer += static_cast<char> (0x0D);
                        mLineBuffer += static_cast<char> (0x0A);
                        }
                    
                    mReadState = kReadStateReady;
                    done = true;    // done, bail out and return the string

                    this->updateLineEndingsReadKind(kLineEndingsDOS);
                    }
                else    // found a normal character, so we have a Mac line end pending
                    {
                    if (includeLineEnding)
                        mLineBuffer += static_cast<char> (0x0D);
                        
                    mPendingCharacter = c;

                    mReadState = kReadStateReady;
                    done = true;

                    this->updateLineEndingsReadKind(kLineEndingsMac);
                    }

                break;
            }

        } while ((numBytesRead == numBytesToRead) && !done);

    s = mLineBuffer;

    ASSERT_INVARIANT();
    }

VChar VTextIOStream::readCharacter()
    {
    char    c;
    
    this->readGuaranteed(reinterpret_cast<Vu8*> (&c), 1);

    return c;
    }

void VTextIOStream::writeLine(const VString& s)
    {
    ASSERT_INVARIANT();

    (void) this->write(reinterpret_cast<Vu8*> (s.chars()), static_cast<Vs64> (s.length()));
    
    int        numBytes = 0;
    char    lineEnding[2];
    
    switch (mLineEndingsWriteKind)
        {
        case kUseUnixLineEndings:
#ifdef VPLATFORM_UNIX
        case kUseNativeLineEndings:
#endif
#ifdef VPLATFORM_MAC
        case kUseNativeLineEndings:    // On Mac OS X, it's usually best to treat Unix as native default text file type. You can explicitly specify kUseMacLineEndings if desired.
#endif
            lineEnding[0] = 0x0A;
            numBytes = 1;
            break;
        case kUseDOSLineEndings:
#ifdef VPLATFORM_WIN
        case kUseNativeLineEndings:
#endif
            lineEnding[0] = 0x0D;
            lineEnding[1] = 0x0A;
            numBytes = 2;
            break;
        case kUseMacLineEndings:
            lineEnding[0] = 0x0D;
            numBytes = 1;
            break;
        case kUseSuppliedLineEndings:
            // line ending was supplied by caller and already written
            break;
        default:
            throw VException("VTextIOStream::writeLine using invalid line ending mode.");
            break;
        }
    
    if (numBytes != 0)
        (void) this->write(reinterpret_cast<Vu8*> (lineEnding), static_cast<Vs64> (numBytes));

    ASSERT_INVARIANT();
    }

void VTextIOStream::writeString(const VString& s)
    {
    ASSERT_INVARIANT();

    (void) this->write(reinterpret_cast<Vu8*> (s.chars()), static_cast<Vs64> (s.length()));

    ASSERT_INVARIANT();
    }

void VTextIOStream::assertInvariant() const
    {
    V_ASSERT(mLineEndingsReadKind >= 0);
    V_ASSERT(mLineEndingsReadKind < kNumLineEndingsReadKinds);
    V_ASSERT(mLineEndingsWriteKind >= 0);
    V_ASSERT(mLineEndingsWriteKind < kNumLineEndingsWriteKinds);
    V_ASSERT(mReadState >= 0);
    V_ASSERT(mReadState < kNumReadStates);
    }

void VTextIOStream::updateLineEndingsReadKind(int lineEndingKind)
    {
    switch (mLineEndingsReadKind)
        {
        case kLineEndingsUnknown:
            switch (lineEndingKind)
                {
                case kLineEndingsUnknown:
                case kLineEndingsMixed:
                    V_ASSERT(false);    // invalid input value
                    break;

                case kLineEndingsUnix:
                case kLineEndingsDOS:
                case kLineEndingsMac:
                    mLineEndingsReadKind = lineEndingKind;
                    break;
                }
            break;

        case kLineEndingsUnix:
            switch (lineEndingKind)
                {
                case kLineEndingsUnknown:
                case kLineEndingsMixed:
                    V_ASSERT(false);    // invalid input value
                    break;

                case kLineEndingsUnix:
                    // no change
                    break;

                case kLineEndingsDOS:
                case kLineEndingsMac:
                    mLineEndingsReadKind = kLineEndingsMixed;
                    break;

                }
            break;

        case kLineEndingsDOS:
            switch (lineEndingKind)
                {
                case kLineEndingsUnknown:
                case kLineEndingsMixed:
                    V_ASSERT(false);    // invalid input value
                    break;

                case kLineEndingsUnix:
                case kLineEndingsMac:
                    mLineEndingsReadKind = kLineEndingsMixed;
                    break;

                case kLineEndingsDOS:
                    // no change
                    break;

                }
            break;

        case kLineEndingsMac:
            switch (lineEndingKind)
                {
                case kLineEndingsUnknown:
                case kLineEndingsMixed:
                    V_ASSERT(false);    // invalid input value
                    break;

                case kLineEndingsUnix:
                case kLineEndingsDOS:
                    mLineEndingsReadKind = kLineEndingsMixed;
                    break;

                case kLineEndingsMac:
                    // no change
                    break;

                }
            break;

        case kLineEndingsMixed:
            // Once we detect mixed input, it stays that way.
            break;
        }


    }

int VTextIOStream::lineEndingsReadKindForWrite() const
    {
    int    writeKind = kUseNativeLineEndings;

    switch (mLineEndingsReadKind)
        {
        case kLineEndingsUnix:
            writeKind = kUseUnixLineEndings;
            break;

        case kLineEndingsDOS:
            writeKind = kUseDOSLineEndings;
            break;

        case kLineEndingsMac:
            writeKind = kUseMacLineEndings;
            break;

        }
    
    return writeKind;
    }

void VTextIOStream::setLineEndingsKind(int kind)
    {
    mLineEndingsWriteKind = kind;
    }


