/*  Sirikata
 *  LocalServerIDMap.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include <sirikata/core/network/Address4.hpp>
#include "LocalServerIDMap.hpp"
#include <boost/lexical_cast.hpp>

namespace Sirikata {


LocalServerIDMap::LocalServerIDMap(const String& server_host, uint16 server_port)
 : mID(ServerID(1)),
   mAddress(Network::Address(server_host, boost::lexical_cast<String>(server_port)))
{
}

ServerID *LocalServerIDMap::lookupInternal(const Address4& address) {
    SILOG(local_serverid_map,detailed,"[LocalServerIDMap] Tried to look up internal address in LocalServerIDMap.");
    return NULL;
}

Address4 *LocalServerIDMap::lookupInternal(const ServerID& server_id) {
    SILOG(local_serverid_map,detailed,"[LocalServerIDMap] Tried to look up internal address in LocalServerIDMap.");
    return NULL;
}

ServerID *LocalServerIDMap::lookupExternal(const Address4& address) {
    if (address != mAddress) {
        SILOG(local_serverid_map,detailed,"[LocalServerIDMap] External address lookup does not match known address.");
        return NULL;
    }
    return &mID;
}

Address4 *LocalServerIDMap::lookupExternal(const ServerID& server_id) {
    if (server_id != mID) {
        SILOG(local_serverid_map,detailed,"[LocalServerIDMap] External server lookup does not match known ServerID.");
        return NULL;
    }
    return &mAddress;
}

}
