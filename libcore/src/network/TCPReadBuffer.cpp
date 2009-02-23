/*  Sirikata Network Utilities
 *  TCPReadBuffer.cpp
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
#include "Stream.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"
#include "TCPReadBuffer.hpp"
namespace Sirikata { namespace Network {
void MakeTCPReadBuffer(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket,unsigned int whichSocket) {
    new TCPReadBuffer(parentSocket,whichSocket);
}
void TCPReadBuffer::processError(MultiplexedSocket*parentSocket, const boost::system::error_code &error){
    parentSocket->hostDisconnectedCallback(mWhichBuffer,error);
    delete this;
}
void TCPReadBuffer::processFullChunk(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket, unsigned int whichSocket, const Stream::StreamID&id, const Chunk&newChunk){
    parentSocket->receiveFullChunk(whichSocket,id,newChunk);
}
void TCPReadBuffer::readIntoFixedBuffer(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket){
    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(mBuffer+mBufferPos,sBufferLength-mBufferPos),
                       boost::bind(&TCPReadBuffer::asioReadIntoFixedBuffer,
                                   this,
                                   boost::asio::placeholders::error,
                                   boost::asio::placeholders::bytes_transferred));
}
void TCPReadBuffer::readIntoChunk(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket){
    assert(mNewChunk.size()>0);//otherwise should have been filtered out by caller
    assert(mBufferPos<mNewChunk.size());
    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(&*(mNewChunk.begin()+mBufferPos),mNewChunk.size()-mBufferPos),
                       boost::bind(&TCPReadBuffer::asioReadIntoChunk,
                                   this,
                                   boost::asio::placeholders::error,
                                   boost::asio::placeholders::bytes_transferred));
}
} }
