/*  Sirikata Network Utilities
 *  ASIOSocketWrapper.cpp
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


#include "util/Standard.hh"
#include "TCPDefinitions.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"

namespace Sirikata { namespace Network {
const uint32 ASIOSocketWrapper::ASYNCHRONOUS_SEND_FLAG;
const uint32 ASIOSocketWrapper::QUEUE_CHECK_FLAG;
const size_t ASIOSocketWrapper::PACKET_BUFFER_SIZE;
void copyHeader(void * destination, const UUID&key, unsigned int num) {
    std::memcpy(destination,TCPStream::STRING_PREFIX(),TCPStream::STRING_PREFIX_LENGTH);
    ((char*)destination)[TCPStream::STRING_PREFIX_LENGTH]='0'+(num/10)%10;
    ((char*)destination)[TCPStream::STRING_PREFIX_LENGTH+1]='0'+(num%10);
    std::memcpy(((char*)destination)+TCPStream::STRING_PREFIX_LENGTH+2,
                key.getArray().begin(),
                UUID::static_size);
}

void ASIOSocketWrapper::sendProtocolHeader(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const UUID&value, unsigned int numConnections) {
    UUID return_value=UUID::random();
    
    Chunk *headerData=new Chunk(TCPStream::TcpSstHeaderSize);
    copyHeader(&*headerData->begin(),value,numConnections);
    rawSend(parentMultiSocket,headerData);
}

} }
