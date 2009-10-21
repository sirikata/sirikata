/*  Sirikata Network Utilities
 *  IOStrand.cpp
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

#include "util/Standard.hh"
#include "AsioStrandIO.hpp"
#include "IOService.hpp"
#include "IOStrand.hpp"

namespace Sirikata {
namespace Network {

StrandTCPSocket::StrandTCPSocket(IOService& io)
 : TCPSocket(io),
   mStrand(NULL)
{
}

StrandTCPSocket::StrandTCPSocket(IOStrand* strand)
 : TCPSocket(strand->service()),
   mStrand(strand)
{
}

void StrandTCPSocket::bind(IOStrand* strand) {
    mStrand = strand;
}

void StrandTCPSocket::unbind() {
    mStrand = NULL;
}

void StrandTCPSocket::async_connect(const endpoint_type& peer_endpoint, ConnectHandler handler) {
    if (mStrand != NULL)
        TCPSocket::async_connect(peer_endpoint, StrandWrapper::wrap(mStrand, handler));
    else
        TCPSocket::async_connect(peer_endpoint, handler);
}


} // namespace Network
} // namespace Sirikata
