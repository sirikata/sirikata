/*  cbr
 *  ServerNetworkImpl.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_SERVER_NETWORK_IMPL_HPP_
#define _CBR_SERVER_NETWORK_IMPL_HPP_

#include "ServerNetwork.hpp"

namespace CBR {

// Server to server routing header
class ServerMessageHeader {
public:
    ServerMessageHeader(const ServerID& src_server, const ServerID& dest_server)
     : mSourceServer(src_server),
       mDestServer(dest_server)
    {
    }

    const ServerID& sourceServer() const {
        return mSourceServer;
    }

    const ServerID& destServer() const {
        return mDestServer;
    }

    // Serialize this header into the network chunk, starting at the given offset.
    // Returns the ending offset of the header.
    uint32 serialize(Network::Chunk& wire, uint32 offset) {
        if (wire.size() < offset + 2 * sizeof(ServerID) )
            wire.resize( offset + 2 * sizeof(ServerID) );

        memcpy( &wire[offset], &mSourceServer, sizeof(ServerID) );
        offset += sizeof(ServerID);

        memcpy( &wire[offset], &mDestServer, sizeof(ServerID) );
        offset += sizeof(ServerID);

        return offset;
    }

    static ServerMessageHeader deserialize(const Network::Chunk& wire, uint32 &offset) {
        ServerID raw_source;
        ServerID raw_dest;

        memcpy( &raw_source, &wire[offset], sizeof(ServerID) );
        offset += sizeof(ServerID);

        memcpy( &raw_dest, &wire[offset], sizeof(ServerID) );
        offset += sizeof(ServerID);

        return ServerMessageHeader(raw_source, raw_dest);
    }
private:
    ServerMessageHeader();

    ServerID mSourceServer;
    ServerID mDestServer;
}; // class ServerMessageHeader


} // namespace CBR

#endif //_CBR_SERVER_NETWORK_IMPL_HPP_
