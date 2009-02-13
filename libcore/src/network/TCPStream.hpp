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
namespace Sirikata { namespace Network {
class MultiplexedSocket;
class TCPStreamBuilder;
class TCPStream:public Stream {
    friend class MultiplexedSocket;
    friend class TCPStreamBuilder;
    std::tr1::shared_ptr<MultiplexedSocket> mSocket;
    void addCallbacks(Callbacks*);
    StreamID mID;
    StreamID getID()const {return mID;}
public:
    TCPStream(IOService&);
    TCPStream(const std::tr1::shared_ptr<MultiplexedSocket> &shared_socket);
public:
    virtual void send(const Chunk&data,Reliability);
    virtual void connect(
        const Address& addy,
        const ConnectionCallback &connectionCallback,
        const SubstreamCallback &substreamCallback,
        const BytesReceivedCallback&chunkReceivedCallback);
    ///Creates a new substream on this connection
    virtual Stream*clone(
        const ConnectionCallback &connectionCallback,
        const SubstreamCallback &substreamCallback,
        const BytesReceivedCallback&chunkReceivedCallback);

    virtual void close();
};
} }
#endif
