/*  Sirikata Network Utilities
 *  TCPStream.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SIRIKATA_TCPStream_HPP__
#define SIRIKATA_TCPStream_HPP__
#include "Stream.hpp"
#include "util/AtomicTypes.hpp"
namespace Sirikata { namespace Network {
class MultiplexedSocket;
class TCPSetCallbacks;
class IOService;

/**
 * This is a particular example implementation of the Stream interface sitting atop TCP.
 * The protocol specified in the Structured streams paper http://pdos.csail.mit.edu/uia/sst/
 * May be used as another implementation of the Stream interface. This is provided as the 
 * default implementation as it supports a wide range of network devices and is not subject to
 * prejudice against UDP connections which we routinely see on the net (my home system gets 10 
 * second lag on UDP connections just due to ISP policy combating P2P networks)
 *
 * The protocol is as follows:
 *
 * --Connection--
 * To connect to a remote host, the client opens a small number (N) of TCP sockets to that host
 * and sends the version string (currently SSTTCP ), an ascii 2 digit representation of N (in this case '03')
 * and a 16 byte unique ID used to pair up connections and possibly in the future for encryption
 * The other side respinds in turn on each connection with a similar handshake (though the 16 bytes may be different)
 * now the connection is online. The remote host may immediately follow the handshake with live packets and 
 * the other side may respond as soon as it receives the remote hosts handshake response
 *
 * --Live Phase--
 * The first live stream has StreamID of 1. New streams coming from listener must have even StreamID's and new 
 * streams coming from connector must have odd stream ID's.
 * To indicate a new substream, simply send a packet with a new stream ID
 *
 * --Packet Format--
 * All packets are prepended by a variable length int30 length and int30 StreamID and then the bytestream of data
 * The length value includes the length of the variable-length StreamID. 
 * the int30 format is as follows
 *      if the highest value bit in the first byte is 0, the other 7 bits represent numbers 0-128
 *      if the highest value bit in the first byte is 1, and the highest value bit in the second byte is 0
 *            the first 7 bits represent the 0-127 remainder and the first 7 bits in the second byte represent
 *            the 0-128 significant 7 bit digit valued between 0 and 16256
 *      if the highest value bit in the first and second bytes are both 1
 *            the value between 0 and 16383 above is added to 16384*the third byte added to 16384*256*the fourth byte
 *
 * --Shutting down a Stream--
 * If either side decides to close a stream they must send a control packet (which is the implicit stream with
 * StreamID = default int30, which serializes to a single byte=0) 
 * and then a control code which must be a byte equal to 1, and finally a variable length StreamID indicating which stream to close
 * This control packet must be broadcast on every related open TCP socket connection
 * This party may not reuse the streamID (given parity match) until it receives control packets with control code equal to 2 (close Ack) on all sockets.
 * The other side must keep the bargain and send the control packet with control code 2 on all sockets to allow ID reuse.
 * 
 * If all streams are shut down, the sockets may be deactivated
 * If the socket disconnects due to error, then Disconnect callbacks must be called
 */
class SIRIKATA_EXPORT TCPStream:public Stream {
public:
    class Callbacks;
    static const char * STRING_PREFIX() {
        return "SSTTCP";
    }
    enum HeaderSizeEnumerant {
        STRING_PREFIX_LENGTH=6,
        TcpSstHeaderSize=24
    };
    enum TCPStreamControlCodes {
        TCPStreamCloseStream=1,
        TCPStreamAckCloseStream=2
    };
private:
    friend class MultiplexedSocket;
    friend class TCPSetCallbacks;
    ///The low level boost::asio io service handle
    IOService* mIO;
    ///the shared pointer to the communal sending connection
    std::tr1::shared_ptr<MultiplexedSocket> mSocket;
    ///A function to add callbacks to this particular stream, called by the relevant TCPSetCallbacks function inheriting from SetCallbacks
    void addCallbacks(Callbacks*);
    ///The streamID that must be prepended to the data within any packet sent and all received packets for this Stream
    StreamID mID;
    enum {
    ///A bit flag indicating that the socket is being shut down and no further sends may proceed
        SendStatusClosing=(1<<29)
    };
    ///incremented while sending: or'd in SendStatusClosing when close function triggered so no further packets will be sent using old ID.
    std::tr1::shared_ptr<AtomicValue<int> >mSendStatus;
public:
    ///Atomically sets the sendStatus for this socket to closed. FIXME: should use atomic compare and swap for |= instead of += right now only supports 2 non-io threads closing at once
    static bool closeSendStatus(AtomicValue<int>&vSendStatus);
    ///Returns the active stream ID
    StreamID getID()const {return mID;}
    /**
     * A pair of callbacks related to a stream. Used for a target of a map type
     * Holds the callback to receive for a connection and the bytes received callback
     */
    class Callbacks:public Noncopyable {        
    public:
        Stream::ConnectionCallback mConnectionCallback;
        Stream::BytesReceivedCallback mBytesReceivedCallback;
        std::tr1::weak_ptr<AtomicValue<int> > mSendStatus;
        Callbacks(const Stream::ConnectionCallback &connectionCallback,
                  const Stream::BytesReceivedCallback &bytesReceivedCallback,
                  const std::tr1::weak_ptr<AtomicValue<int> >&sendStatus):
            mConnectionCallback(connectionCallback),
            mBytesReceivedCallback(bytesReceivedCallback),
            mSendStatus(sendStatus){
        }
    };
    ///Constructor which leaves socket in a disconnection state, prepared for a connect() or a clone()
    TCPStream(IOService&);
    ///Constructor which brings the socket up to speed in a completely connected state, prepped with a StreamID and communal link pointer
    TCPStream(const std::tr1::shared_ptr<MultiplexedSocket> &shared_socket, const Stream::StreamID&);
    virtual Stream*factory();
    ///Implementation of send interface
    virtual void send(MemoryReference, StreamReliability);
    ///Implementation of send interface
    virtual void send(MemoryReference, MemoryReference, StreamReliability);
    ///Implementation of send interface
    virtual void send(const Chunk&data,StreamReliability);
    ///Implementation of connect interface
    virtual void connect(
        const Address& addy,
        const SubstreamCallback &substreamCallback,
        const ConnectionCallback &connectionCallback,
        const BytesReceivedCallback&chunkReceivedCallback);
    /**
     * Will specify all callbacks for the first successful stream and allow this stream to be cloned
     * The stream is immediately active and may have bytes sent on it immediately. 
     * A connectionCallback will be called as soon as connection has succeeded or failed
     */
    virtual void prepareOutboundConnection(
        const SubstreamCallback &substreamCallback,
        const ConnectionCallback &connectionCallback,
        const BytesReceivedCallback&chunkReceivedCallback);
    /**
     * Will attempt to connect to the given provided address, specifying all callbacks for the first successful stream
     * A connectionCallback specified in prepareConnection will be called as soon as connection has succeeded or failed
     * must call prepareConnection prior to connect()
     */
    virtual void connect(
        const Address& addy);


    ///Creates a new substream on this connection
    virtual Stream* clone(const SubstreamCallback&cb);
    ///Creates a new substream on this connection. This is for when the callbacks do not require the Stream*
    virtual Stream* clone(const ConnectionCallback &connectionCallback,
                          const BytesReceivedCallback&chunkReceivedCallback);
    //Shuts down the socket, allowing StreamID to be reused and opposing stream to get disconnection callback
    virtual void close();
    ~TCPStream();
};
} }
#endif
