/*
Copyright c1997-2005 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 2.3.2
http://www.bombaydigital.com/
*/

#ifndef vsocketbase_h
#define vsocketbase_h

/** @file */

#include "vinstant.h"
#include "vstring.h"

/**

    @defgroup vsocket Vault Sockets

    The Vault implements platform-independent sockets, for both clients and
    servers. The abstract base class VSocketBase defines the low-level API for dealing
    with sockets. It's low-level in the sense that most Vault users will not have
    to use this API other than calling init() to connect the socket when implementing
    a client-side connection. Instead, you'll be using classes like VSocketStream to
    associate a stream with a socket, and the upper layer stream classes to perform
    the actual socket i/o. And server implementors will similarly just be attaching
    a stream object to each socket that gets created for an incoming client connection.
    
    Each socket platform needs to define an implementation of the
    concrete subclass VSocket, so there is currently one defined when compiling for
    BSD/Unix sockets and one for WinSock, though these are very similar because their
    platform APIs are different mostly in small ways.
    
    Client code that needs to connect will instantiate a VSocket (whether this is the
    BSD version or the WinSock version just depends on what platform you are compiling
    on), and then presumably use a VSocketStream and a VIOStream to do i/o
    over the socket. Server code will typically create a VListenerThread, which will
    use a VListenerSocket, and turn each incoming connection into a VSocket and
    VSocketThread, both created via a factory that you can supply to define the
    concrete classes that are instantiated for each.
    
*/
    
// We have to define VSockID here because we don't include platform (it includes us).
#ifdef VPLATFORM_WIN
    typedef SOCKET VSockID;    ///< The platform-dependent definition of a socket identifier.
#else
    typedef int VSockID;    ///< The platform-dependent definition of a socket identifier.
#endif

typedef Vu32 VNetAddr;    ///< A 32-bit IP address, in network byte order (think of it as an array of 4 bytes, not as a 32-bit integer).

class VSocket;

/**
    @ingroup vsocket
*/

/**
VSocketBase is the abstract base class from which each platform's VSocket
implementation is derived. So you instantiate VSocket objects, but you
can see the API by looking at the VSocketBase class.

The basic way of using a client socket is to instantiate a VSocket (either
directly or through a VSocketFactory), and then call init() to connect to
the server.

The basic way of using a server socket is to instantiate a VListenerThread
and supplying it a VSocketFactory and a subclass of VSocketThreadFactory
that will create your kind of VSocketThread to handle requests for a given
connection.

The best way to do i/o on a socket once it's connected, is to instantiate
a VSocketStream for the socket, and then instantiate the appropriate subclass
of VIOStream to do well-typed reads and writes on the socket.

Here is how you would connect to a host, write a 32-bit integer in network
byte order, receive a similar response, and clean up:

<tt>
Vu32 exchangeSingleMessage(const VString& host, int port, Vu32 request)<br>
{<br>
VSocket    socket;<br>
socket.init(host, port);<br>
<br>
VSocketStream    stream(socket);<br>
VBinaryIOStream    io(stream);<br>
Vu32            response;<br>
<br>
io.writeU32(request);<br>
io.flush();<br>
response = io.readU32();<br>
<br>
return response;<br>
}    
</tt>

@see    VSocketFactory
@see    VSocketStream
@see    VListenerSocket
@see    VIOStream

FIXME:
According to some docs I have found, the proper shutdown sequence for a connected
socket is:
1. call shutdown(id, 1)    // well, not 1, so SHUT_WR or SHUT_RDWR? SHUT_RD would seem to make the next step fail
2. loop on recv until it returns 0 or any error
3. call close (or closesocket + WSACleanup on Win (is WSACleanup gone in WS2?))
*/
class VSocketBase
    {
    public:
    
        /**
        Returns the current processor's IP address. If an error occurs, this
        function throws an exception containing the error code and message.
        Because there is some overhead involved in this function, it's best
        if you try to call it only once, and stash the result in a global
        that you can reference later if needed.
        @param    ipAddress    a string in which to place the host name
        */
        static void getLocalHostIPAddress(VString& ipAddress);
        /**
        Converts an IP address string in dot notation to a 4-byte value
        that can be stored in a stream as an int. Note that the return
        value is in network byte order by definition--think of it as
        an array of 4 bytes, not a 32-bit integer.
        @param    ipAddress    the string to convert (must be in x.x.x.x
                                notation)
        @return a 4-byte IP address
        */
        static VNetAddr ipAddressStringToNetAddr(const VString& ipAddress);
        /**
        Converts a 4-byte IP address value into a string in the dot notation
        format. Note that the input value is in network byte order by
        definition--think of it as an array of 4 bytes, not a 32-bit integer.
        @param    netAddr        the 4-byte IP address
        @param    ipAddress    the string in which to place the dot notation
                                version of the address
        */
        static void netAddrToIPAddressString(VNetAddr netAddr, VString& ipAddress);

        /**
        Constructs the object; does NOT open a connection.
        */
        VSocketBase();
        /**
        Destructor, cleans up by closing the socket.
        */
        virtual ~VSocketBase();

        /**
        Initializes the socket object by attaching it to an already-open socket.
        This is the init method you would use if you are handing off the
        lower level sockets between VSocket objects, or if you are talking
        to a different API that opens the socket but you want to use it
        with the VSocket classes.
        @param    id    the id of the already-open socket
        */
        virtual void    init(VSockID id);
        /**
        Initializes the socket object by opening a connection to a server at the
        specified host and port.
        If the connection cannot be opened, a VException is thrown.
        @param    hostName    the host name, numeric or not is fine
        @param    portNumber    the port number to connect to on the host
        */
        virtual void    init(const VString& hostName, int portNumber);
        
        // --------------- These are the various utility and accessor methods.
        
        /**
        Returns the socket id.
        @return    the socket id
        */
        VSockID        getSockID() const;
        /**
        Associates this socket object with the specified socket id. This is
        something you might use if you are managing sockets externally and
        want to have a socket object own a socket temporarily.
        
        Note that this method does not cause a previous socket to be closed,
        nor does it update the name and port number properties of this object.
        If you want those things to happen, you can call close() and
        discoverHostAndPort() separately.
        
        @param    id    the socket id of the socket to manage
        */
        void        setSockID(VSockID id);
        /**
        Returns the name or address of the host to which this socket is
        connected.
        @param    hostName    the string to format
        */
        void    getHostName(VString& hostName) const;
        /**
        Returns the port number on the host to which this socket is
        connected.
        @return     the host's port number to which this socket is connected
        */
        int            getPortNumber() const;
        /**
        Closes the socket. This terminates the connection.
        */
        void        close();
        /**
        Sets the linger value for the socket.
        @param    val    the linger value in seconds
        */
        void        setLinger(int val);
        /**
        Removes the read timeout setting for the socket.
        */
        void        clearReadTimeOut();
        /**
        Sets the read timeout setting for the socket.
        @param    timeout    the read timeout value
        */
        void        setReadTimeOut(const struct timeval& timeout);
        /**
        Removes the write timeout setting for the socket.
        */
        void        clearWriteTimeOut();
        /**
        Sets the write timeout setting for the socket.
        @param    timeout    the write timeout value
        */
        void        setWriteTimeOut(const struct timeval& timeout);
        /**
        Sets the socket options to their default values.
        */
        void        setDefaultSockOpt();
        /**
        Returns the number of bytes that have been read from this socket.
        @return    the number of bytes read from this socket
        */
        Vs64        numBytesRead() const;
        /**
        Returns the number of bytes that have been written to this socket.
        @return    the number of bytes written to this socket
        */
        Vs64        numBytesWritten() const;
        /**
        Returns the number of milliseconds since the last read or write activity
        occurred on this socket.
        */
        Vs64        getIdleTime() const;
        
        // --------------- These are the pure virtual methods that only a platform
        // subclass can implement.

        /**
        Connects to the server.
        */
        virtual void    connect() = 0;
        /**
        Starts listening for incoming connections. Only useful to call
        with a VListenerSocket subclass, but needed here for class
        hierarchy implementation reasons (namely, it is implemented by
        the VSocket platform-specific class that VListenerSocket
        derives from).
        */
        virtual void    listen() = 0;
        /**
        Returns the number of bytes that are available to be read on this
        socket. If you do a read() on that number of bytes, you know that
        it will not block.
        @return the number of bytes currently available for reading
        */
        virtual int        available() = 0;
        /**
        Reads data from the socket.
        
        If you don't have a read timeout set up for this socket, then
        read will block until all requested bytes have been read.
        
        @param    buffer            the buffer to read into
        @param    numBytesToRead    the number of bytes to read from the socket
        @return    the number of bytes read
        */
        virtual int        read(Vu8* buffer, int numBytesToRead) = 0;
        /**
        Writes data to the socket.
        
        If you don't have a write timeout set up for this socket, then
        write will block until all requested bytes have been written.
        
        @param    buffer            the buffer to read out of
        @param    numBytesToWrite    the number of bytes to write to the socket
        @return    the number of bytes written
        */
        virtual int        write(const Vu8* buffer, int numBytesToWrite) = 0;
        /**
        Flushes any unwritten bytes to the socket.
        */
        virtual void    flush();
        /**
        Sets the host name and port number properties of this socket by
        asking the lower level services to whom the socket is connected.
        */
        virtual void    discoverHostAndPort() = 0;
        /**
        Shuts down just the read side of the connection.
        */
        virtual void    closeRead() = 0;
        /**
        Shuts down just the write side of the connection.
        */
        virtual void    closeWrite() = 0;
        /**
        Sets a specified socket option.
        @param    level        the option level
        @param    name        the option name
        @param    valuePtr    a pointer to the new option value data
        @param    valueLength    the length of the data pointed to by valuePtr
        */
        virtual void    setSockOpt(int level, int name, void* valuePtr, int valueLength) = 0;
        
        CLASS_CONST(VSockID, kNoSockID, -1);            ///< The sock id for a socket that is not connected.
        CLASS_CONST(int, kDefaultBufferSize, 65535);    ///< The default buffer size.
        CLASS_CONST(int, kDefaultServiceType, 0x08);    ///< The default service type.
        CLASS_CONST(int, kDefaultNoDelay, 1);            ///< The default nodelay value.

    protected:
    
        /** Asserts if any invariant is broken. */
        void assertInvariant() const;

        VSockID            mSockID;                ///< The sock id.
        VString            mHostName;                ///< The name of the host to which the socket is connected.
        int                mPortNumber;            ///< The port number on the host to which the socket is connected.
        bool            mReadTimeOutActive;        ///< True if reads should time out.
        struct timeval    mReadTimeOut;            ///< The read timeout value, if used.
        bool            mWriteTimeOutActive;    ///< True if writes should time out.
        struct timeval    mWriteTimeOut;            ///< The write timeout value, if used.
        int                mListenBacklog;            ///< The listen backlog value.
        bool            mRequireReadAll;        ///< True if we throw when read returns less than # bytes asked for.
        Vs64            mNumBytesRead;            ///< Number of bytes read from this socket.
        Vs64            mNumBytesWritten;        ///< Number of bytes written to this socket.
        VInstant        mLastEventTime;            ///< Timestamp of last read or write.
    };

/**
VSocketInfo is essentially a structure that just contains a copy of
information about a VSocket as it existed at the point in time when
the VSocketInfo was created.
*/
class VSocketInfo
    {
    public:
    
        /**
        Constructs the object by copying the info from the socket.
        @param    socket    the socket to take the information from
        */
        VSocketInfo(const VSocket& socket);
        /**
        Destructor.
        */
        virtual ~VSocketInfo() {}
    
        VSockID    mSockID;            ///< The sock id.
        VString    mHostName;            ///< The name of the host to which the socket is connected.
        int        mPortNumber;        ///< The port number on the host to which the socket is connected.
        Vs64    mNumBytesRead;        ///< Number of bytes read from this socket.
        Vs64    mNumBytesWritten;    ///< Number of bytes written to this socket.
        Vs64    mIdleTime;            ///< Milliseconds elapsed since last activity.
    };

/**
VSocketInfoVector is simply a vector of VSocketInfo objects.
*/
typedef std::vector<VSocketInfo> VSocketInfoVector;

#endif /* vsocketbase_h */
