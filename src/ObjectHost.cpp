/*  cbr
 *  ObjectHost.cpp
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

#include "ObjectHost.hpp"
#include "Statistics.hpp"
#include "Object.hpp"
#include "ObjectFactory.hpp"
#include "sirikata/network/StreamFactory.hpp"
#include "sirikata/network/Stream.hpp"
#include "sirikata/util/PluginManager.hpp"
#include "ServerIDMap.hpp"
#include "Random.hpp"
#include "Options.hpp"
#include <boost/bind.hpp>

#define OH_LOG(level,msg) SILOG(oh,level,"[OH] " << msg)

using namespace Sirikata;
using namespace Sirikata::Network;

namespace CBR {

ObjectHost::SpaceNodeConnection::SpaceNodeConnection(ObjectHostContext* ctx, OptionSet* streamOptions, ServerID sid)
 : server(sid),
   socket(Sirikata::Network::StreamFactory::getSingleton().getConstructor(GetOption("ohstreamlib")->as<String>())(ctx->ioService,streamOptions)),
   queue(16*1024 /* FIXME */, std::tr1::bind(&ObjectMessage::size, std::tr1::placeholders::_1)),
   rateLimiter(1024*1024),
   streamTx(socket, ctx->mainStrand, Duration::milliseconds((int64)0))
{
    static Sirikata::PluginManager sPluginManager;
    static int tcpSstLoaded=(sPluginManager.load(Sirikata::DynamicLibrary::filename(GetOption("ohstreamlib")->as<String>())),0);

    connecting = false;

    queue.connect(0, &streamTx, 0);
}

ObjectHost::SpaceNodeConnection::~SpaceNodeConnection() {
    delete socket;
}

ObjectHost::ObjectInfo::ObjectInfo()
 : object(NULL),
   connectingTo(0),
   connectedTo(0),
   migratingTo(0)
{
}

ObjectHost::ObjectInfo::ObjectInfo(Object* obj)
 : object(obj),
   connectingTo(0),
   connectedTo(0),
   migratingTo(0)
{
}


ObjectHost::ObjectHost(ObjectHostContext* ctx, ObjectFactory* obj_factory, Trace* trace, ServerIDMap* sidmap)
 : PollingService(ctx->mainStrand),
   mContext( ctx ),
   mServerIDMap(sidmap)
{
    static Sirikata::PluginManager sPluginManager;
    static int tcpSstLoaded=(sPluginManager.load(Sirikata::DynamicLibrary::filename(GetOption("ohstreamlib")->as<String>())),0);

    mStreamOptions=Sirikata::Network::StreamFactory::getSingleton().getOptionParser(GetOption("ohstreamlib")->as<String>())(GetOption("ohstreamoptions")->as<String>());

    mProfiler = mContext->profiler->addStage("Object Host Tick");

    mLastRRObject=UUID::null();
    mPingId=0;
    mContext->objectHost = this;
}

const ObjectHostContext* ObjectHost::context() const {
    return mContext;
}

void ObjectHost::openConnection(Object* obj, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, const SolidAngle& init_sa, ConnectedCallback cb) {
    openConnection(obj, init_loc, init_bounds, true, init_sa, cb);
}

void ObjectHost::openConnection(Object* obj, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, ConnectedCallback cb) {
    openConnection(obj, init_loc, init_bounds, false, SolidAngle::Max, cb);
}

void ObjectHost::openConnection(Object* obj, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, bool regQuery, const SolidAngle& init_sa, ConnectedCallback cb) {
    using std::tr1::placeholders::_1;

    // Make sure we have this object's info stored
    ObjectInfoMap::iterator it = mObjectInfo.find(obj->uuid());
    if (it == mObjectInfo.end()) {
        it = mObjectInfo.insert( ObjectInfoMap::value_type( obj->uuid(), ObjectInfo(obj) ) ).first;
    }
    mObjectInfo[obj->uuid()].connecting.loc = init_loc;
    mObjectInfo[obj->uuid()].connecting.bounds = init_bounds;
    mObjectInfo[obj->uuid()].connecting.regQuery = regQuery;
    mObjectInfo[obj->uuid()].connecting.queryAngle = init_sa;
    mObjectInfo[obj->uuid()].connecting.cb = cb;

    // Get a connection to request
    getSpaceConnection(
        std::tr1::bind(&ObjectHost::openConnectionStartSession, this, obj->uuid(), _1)
    );
}
void ObjectHost::retryOpenConnection(const UUID&uuid,ServerID sid) {
    getSpaceConnection(
        sid,
        std::tr1::bind(&ObjectHost::openConnectionStartSession, this, uuid, _1)
    );
}
void ObjectHost::openConnectionStartSession(const UUID& uuid, SpaceNodeConnection* conn) {
    if (conn == NULL) {
        OH_LOG(warn,"Couldn't initiate connection for " << uuid.toString());
        // FIXME disconnect? retry?
        mObjectInfo[uuid].connecting.cb(NullServerID);
        return;
    }

    // Send connection msg, store callback info so it can be called when we get a response later in a service call
    mObjectInfo[uuid].connectingTo = conn->server;

    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    connect_msg.set_type(CBR::Protocol::Session::Connect::Fresh);
    connect_msg.set_object(uuid);
    CBR::Protocol::Session::ITimedMotionVector loc = connect_msg.mutable_loc();
    loc.set_t( mObjectInfo[uuid].connecting.loc.updateTime() );
    loc.set_position( mObjectInfo[uuid].connecting.loc.position() );
    loc.set_velocity( mObjectInfo[uuid].connecting.loc.velocity() );
    connect_msg.set_bounds( mObjectInfo[uuid].connecting.bounds );
    if (mObjectInfo[uuid].connecting.regQuery)
        connect_msg.set_query_angle( mObjectInfo[uuid].connecting.queryAngle.asFloat() );

    if (!send(mContext->time,
              uuid, OBJECT_PORT_SESSION,
              UUID::null(), OBJECT_PORT_SESSION,
              serializePBJMessage(session_msg),
              conn->server
            )) {
        printf ("RETRYING CONNECTION\n");
        mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&ObjectHost::retryOpenConnection,this,uuid,conn->server));
    }
}


void ObjectHost::migrate(const UUID& obj_id, ServerID sid) {
    using std::tr1::placeholders::_1;

    OH_LOG(insane,"Starting migration of " << obj_id.toString() << " to " << sid);

    mObjectInfo[obj_id].migratingTo = sid;
    std::vector<UUID>*objects=&mObjectServerMap[mObjectInfo[obj_id].connectedTo];
    std::vector<UUID>::iterator where=std::find(objects->begin(),objects->end(),obj_id);
    assert(where!=objects->end());
    if (where!=objects->end()) {
        objects->erase(where);
    }
    // Get or start the connection we need to start this migration
    getSpaceConnection(
        sid,
        std::tr1::bind(&ObjectHost::openConnectionStartMigration, this, obj_id, sid, _1)
    );
}

void ObjectHost::openConnectionStartMigration(const UUID& obj_id, ServerID sid, SpaceNodeConnection* conn) {
    if (conn == NULL) {
        OH_LOG(warn,"Couldn't open connection to server " << sid << " for migration of object " << obj_id.toString());
        // FIXME disconnect? retry?
        return;
    }

    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    connect_msg.set_type(CBR::Protocol::Session::Connect::Migration);
    connect_msg.set_object(obj_id);
    if (!send(
            mContext->time,
            obj_id, OBJECT_PORT_SESSION,
            UUID::null(), OBJECT_PORT_SESSION,
            serializePBJMessage(session_msg),
            sid
            ))    {
        printf ("RETRYING MIGRATION\n");
        std::tr1::function<void(SpaceNodeConnection*)> retry(std::tr1::bind(&ObjectHost::openConnectionStartMigration,
                                                                          this, 
                                                                          obj_id, 
                                                                          sid, 
                                                                          _1));
        mContext->mainStrand
            ->post(Duration::seconds(.05),
                   std::tr1::bind(&ObjectHost::getSpaceConnection,
                                  this,
                                  sid,
                                  retry));
    }
        
    // FIXME do something on failure
}


ServerID ObjectHost::getConnectedServer(const UUID& obj_id, bool allow_connecting) {
    // FIXME getConnectedServer during migrations?

    ServerID dest_server = mObjectInfo[obj_id].connectedTo;

    if (dest_server == NullServerID && allow_connecting)
        dest_server = mObjectInfo[obj_id].connectingTo;

    return dest_server;
}

bool ObjectHost::send(const Time&t, const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    // FIXME getConnectedServer during migrations?
    return send(t,src->uuid(), src_port, dest, dest_port, payload, getConnectedServer(src->uuid()));
}

bool ObjectHost::send(const Time&t, const UUID& src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server) {
    if (dest_server == NullServerID) {
        OH_LOG(error,"Tried to send message when not connected.");
        return false;
    }

    ServerConnectionMap::iterator it = mConnections.find(dest_server);
    if (it == mConnections.end()) {
        OH_LOG(warn,"Tried to send message for object to unconnected server.");
        return false;
    }
    SpaceNodeConnection* conn = it->second;

    // FIXME would be nice not to have to do this alloc/dealloc
    ObjectMessage* obj_msg = createObjectHostMessage(mContext->id, src, src_port, dest, dest_port, payload);
    mContext->trace()->timestampMessage(t,obj_msg->unique(),Trace::CREATED,src_port, dest_port);
    return conn->queue.push( obj_msg );
}
void ObjectHost::ping(const Object*src, const UUID&dest, double distance) {

    CBR::Protocol::Object::Ping ping_msg;
    Time t=mContext->time;
    ping_msg.set_ping(t);
    if (distance>=0)
        ping_msg.set_distance(distance);
    ping_msg.set_id(mPingId++);
    ping_msg.set_dat1(0);
    ping_msg.set_dat2(0);
    ping_msg.set_dat3(0);
    ping_msg.set_dat4(0);
    ping_msg.set_dat5(0);
    ping_msg.set_dat6(0);
    ping_msg.set_dat7(0);
    ping_msg.set_dat8(0);
    ServerID destServer=getConnectedServer(src->uuid());
    if (destServer!=NullServerID) {
        send(t,src->uuid(),OBJECT_PORT_PING,dest,OBJECT_PORT_PING,serializePBJMessage(ping_msg),destServer);
    }
}
Object*ObjectHost::roundRobinObject(ServerID whichServer) {
    if (mObjectInfo.size()==0) return NULL;
    UUID myrand(mLastRRObject);
    ObjectInfoMap::iterator i=mObjectInfo.end();
    if (whichServer==NullServerID){
        i=mObjectInfo.find(myrand);
        if (i!=mObjectInfo.end()) ++i;
        if (i==mObjectInfo.end()) i=mObjectInfo.begin();
    }else {
        std::vector<UUID> *osm=&mObjectServerMap[whichServer];
        ++mLastRRIndex;
        if (mLastRRIndex>=osm->size()) {
            mLastRRIndex=0;
        }
        if (osm->size()) {
            i=mObjectInfo.find((*osm)[mLastRRIndex]);
        }
    }
    if (i!=mObjectInfo.end()) {
        mLastRRObject=i->first;
        return i->second.object;
    }
    return NULL;
}
Object* ObjectHost::randomObject () {
    if (mObjectServerMap.size()==0) return NULL;
    int iteratorLevel=rand()%mObjectServerMap.size();
    ObjectServerMap::iterator i=mObjectServerMap.begin();
    for (int index=0;index<iteratorLevel;++index,++i) {
    }
    std::vector<UUID> *uuidMap=&i->second;
    if (uuidMap->size()) {
        ObjectInfoMap::iterator i=mObjectInfo.find((*uuidMap)[rand()%uuidMap->size()]);
        if (i!=mObjectInfo.end())
            return i->second.object;
    }
    return NULL;
}

Object* ObjectHost::randomObject (ServerID whichServer) {
    if (whichServer==NullServerID) return randomObject();
    ObjectServerMap::iterator i=mObjectServerMap.find(whichServer);
    if (i==mObjectServerMap.end())
        return NULL;
    std::vector<UUID> *uuidMap=&i->second;
    if (uuidMap->size()) {
        ObjectInfoMap::iterator i=mObjectInfo.find((*uuidMap)[rand()%uuidMap->size()]);
        if (i!=mObjectInfo.end())
            return i->second.object;
    }
    return NULL;
}
void ObjectHost::randomPing(const Time&t) {



    Object * a=randomObject();
    Object * b=randomObject();

    if (a&&b) {
        if (a == NULL || b == NULL) return;
        if (mObjectInfo[a->uuid()].connectedTo == NullServerID ||
            mObjectInfo[b->uuid()].connectedTo == NullServerID)
            return;
        if (mObjectInfo[b->uuid()].connectedTo == mObjectInfo[a->uuid()].connectedTo)
            return;
        ping(a,b->uuid(),(a->location().extrapolate(t).position()-b->location().extrapolate(t).position()).length());
    }
}
void ObjectHost::sendTestMessage(const Time&t, float idealDistance){
    Vector3f minLocation(1.e6,1.e6,1.e6);
    Vector3f maxLocation=minLocation;
    ObjectInfo * corner=NULL;
    for (ObjectInfoMap::iterator i=mObjectInfo.begin(),ie=mObjectInfo.end();
         i!=ie;
         ++i) {
        Vector3f l=i->second.object->location().extrapolate(t).position();
        if ((l-maxLocation).lengthSquared()>(minLocation-maxLocation).lengthSquared()) {
            corner=&i->second;
            minLocation=l;
        }
    }
    if (!corner) return;
    double distance=1.e30;
    Vector3f cornerLocation=corner->object->location().extrapolate(t).position();
    double bestDistance=1.e30;
    ObjectInfo * destObject=NULL;
    for (ObjectInfoMap::iterator i=mObjectInfo.begin(),ie=mObjectInfo.end();
         i!=ie;
         ++i) {
        Vector3f l=i->second.object->location().extrapolate(t).position();
        double curDistance=(l-cornerLocation).length();
        if (i->second.object!=corner->object&&(destObject==NULL||fabs(curDistance-distance)<fabs(bestDistance-distance))) {
            bestDistance=curDistance;
            destObject=&i->second;
        }
    }
    if (!destObject) return;
    ping(corner->object,destObject->object->uuid(),bestDistance);
}

void ObjectHost::poll() {
    mProfiler->started();

    //sendTestMessage(t,400.);
    for (int i=0;i<100;++i)
        randomPing(mContext->time);

    mProfiler->finished();
}


void ObjectHost::getSpaceConnection(GotSpaceConnectionCallback cb) {
    // Check if we have any fully open connections we can already use
    if (!mConnections.empty()) {
        for(ServerConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
            SpaceNodeConnection* rand_conn = it->second;
            if (rand_conn == NULL) continue;
            if (!rand_conn->connecting) {
                cb(rand_conn);
                return;
            }
            else {
                rand_conn->connectCallbacks.push_back(cb);
                return;
            }
        }
    }

    // Otherwise, initiate one at random
    ServerID server_id = (ServerID)1; // FIXME should be selected at random somehow, and shouldn't already be in the connection map
    getSpaceConnection(server_id, cb);
}

void ObjectHost::getSpaceConnection(ServerID sid, GotSpaceConnectionCallback cb) {
    // Check if we have the connection already
    ServerConnectionMap::iterator it = mConnections.find(sid);
    if (it != mConnections.end()) {
        cb(it->second);
        return;
    }

    // Otherwise start it up
    setupSpaceConnection(sid, cb);
}

void ObjectHost::setupSpaceConnection(ServerID server, GotSpaceConnectionCallback cb) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    SpaceNodeConnection* conn = new SpaceNodeConnection(mContext, mStreamOptions, server);
    conn->connectCallbacks.push_back(cb);
    mConnections[server] = conn;

    // Lookup the server's address
    Address4* addr = mServerIDMap->lookupExternal(server);
    Address addy(convertAddress4ToSirikata(*addr));
    conn->socket->connect(addy,
                          &Sirikata::Network::Stream::ignoreSubstreamCallback,
                          std::tr1::bind(&ObjectHost::handleSpaceConnection,
                                         this,
                                         _1, _2, server),
                          std::tr1::bind(&ObjectHost::handleConnectionRead,
                                         this,
                                         conn,
                                         _1),
                          &Sirikata::Network::Stream::ignoreReadySend);
    OH_LOG(debug,"Trying to connect to " << addy.toString());
    conn->connecting = true;
}

void ObjectHost::handleSpaceConnection(const Sirikata::Network::Stream::ConnectionStatus status,
                                       const std::string&reason,
                                       ServerID sid) {
     SpaceNodeConnection* conn = mConnections[sid];

    OH_LOG(debug,"Handling space connection...");
    if (status!=Sirikata::Network::Stream::Connected) {
        OH_LOG(error,"Failed to connect to server " << sid << ": " << reason);
        for(std::vector<GotSpaceConnectionCallback>::iterator it = conn->connectCallbacks.begin(); it != conn->connectCallbacks.end(); it++)
            (*it)(NULL);
        conn->connectCallbacks.clear();
        delete conn;
        mConnections.erase(sid);
        return;
    }

    OH_LOG(debug,"Successfully connected to " << sid);
    conn->connecting = false;

    // Tell all requesters that the connection is available
    for(std::vector<GotSpaceConnectionCallback>::iterator it = conn->connectCallbacks.begin(); it != conn->connectCallbacks.end(); it++)
        (*it)(conn);
    conn->connectCallbacks.clear();
}

bool ObjectHost::handleConnectionRead(SpaceNodeConnection* conn, Chunk& chunk) {
    CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
    bool parse_success = obj_msg->ParseFromArray(chunk.data(),chunk.size());
    assert(parse_success == true);

    handleServerMessage( conn, obj_msg );
    return true;
}


void ObjectHost::handleServerMessage(SpaceNodeConnection* conn, CBR::Protocol::Object::ObjectMessage* msg) {
    // As a special case, messages dealing with sessions are handled by the object host
    mContext->trace()->timestampMessage(mContext->time,msg->unique(),Trace::DESTROYED,msg->source_port(),msg->dest_port());
    if (msg->source_object() == UUID::null() && msg->dest_port() == OBJECT_PORT_SESSION) {
        handleSessionMessage(msg);
        return;
    }
    if (msg->source_port()==OBJECT_PORT_PING&&msg->dest_port()==OBJECT_PORT_PING) {

        CBR::Protocol::Object::Ping ping_msg;
        ping_msg.ParseFromString(msg->payload());
        mContext->trace()->ping(ping_msg.ping(),msg->source_object(),mContext->time,msg->dest_object(), ping_msg.has_id()?ping_msg.id():(uint64)-1,ping_msg.has_distance()?ping_msg.distance():-1,msg->unique());
        //std::cerr<<"Ping "<<ping_msg.ping()-Time::now()<<'\n';

        return;
    }

    // Otherwise, by default, we just ship it to the correct object
    mObjectInfo[ msg->dest_object() ].object->receiveMessage(msg);
}

void ObjectHost::handleSessionMessage(CBR::Protocol::Object::ObjectMessage* msg) {
    using std::tr1::placeholders::_1;

    CBR::Protocol::Session::Container session_msg;
    bool parse_success = session_msg.ParseFromString(msg->payload());
    assert(parse_success);

    assert(!session_msg.has_connect());

    if (session_msg.has_connect_response()) {
        CBR::Protocol::Session::ConnectResponse conn_resp = session_msg.connect_response();

        UUID obj = msg->dest_object();

        if (conn_resp.response() == CBR::Protocol::Session::ConnectResponse::Success) {
            if (mObjectInfo[obj].connectingTo != NullServerID) { // We were connecting to a server
                ServerID connectedTo = mObjectInfo[obj].connectingTo;
                OH_LOG(debug,"Successfully connected " << obj.toString() << " to space node " << connectedTo);
                mObjectInfo[obj].connectedTo = connectedTo;
                mObjectInfo[obj].connectingTo = NullServerID;
                mObjectInfo[obj].connecting.cb(connectedTo);
                mObjectInfo[obj].connecting.cb = NULL;
                mObjectServerMap[connectedTo].push_back(obj);
            }
            else if (mObjectInfo[obj].migratingTo != NullServerID) { // We were migrating
                ServerID migratedTo = mObjectInfo[obj].migratingTo;
                OH_LOG(debug,"Successfully migrated " << obj.toString() << " to space node " << migratedTo);
                mObjectInfo[obj].connectedTo = migratedTo;
                mObjectInfo[obj].migratingTo = NullServerID;
                mObjectServerMap[migratedTo].push_back(obj);
            }
            else { // What were we doing?
                OH_LOG(error,"Got connection response with no outstanding requests.");
            }
        }
        else if (conn_resp.response() == CBR::Protocol::Session::ConnectResponse::Redirect) {
            ServerID redirected = conn_resp.redirect();
            OH_LOG(debug,"Object connection for " << obj.toString() << " redirected to " << redirected);
            // Get a connection to request
            getSpaceConnection(
                redirected,
                std::tr1::bind(&ObjectHost::openConnectionStartSession, this, obj, _1)
            );

        }
        else if (conn_resp.response() == CBR::Protocol::Session::ConnectResponse::Error) {
            OH_LOG(error,"Error connecting " << obj.toString() << " to space");
            mObjectInfo[obj].connectingTo = NullServerID;
            mObjectInfo[obj].connecting.cb(NullServerID);
            mObjectInfo[obj].connecting.cb = NULL;
        }
        else {
            OH_LOG(error,"Unknown connection response code: " << conn_resp.response());
        }
    }

    if (session_msg.has_init_migration()) {
        CBR::Protocol::Session::InitiateMigration init_migr = session_msg.init_migration();
        OH_LOG(insane,"Received migration request for " << msg->dest_object().toString() << " to " << init_migr.new_server());
        migrate(msg->dest_object(), init_migr.new_server());
    }

    delete msg;
}

} // namespace CBR
