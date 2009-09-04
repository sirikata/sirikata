/*  cbr
 *  ObjectConnection.hpp
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

#ifndef _CBR_OBJECT_CONNECTION_HPP_
#define _CBR_OBJECT_CONNECTION_HPP_

#include "Utility.hpp"
#include "Message.hpp"
#include "ObjectHostConnectionManager.hpp"

namespace CBR {

class Trace;

/** Represents a connection a space has to an object.
 *  Only valid while a valid network connection to the object
 *  is open.
 */
class ObjectConnection {
public:
    ObjectConnection(const UUID& _id, ObjectHostConnectionManager* conn_mgr, const ObjectHostConnectionManager::ConnectionID& conn_id);
    ~ObjectConnection();

    // Get the UUID of the object associated with this connection.
    UUID id() const;

    void send(CBR::Protocol::Object::ObjectMessage* msg);

    void service();

    // Returns true if no messages are waiting to be delivered
    bool empty() const { return mReceiveQueue.empty(); }

private:
    UUID mID;
    ObjectHostConnectionManager* mConnectionManager;
    ObjectHostConnectionManager::ConnectionID mOHConnection;

    typedef std::vector<CBR::Protocol::Object::ObjectMessage*> MessageQueue;
    MessageQueue mReceiveQueue;
};

} // namespace CBR

#endif //_CBR_OBJECT_CONNECTION_HPP_
