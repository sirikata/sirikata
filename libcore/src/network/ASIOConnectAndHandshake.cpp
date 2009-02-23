#include "util/Standard.hh"
#include "TCPDefinitions.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "TCPStream.hpp"
#include "MultiplexedSocket.hpp"
#include "ASIOConnectAndHandshake.hpp"
namespace Sirikata { namespace Network {
using namespace boost::asio::ip;
ASIOConnectAndHandshake::ASIOConnectAndHandshake(const std::tr1::shared_ptr<MultiplexedSocket> &connection,
                                                 const UUID&sharedUuid):
    mResolver(connection->getASIOService()),
        mConnection(connection),
        mFinishedCheckCount(connection->numSockets()),
        mHeaderUUID(sharedUuid) {
}

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
                MakeTCPReadBuffer(connection,whichSocket);
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
        connection->connectionFailedCallback(error.message());
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

} }
