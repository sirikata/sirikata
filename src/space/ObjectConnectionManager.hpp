/*  cbr
 *  ObjectConnectionManager.hpp
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

#ifndef _CBR_OBJECT_CONNECTION_MANAGER_HPP_
#define _CBR_OBJECT_CONNECTION_MANAGER_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Network.hpp"
#include <boost/asio.hpp>

namespace CBR {

/** ObjectConnectionManager handles the networking aspects of interacting
 *  with object hosts.  It listens for connections, maintains per object
 *  connections, and handles shipping messages out to the network.
 */
class ObjectConnectionManager {
public:
    ObjectConnectionManager(SpaceContext* ctx, const Address4& listen_addr);
    ~ObjectConnectionManager();

    void service();
private:
    SpaceContext* mContext;

    boost::asio::io_service mIOService;
    boost::asio::ip::tcp::acceptor* mAcceptor;

    struct ObjectHostConnection {
        ObjectHostConnection(boost::asio::io_service& ios);
        ~ObjectHostConnection();

        boost::asio::ip::tcp::socket* socket;
        std::queue<std::string*> queue;

        bool is_writing;

        boost::asio::streambuf read_buf;
        std::string read_avail;
        boost::asio::streambuf write_buf;
    };
    typedef std::set<ObjectHostConnection*> ObjectHostConnectionMap;
    ObjectHostConnectionMap mConnections;


    /** Listen for and handle new connections. */
    void listen(const Address4& listen_addr); // sets up the acceptor, starts the listening cycle
    void startListening(); // initiates listening for next connection
    void handleNewConnection(const boost::system::error_code& error, ObjectHostConnection* new_conn);

    /** Reading and writing handling for ObjectHostConnections. */

    // Start async reading for this connection
    void startReading(ObjectHostConnection* conn);
    // Handle async reading callbacks for this connection
    void handleConnectionRead(const boost::system::error_code& err, ObjectHostConnection* conn);

    // Start async writing for this connection if it has data to be sent
    void startWriting(ObjectHostConnection* conn);
    // Handle the async writing callback for this connection
    void handleConnectionWrite(const boost::system::error_code& err, ObjectHostConnection* conn);
};

} // namespace CBR

#endif //_CBR_OBJECT_CONNECTION_MANAGER_HPP_
