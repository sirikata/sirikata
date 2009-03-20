/*  cbr
 *  Server.hpp
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

#ifndef _CBR_SERVER_HPP_
#define _CBR_SERVER_HPP_

#include "Utility.hpp"
#include "Time.hpp"
#include "Proximity.hpp"
#include "LocationService.hpp"


namespace CBR {

class Proximity;
class Object;
class ObjectFactory;
class ServerIDMap;
class CoordinateSegmentation;
class Message;
class ObjectToObjectMessage;
class SendQueue;
class Network;
class Trace;
class MigrateMessage;

/** Handles all the basic services provided for objects by a server,
 *  including routing and message delivery, proximity services, and
 *  object -> server mapping.  This is a singleton for each simulated
 *  server.  Other servers are referenced by their ServerID.
 */
class Server {
public:
    Server(ServerID id, ObjectFactory* obj_factory, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, Network* net, SendQueue*sq, Trace* trace);
    ~Server();

    const ServerID& id() const;

    // Routing interface for objects.  Destination server will come from the message.
    // Servers should use the private route method which follows different rules.
    void route(ObjectToObjectMessage* msg);

    void tick(const Time& t);
private:
    void proximityTick(const Time& t);
    void networkTick(const Time& t);
    void checkObjectMigrations();
    void processChunk(const Sirikata::Network::Chunk&chunk, bool forwarded_self_msg);
    // Routing interface for servers.  This is used to route messages that originate from
    // a server provided service, and thus don't have a source object.  Messages may be destined
    // for either servers or objects.  The second form will simply automatically do the destination
    // server lookup.
    // if forwarding is true the message will be stuck onto a queue no matter what, otherwise it may be delivered directly
    void route(Message* msg, const ServerID& dest_server, bool is_forward = false);
    void route(Message* msg, const UUID& dest_obj, bool is_forward = false);
    void route(Message* msg, const ServerID& dest_server, const UUID& src_uuid, bool is_forward = false);

    // Delivery interface.  This should be used to deliver received messages to the correct location -
    // the server or object it is addressed to.
    void deliver(Message* msg);
    // Get the object pointer for the given ID, or NULL if it isn't available on this server.
    Object* object(const UUID& dest) const;
    // Forward the given message to its proper server.  Use this when a message arrives and the object
    // no longer lives on this server.
    void forward(Message* msg, const UUID& dest);

    MigrateMessage* wrapObjectStateForMigration(Object* obj);

    ServerID lookup(const Vector3f&);
    ServerID lookup(const UUID&);

    typedef std::map<UUID, Object*> ObjectMap;
    ServerID mID;
    ObjectMap mObjects;
    ObjectFactory* mObjectFactory;
    LocationService* mLocationService;
    CoordinateSegmentation* mCSeg;
    Proximity* mProximity;
    Network * mNetwork;
    SendQueue* mSendQueue;

    struct SelfMessage {
        SelfMessage(const Network::Chunk& d, bool f)
         : data(d), forwarded(f) {}

        Network::Chunk data;
        bool forwarded;
    };
    std::deque<SelfMessage>mSelfMessages;

    Time mCurrentTime;
    Trace* mTrace;
}; // class Server

} // namespace CBR

#endif //_CBR_SERVER_HPP_
