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

class TCPStream:public Stream {
public:
    class Callbacks;
private:
    friend class MultiplexedSocket;
    IOService* mIO;
    std::tr1::shared_ptr<MultiplexedSocket> mSocket;
    void addCallbacks(Callbacks*);
    StreamID mID;
    static const int SendStatusClosing;
    ///incremented while sending: or'd in SendStatusClosing when close function triggered so no further packets will be sent using old ID.
    AtomicValue<int>mSendStatus;
public:
    StreamID getID()const {return mID;}
    class Callbacks:public Noncopyable {        
    public:
        Stream::ConnectionCallback mConnectionCallback;
        Stream::BytesReceivedCallback mBytesReceivedCallback;
        Callbacks(const Stream::ConnectionCallback &connectionCallback,
                  const Stream::BytesReceivedCallback &bytesReceivedCallback):
            mConnectionCallback(connectionCallback),mBytesReceivedCallback(bytesReceivedCallback){
        }
    };
    TCPStream(IOService&);
    TCPStream(const std::tr1::shared_ptr<MultiplexedSocket> &shared_socket, const Stream::StreamID&);
public:
    virtual void send(const Chunk&data,Reliability);
    virtual void connect(
        const Address& addy,
        const SubstreamCallback &substreamCallback,
        const ConnectionCallback &connectionCallback,
        const BytesReceivedCallback&chunkReceivedCallback);
    ///Creates a stream of the same type as this stream, with the same IO factory
    Stream* factory();
    ///Creates a new substream on this connection
    virtual bool cloneFrom(Stream*,
        const ConnectionCallback &connectionCallback,
        const BytesReceivedCallback&chunkReceivedCallback);

    virtual void close();
};
} }
#endif
