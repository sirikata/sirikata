/*  Sirikata Network Utilities
 *  ASIOConnectAndHandshake.hpp
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
namespace Sirikata { namespace Network {

class ASIOConnectAndHandshake{
    boost::asio::ip::tcp::resolver mResolver;
    std::tr1::weak_ptr<MultiplexedSocket> mConnection;
    ///num positive checks remaining (or -n for n sockets of which at least 1 failed)
    int mFinishedCheckCount;
    UUID mHeaderUUID;
    Array<uint8,TCPStream::TcpSstHeaderSize> mFirstReceivedHeader;
    typedef boost::system::error_code ErrorCode;
                            
   /**
    * This function checks a particular sockets initial handshake header.
    * If this is the first header read, it will save it for comparison
    * If this is the nth header read and everythign is successful it will decrement the first header check integer
    * If anything goes wrong and it is the first time, it will decrement the first header check integer below zero to indiate error and call connectionFailed
    * If anything goes wrong and the first header check integer is already below zero it will decline to take action
    * The buffer passed in will be deleted by this function
    */
    void checkHeaderContents(unsigned int whichSocket, 
                             Array<uint8,TCPStream::TcpSstHeaderSize>* buffer, 
                             const ErrorCode&error,
                             std::size_t bytes_received);
    /**
     * This function simply wraps checkHeaderContents having been passed a shared_ptr from an asio_callback
     */
    static void checkHeader(const std::tr1::shared_ptr<ASIOConnectAndHandshake>&thus, 
                            unsigned int whichSocket, 
                            Array<uint8,TCPStream::TcpSstHeaderSize>* buffer, 
                            const ErrorCode&error,
                            std::size_t bytes_received) {
        thus->checkHeaderContents(whichSocket,buffer,error,bytes_received);
    }

   /**
    * This function checks if a particular sockets has connected to its destination IP address
    * If everything is successful it will decrement the first header check integer
    * If the last resolver fails and it is the first time, it will decrement the first header check integer below zero to indiate error and call connectionFailed
    * If anything goes wrong and the first header check integer is already below zero it will decline to take action
    * The buffer passed in will be deleted by this function
    */
    static void connectToIPAddress(const std::tr1::shared_ptr<ASIOConnectAndHandshake>&thus,
                                   unsigned int whichSocket,
                                   const boost::asio::ip::tcp::resolver::iterator &it,
                                   const ErrorCode &error);
   /**
    * This function is a callback from the async_resolve call from ASIO initialized from the public interface connect
    * It may get an error if the host was not found or otherwise a valid iterator to a number of ip addresses
    */
    static void handleResolve(const std::tr1::shared_ptr<ASIOConnectAndHandshake>&thus,
                              const boost::system::error_code &error,
                              boost::asio::ip::tcp::resolver::iterator it);
public:
    /**
     *  This function transforms the member mConnection from the PRECONNECTION socket phase to the CONNECTED socket phase
     *  It first performs a resolution on the address and handles the callback in handleResolve. 
     *  If the header checks out and matches with the other live sockets to the same sockets 
     *    - MultiplexedSocket::connectedCallback() is called
     *    - An ASIOReadBuffer is created for handling future reads
     */
    static void connect(const std::tr1::shared_ptr<ASIOConnectAndHandshake> &thus,
                        const Address&address);
    ASIOConnectAndHandshake(const std::tr1::shared_ptr<MultiplexedSocket> &connection,
                            const UUID&sharedUuid);
};
} }
