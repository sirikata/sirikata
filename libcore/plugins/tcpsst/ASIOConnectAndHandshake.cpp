/*  Sirikata Network Utilities
 *  ASIOConnectAndHandshake.cpp
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

#include "util/Platform.hpp"
#include "network/Asio.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "ASIOReadBuffer.hpp"
#include "TCPStream.hpp"
#include "MultiplexedSocket.hpp"
#include "ASIOConnectAndHandshake.hpp"
namespace Sirikata { namespace Network {
using namespace boost::asio::ip;
void ASIOConnectAndHandshake::checkHeaderContents(unsigned int whichSocket,
                                                  Array<uint8,TCPStream::TcpSstHeaderSize>* buffer,
                                                  const ErrorCode&error,
                                                  std::size_t bytes_received) {
    std::tr1::shared_ptr<MultiplexedSocket> connection=mConnection.lock();
    if (connection) {
        if (mFinishedCheckCount==(int)connection->numSockets()) {
            mFirstReceivedHeader=*buffer;
        }
        if (mFinishedCheckCount>=1) {
            if (mFirstReceivedHeader!=*buffer) {
                connection->connectionFailedCallback(whichSocket,"Bad header comparison "
                                                     +std::string((char*)buffer->begin(),TCPStream::TcpSstHeaderSize)
                                                     +" does not match "
                                                     +std::string((char*)mFirstReceivedHeader.begin(),TCPStream::TcpSstHeaderSize));
                mFinishedCheckCount-=connection->numSockets();
                mFinishedCheckCount-=1;
            }else {
                mFinishedCheckCount--;
                if (mFinishedCheckCount==0) {
                    connection->connectedCallback();
                }
                MakeASIOReadBuffer(connection,whichSocket);
            }
        }else {
            mFinishedCheckCount-=1;
        }
    }
    delete buffer;
}

void ASIOConnectAndHandshake::connectToIPAddress(const std::tr1::shared_ptr<ASIOConnectAndHandshake>&thus,
                                                 unsigned int whichSocket,
                                                 const tcp::resolver::iterator &it,
                                                 const ErrorCode &error) {
    std::tr1::shared_ptr<MultiplexedSocket> connection=thus->mConnection.lock();
    if (!connection) {
        return;
    }
    if (error) {
        if (it == tcp::resolver::iterator()) {
            //this checks if anyone else has failed
            if (thus->mFinishedCheckCount>=1) {
                //We're the first to fail, decrement until negative
                thus->mFinishedCheckCount-=connection->numSockets();
                thus->mFinishedCheckCount-=1;
                connection->connectionFailedCallback(whichSocket,error);
            }else {
                //keep it negative, indicate one further failure
                thus->mFinishedCheckCount-=1;
            }
        }else {
            tcp::resolver::iterator nextIterator=it;
            ++nextIterator;
            connection->getASIOSocketWrapper(whichSocket).getSocket()
                .async_connect(*it,
                               boost::bind(&ASIOConnectAndHandshake::connectToIPAddress,
                                           thus,
                                           whichSocket,
                                           nextIterator,
                                           boost::asio::placeholders::error));
        }
    } else {
        connection->getASIOSocketWrapper(whichSocket).getSocket()
            .set_option(tcp::no_delay(true));
        connection->getASIOSocketWrapper(whichSocket)
            .sendProtocolHeader(connection,
                                thus->mHeaderUUID,
                                connection->numSockets());
        Array<uint8,TCPStream::TcpSstHeaderSize> *header=new Array<uint8,TCPStream::TcpSstHeaderSize>;
        boost::asio::async_read(connection->getASIOSocketWrapper(whichSocket).getSocket(),
                                boost::asio::buffer(header->begin(),TCPStream::TcpSstHeaderSize),
                                boost::asio::transfer_at_least(TCPStream::TcpSstHeaderSize),
                                boost::bind(&ASIOConnectAndHandshake::checkHeader,
                                            thus,
                                            whichSocket,
                                            header,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }
}

void ASIOConnectAndHandshake::handleResolve(const std::tr1::shared_ptr<ASIOConnectAndHandshake>&thus,
                                            const boost::system::error_code &error,
                                            tcp::resolver::iterator it) {
    std::tr1::shared_ptr<MultiplexedSocket> connection=thus->mConnection.lock();
    if (!connection) {
        return;
    }
    if (error) {
        connection->connectionFailedCallback(error);
    }else {
        unsigned int numSockets=connection->numSockets();
        for (unsigned int whichSocket=0;whichSocket<numSockets;++whichSocket) {
            connectToIPAddress(thus,
                               whichSocket,
                               it,
                               boost::asio::error::host_not_found);
        }
    }

}

void ASIOConnectAndHandshake::connect(const std::tr1::shared_ptr<ASIOConnectAndHandshake> &thus,
                                      const Address&address){
    tcp::resolver::query query(tcp::v4(), address.getHostName(), address.getService());
    thus->mResolver.async_resolve(query,
                                  boost::bind(&ASIOConnectAndHandshake::handleResolve,
                                              thus,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::iterator));

}

ASIOConnectAndHandshake::ASIOConnectAndHandshake(const std::tr1::shared_ptr<MultiplexedSocket> &connection,
                                                 const UUID&sharedUuid):
    mResolver(connection->getASIOService()),
        mConnection(connection),
        mFinishedCheckCount(connection->numSockets()),
        mHeaderUUID(sharedUuid) {
}


} }
