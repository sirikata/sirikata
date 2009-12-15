/*  Sirikata Network Utilities
 *  TCPStreamListener.hpp
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

#ifndef SIRIKATA_TCPStreamListener_HPP__
#define SIRIKATA_TCPStreamListener_HPP__

#include "network/IODefs.hpp"
#include "network/StreamListener.hpp"

namespace Sirikata {
namespace Network {

/**
 * This class waits on a service and listens for incoming connections
 * It calls the callback whenever such connections are encountered
 */
class TCPStreamListener : public StreamListener {
public:
    TCPStreamListener(IOService&, OptionSet*);
    virtual ~TCPStreamListener();

    static TCPStreamListener* construct(Network::IOService*io, OptionSet*options) {
        return new TCPStreamListener(*io,options);
    }

    virtual bool listen(const Address&addr, const Stream::SubstreamCallback&newStreamCallback);
    virtual String listenAddressName()const;
    virtual Address listenAddress()const;
    virtual void close();

private:
    struct Data; // Data which may be needed in callbacks, so is stored separately in shared_ptr
    typedef std::tr1::shared_ptr<Data> DataPtr;
    DataPtr mData;
    uint8 mMaxSimultaneousSockets;
    uint32 mSendBufferSize;
};

} // namespace Network
} // namespace Sirikata


#endif
