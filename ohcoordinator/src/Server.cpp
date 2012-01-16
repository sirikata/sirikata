/*  Sirikata
 *  Server.cpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#include <sirikata/ohcoordinator/SpaceNetwork.hpp>
#include "Server.hpp"
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/ohcoordinator/Authenticator.hpp>
#include "LocalForwarder.hpp"

#include "ObjectConnection.hpp"
#include <sirikata/ohcoordinator/ObjectSessionManager.hpp>
#include <sirikata/ohcoordinator/ObjectHostSession.hpp>

#include <sirikata/core/util/Random.hpp>

#include <iostream>
#include <iomanip>
#include <sys/time.h>

#include <sirikata/core/network/IOStrandImpl.hpp>

#define SPACE_LOG(lvl,msg) SILOG(space, lvl, msg)

namespace Sirikata
{

namespace {
// Helper for filling in version info for connection responses
void fillVersionInfo(Sirikata::Protocol::Session::IVersionInfo vers_info, SpaceContext* ctx) {
    vers_info.set_name(ctx->name());
    vers_info.set_version(SIRIKATA_VERSION);
    vers_info.set_major(SIRIKATA_VERSION_MAJOR);
    vers_info.set_minor(SIRIKATA_VERSION_MINOR);
    vers_info.set_revision(SIRIKATA_VERSION_REVISION);
    vers_info.set_vcs_version(SIRIKATA_GIT_REVISION);
}
void logVersionInfo(Sirikata::Protocol::Session::VersionInfo vers_info) {
    SPACE_LOG(info, "Object host connection " << (vers_info.has_name() ? vers_info.name() : "(unknown)") << " version " << (vers_info.has_version() ? vers_info.version() : "(unknown)") << " (" << (vers_info.has_vcs_version() ? vers_info.vcs_version() : "") << ")");
}
} // namespace

Server::Server(SpaceContext* ctx, Authenticator* auth, Address4 oh_listen_addr, ObjectHostSessionManager* oh_sess_mgr, ObjectSessionManager* obj_sess_mgr)
 :ODP::DelegateService( std::tr1::bind(&Server::createDelegateODPPort, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, std::tr1::placeholders::_3) ),
  OHDP::DelegateService( std::tr1::bind(&Server::createDelegateOHDPPort, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2) ),
  mContext(ctx),
  mAuthenticator(auth),
  mLocalForwarder(NULL),
  mOHSessionManager(oh_sess_mgr),
  mObjectSessionManager(obj_sess_mgr),
  mShutdownRequested(false),
  mObjectHostConnectionManager(NULL),
  mRouteObjectMessage(Sirikata::SizedResourceMonitor(GetOptionValue<size_t>("route-object-message-buffer")))
{
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    mContext->sstConnectionManager()->createDatagramLayer(
            SpaceObjectReference(SpaceID::null(), ObjectReference::spaceServiceID()), mContext, this
        );

    mContext->sstConnectionManager()->listen(
        std::tr1::bind(&Server::newStream, this, _1, _2),
        SST::EndPoint<SpaceObjectReference>(SpaceObjectReference(SpaceID::null(), ObjectReference::spaceServiceID()), OBJECT_SPACE_PORT)
    );
    // ObjectHostConnectionManager takes care of listening for raw connections
    // and setting up SST connections with them.
    mObjectHostConnectionManager = new ObjectHostConnectionManager(
        mContext, oh_listen_addr,
        static_cast<OHDP::Service*>(this),
        static_cast<ObjectHostConnectionManager::Listener*>(this)
    );

    mLocalForwarder = new LocalForwarder(mContext);

    mCount = 0;

    timeval ts;
    gettimeofday(&ts,NULL);
    mtime_s = ts.tv_sec;
    mtime_us=ts.tv_usec;
}

void Server::newStream(int err, SST::Stream<SpaceObjectReference>::Ptr s) {
  if (err != SST_IMPL_SUCCESS){
    return;
  }

  // If we've lost the object's connection, we should just ignore this
  ObjectReference objid = s->remoteEndPoint().endPoint.object();
  if (mObjects.find(objid.getAsUUID()) == mObjects.end()) {
      s->close(false);
      return;
  }

  // Otherwise, they have a complete session
  mObjectSessionManager->completeSession(objid, s);
}

Server::~Server()
{
    SPACE_LOG(debug, "mObjects.size=" << mObjects.size());
    mObjects.clear();
    delete mObjectHostConnectionManager;
}

ODP::DelegatePort* Server::createDelegateODPPort(ODP::DelegateService*, const SpaceObjectReference& sor, ODP::PortID port) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    ODP::Endpoint port_ep(sor, port);
    return new ODP::DelegatePort(
        (ODP::DelegateService*)this,
        port_ep,
        std::tr1::bind(
            &Server::delegateODPPortSend, this, port_ep, _1, _2
        )
    );
}

bool Server::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload) {
    // Create new ObjectMessage
    Sirikata::Protocol::Object::ObjectMessage* msg =
        createObjectMessage(
            mContext->id(),
            source_ep.object().getAsUUID(), source_ep.port(),
            dest_ep.object().getAsUUID(), dest_ep.port(),
            String((char*)payload.data(), payload.size())
        );


    // This call needs to be thread safe, and we shouldn't be using this
    // ODP::Service to communicate with any non-local objects, so just use the
    // local forwarder.
    bool send_success = mLocalForwarder->tryForward(msg);

    // If the send failed, we need to destroy the message.
    if (!send_success)
        delete msg;

    return send_success;
}


OHDP::DelegatePort* Server::createDelegateOHDPPort(OHDP::DelegateService*, const OHDP::Endpoint& ept) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    // FIXME sanity check the connection

    return new OHDP::DelegatePort(
        (OHDP::DelegateService*)this,
        ept,
        std::tr1::bind(
            &Server::delegateOHDPPortSend, this, ept, _1, _2
        )
    );
}

bool Server::delegateOHDPPortSend(const OHDP::Endpoint& source_ep, const OHDP::Endpoint& dest_ep, MemoryReference payload) {
    // Create new ObjectMessage. We use ObjectMessages, but the connection we
    // send over actually determines most of these components. The space portion
    // of the endpoint is ignored (the space uses SpaceID::null() internally)
    // and the NodeIDs are irrelevant in the message (only used below to map to
    // a connection).
    Sirikata::Protocol::Object::ObjectMessage* msg =
        createObjectMessage(
            mContext->id(),
            UUID::null(), source_ep.port(),
            UUID::null(), dest_ep.port(),
            String((char*)payload.data(), payload.size())
        );

    OHDP::NodeID nid = dest_ep.node();
    uint32 nid_raw = nid;
    bool send_success = mObjectHostConnectionManager->send(nid_raw, msg);

    // If the send failed, we need to destroy the message.
    if (!send_success)
        delete msg;

    return send_success;
}

bool Server::isObjectConnected(const UUID& object_id) const {
    return (mObjects.find(object_id) != mObjects.end());
}

bool Server::isObjectConnecting(const UUID& object_id) const {
    return (mStoredConnectionData.find(object_id) != mStoredConnectionData.end());
}

void Server::sendSessionMessageWithRetry(const ObjectHostConnectionID& conn, Sirikata::Protocol::Object::ObjectMessage* msg, const Duration& retry_rate) {
    bool sent = mObjectHostConnectionManager->send( conn, msg );
    if (!sent) {
        mContext->mainStrand->post(
            retry_rate,
            std::tr1::bind(&Server::sendSessionMessageWithRetry, this, conn, msg, retry_rate)
        );
    }
}

bool Server::onObjectHostMessageReceived(const ObjectHostConnectionID& conn_id, const ShortObjectHostConnectionID short_conn_id, Sirikata::Protocol::Object::ObjectMessage* obj_msg) {
    // NOTE that we do forwarding even before the

    static UUID spaceID = UUID::null();

    // Before admitting a message, we need to do some sanity checks.  Also, some types of messages get
    // exceptions for bootstrapping purposes (namely session messages to the space).

    // 2. For connection bootstrapping purposes we need to exempt session messages destined for the space.
    // Note that we need to check this before the connected sanity check since obviously the object won't
    // be connected yet.  We dispatch directly from here since this needs information about the object host
    // connection to be passed along as well.
    bool session_msg = (obj_msg->dest_port() == OBJECT_PORT_SESSION);
    if (session_msg)
    {
        bool space_dest = (obj_msg->dest_object() == spaceID);

        // FIXME infinite queue
        mContext->mainStrand->post(
            std::tr1::bind(
                &Server::handleSessionMessage, this,
                conn_id, obj_msg
            )
        );
        return true;
    }

    // Otherwise, we're going to have to ship this to the main thread, either
    // for handling session messages, messages to the space, or to make a
    // routing decision.
    bool hit_empty;
    bool push_for_processing_success;
    {
        boost::lock_guard<boost::mutex> lock(mRouteObjectMessageMutex);
        hit_empty = (mRouteObjectMessage.probablyEmpty());
        push_for_processing_success = mRouteObjectMessage.push(ConnectionIDObjectMessagePair(conn_id,obj_msg),false);
    }
    if (!push_for_processing_success) {
        TIMESTAMP(obj_msg, Trace::SPACE_DROPPED_AT_MAIN_STRAND_CROSSING);
        TRACE_DROP(SPACE_DROPPED_AT_MAIN_STRAND_CROSSING);
        delete obj_msg;
    } else {
        if (hit_empty)
            scheduleObjectHostMessageRouting();
    }

    // NOTE: We always "accept" the data, even if we're just dropping
    // it.  This keeps packets flowing.  We could use flow control to
    // slow things down, but since the data path splits in this method
    // between local and remote, we don't want to slow the local
    // packets just because of a backup in routing.
    return true;
}

void Server::onObjectHostConnected(const ObjectHostConnectionID& conn_id, const ShortObjectHostConnectionID short_conn_id, OHDPSST::Stream::Ptr stream) {
    assert( short_conn_id == (ShortObjectHostConnectionID)stream->remoteEndPoint().endPoint.node() );
    mOHSessionManager->fireObjectHostSession(stream->remoteEndPoint().endPoint.node(), stream);
}

void Server::onObjectHostDisconnected(const ObjectHostConnectionID& oh_conn_id, const ShortObjectHostConnectionID short_conn_id) {
	SPACE_LOG(info, "OH connection "<<short_conn_id<<": "<<mObjectsDistribution[short_conn_id]->ObjectHostName<<" disconnected");
	mOHNameConnections.erase(mObjectsDistribution[short_conn_id]->ObjectHostName);
	mCount = mCount-mObjectsDistribution[short_conn_id]->counter;
	mObjectsDistribution.erase(short_conn_id);
    mContext->mainStrand->post( std::tr1::bind(&Server::handleObjectHostConnectionClosed, this, oh_conn_id) );
    mOHSessionManager->fireObjectHostSessionEnded( OHDP::NodeID(short_conn_id) );
}

void Server::scheduleObjectHostMessageRouting() {
    mContext->mainStrand->post(
        std::tr1::bind(
            &Server::handleObjectHostMessageRouting,
            this));
}

void Server::handleObjectHostMessageRouting() {
#define MAX_OH_MESSAGES_HANDLED 100

    for(uint32 i = 0; i < MAX_OH_MESSAGES_HANDLED; i++)
        if (!handleSingleObjectHostMessageRouting())
            break;

    {
        boost::lock_guard<boost::mutex> lock(mRouteObjectMessageMutex);
        if (!mRouteObjectMessage.probablyEmpty())
            scheduleObjectHostMessageRouting();
    }
}

bool Server::handleSingleObjectHostMessageRouting() {
    ConnectionIDObjectMessagePair front(ObjectHostConnectionID(),NULL);
    if (!mRouteObjectMessage.pop(front))
        return false;

    UUID source_object = front.obj_msg->source_object();

    // OHDP (object host <-> space server communication) piggy backs on ODP
    // messages so that we can use ODP messages as the basis for all
    // communication between space servers object hosts. We need to detect
    // messages that match this and dispatch the message.
    static UUID ohdp_ID = UUID::null();
    if (source_object == ohdp_ID) {
        // Sanity check: if the destination isn't also null, then the message is
        // non-sensical and we can just discard
        UUID dest_object = front.obj_msg->dest_object();
        if (dest_object != ohdp_ID) {
            delete front.obj_msg;
            return true;
        }

        // We need to translate identifiers. The space identifiers are ignored
        // on the space server (only one space to deal with, unlike object
        // hosts). The NodeID uses null() for the local (destination) endpoint
        // and the short ID of the object host connection for the remote
        // (source).
        ShortObjectHostConnectionID ohdp_node_id = front.conn_id.shortID();

        OHDP::DelegateService::deliver(
            OHDP::Endpoint(SpaceID::null(), OHDP::NodeID(ohdp_node_id), front.obj_msg->source_port()),
            OHDP::Endpoint(SpaceID::null(), OHDP::NodeID::null(), front.obj_msg->dest_port()),
            MemoryReference(front.obj_msg->payload())
        );
        delete front.obj_msg;

        return true;
    }

    // If we don't have a connection for the source object, we can't do anything with it.
    // The object could be migrating and we get outdated packets.  Currently this can
    // happen because we need to maintain the connection long enough to deliver the init migration
    // message.  Therefore, we check if its in the currently migrating connections as well as active
    // connections and allow messages through.
    // NOTE that we check connecting objects as well since we need to get past this point to deliver
    // Session messages.
    bool source_connected =
        mObjects.find(source_object) != mObjects.end();
    if (!source_connected)
    {
       SILOG(cbr,warn,"Got message for unknown object: " << source_object.toString());
       delete front.obj_msg;
       return true;
    }
    return true;
}

// Handle Session messages from an object
void Server::handleSessionMessage(const ObjectHostConnectionID& oh_conn_id, Sirikata::Protocol::Object::ObjectMessage* msg) {
    Sirikata::Protocol::Session::Container session_msg;
    bool parse_success = session_msg.ParseFromString(msg->payload());
    if (!parse_success) {
        LOG_INVALID_MESSAGE(space, error, msg->payload());
        delete msg;
        return;
    }

    // Connect or migrate messages
    if (session_msg.has_connect()) {
    	//if (session_msg.connect().has_version())
        //logVersionInfo(session_msg.connect().version());
        if (session_msg.connect().type() == Sirikata::Protocol::Session::Connect::Fresh)
        {
            handleConnect(oh_conn_id, *msg, session_msg.connect());
        }
        else
            SILOG(space,error,"Unknown connection message type");
    }
    else if (session_msg.has_connect_ack()) {
    }
    else if (session_msg.has_disconnect()) {
        ObjectConnectionMap::iterator it = mObjects.find(session_msg.disconnect().object());
        if (it != mObjects.end()) {
            handleDisconnect(session_msg.disconnect().object(), it->second);
        }
    }
    else if (session_msg.has_coordinate()) {
    	if(session_msg.coordinate().type() == Sirikata::Protocol::Session::Coordinate::Add) { //Feng
    		UUID obj_id = session_msg.coordinate().object();
    		UUID entity_id = session_msg.coordinate().entity();

    		mObjectsDistribution[oh_conn_id.shortID()]->counter++;
    		mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].ObjectSet.insert(obj_id);
    		mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].Migrating = false;
    		mCount++;

    	    // Update mObjectInfo
    	    if(!isObjectRecorded(obj_id)){
    	    	mObjectInfo[obj_id].short_id=oh_conn_id.shortID();
    	    	mObjectInfo[obj_id].entity=entity_id;
    	    	mObjectInfo[obj_id].Migrating=false;
    	    	//SPACE_LOG(info, "New object recorded");
    	    }
    	    else{
    	    	if(mObjectInfo[obj_id].Migrating==true) {
    	    		if(mObjectInfo[obj_id].MigratingTo==mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName){
    	    			mObjectsDistribution[mObjectInfo[obj_id].short_id]->migratingTo_N--;
    	    			mObjectsDistribution[oh_conn_id.shortID()]->migratingFrom_N--;

    	    	    	mObjectInfo[obj_id].short_id=oh_conn_id.shortID();
    	    	    	mObjectInfo[obj_id].entity=entity_id;
    	    	    	mObjectInfo[obj_id].Migrating=false;

    	    	    	SPACE_LOG(info, "Migrated object recorded, OH: "<<mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName
    	    	    			<<", migratingFrom_N: "<<mObjectsDistribution[oh_conn_id.shortID()]->migratingFrom_N);
    	    		}
    	    	}
    	    }

            timeval ts;
            gettimeofday(&ts,NULL);
            long int time_s = ts.tv_sec;
            int time_us=ts.tv_usec;
            long int diff_s=time_s - mtime_s;
            int diff_us=time_us - mtime_us;
            double diff_t=diff_s+diff_us/(double)1000000;

    		SPACE_LOG(info, "OH "<<mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName
    						<<" add one object, count: "<<mObjectsDistribution[oh_conn_id.shortID()]->counter<<"/"<<mCount
    						<<", time: "<<diff_t<<" s");

    		rebalance(entity_id, oh_conn_id);

    		/*
    		ObjectsDistributionMap::iterator it;
	        int sum = 0;
	        for (it = mObjectsDistribution.begin(); it != mObjectsDistribution.end(); it++)
	        	sum += it->second->counter2;
	        int avg = sum / mObjectsDistribution.size();
	        bool update_flag = true;
	        for (it = mObjectsDistribution.begin(); it != mObjectsDistribution.end(); it++) {
	        	if (it->second->counter2 != avg && it->second->counter2 != avg + 1)
	        		update_flag = false;
	        }
	        if (update_flag == true) {
	        	for (it = mObjectsDistribution.begin(); it != mObjectsDistribution.end(); it++)
	        		it->second->counter2 = it->second->counter;
	        }
           */

    	}
        if(session_msg.coordinate().type() == Sirikata::Protocol::Session::Coordinate::Remove) { //Feng
        	UUID obj_id = session_msg.coordinate().object();
        	UUID entity_id = session_msg.coordinate().entity();

        	if (mObjectsDistribution[oh_conn_id.shortID()]->entityMap.find(entity_id) != mObjectsDistribution[oh_conn_id.shortID()]->entityMap.end()
        		&& 	mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].ObjectSet.find(obj_id) != mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].ObjectSet.end()) {
        		mCount--;
        		mObjectsDistribution[oh_conn_id.shortID()]->counter--;
        		mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].ObjectSet.erase(obj_id);

                timeval ts;
                gettimeofday(&ts,NULL);
                long int time_s = ts.tv_sec;
                int time_us=ts.tv_usec;
                long int diff_s=time_s - mtime_s;
                int diff_us=time_us - mtime_us;
                double diff_t=diff_s+diff_us/(double)1000000;

        		SPACE_LOG(info, "OH "<<mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName
    							<<" remove one object, count: "<<mObjectsDistribution[oh_conn_id.shortID()]->counter<<"/"<<mCount
    							<<", time: "<<diff_t<<" s");
        	}
        }
        if(session_msg.coordinate().type() == Sirikata::Protocol::Session::Coordinate::Ack) {
        	mObjectsDistribution[oh_conn_id.shortID()]->migrate_capacity = session_msg.coordinate().migrate_capacity();
        	mObjectsDistribution[oh_conn_id.shortID()]->migrate_threshold = session_msg.coordinate().migrate_threshold();
        }
    	else if(session_msg.coordinate().type() == Sirikata::Protocol::Session::Coordinate::MigrateReq) {
    		UUID entity_id = session_msg.coordinate().entity();
    		SPACE_LOG(info, "Receive migration request of entity "<<entity_id.rawHexData());

    		handleMigrateRequest(oh_conn_id, entity_id);

    		/*
    		String DstOHName;
    		String SrcOHName = mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName;

    		if (rebalance(SrcOHName, DstOHName, entity_id)) {
    			mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].MigrationDstOHName = DstOHName;
                mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].Migrating = true;

                int t = mObjectsDistribution[mOHNameConnections[SrcOHName].shortID()]->entityMap[entity_id].ObjectSet.size();
                mObjectsDistribution[mOHNameConnections[SrcOHName].shortID()]->counter2 -= t;

                t = mObjectsDistribution[mOHNameConnections[DstOHName].shortID()]->entityMap[entity_id].ObjectSet.size();
                mObjectsDistribution[mOHNameConnections[DstOHName].shortID()]->counter2 += t;

                informOHMigrationTo(DstOHName, entity_id, oh_conn_id);
    		} else {}
    		*/

    	}
    	else if(session_msg.coordinate().type() == Sirikata::Protocol::Session::Coordinate::Ready) {
    		UUID entity_id = session_msg.coordinate().entity();
    		SPACE_LOG(info, "Entity "<<entity_id.rawHexData()<<" is ready to migrate");
    		String DstOHName = mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].MigrationDstOHName;
    		String SrcOHName = mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName;
    		mObjectsDistribution[oh_conn_id.shortID()]->entityMap.erase(entity_id);

    		informOHMigrationFrom(SrcOHName, entity_id,  mOHNameConnections[DstOHName]);
    	}
    }

    // InitiateMigration messages
    assert(!session_msg.has_connect_response());

    delete msg;
}

void Server::handleMigrateRequest(const ObjectHostConnectionID& oh_conn_id, const UUID& entity_id)
{
	String DstOHName;
	bool success = false;
	uint32 delta = mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].ObjectSet.size();
	ObjectsDistributionMap::iterator it;
	for (it = mObjectsDistribution.begin(); it != mObjectsDistribution.end(); it++) {
		if( it->first != oh_conn_id.shortID()
				&& it->second->counter + delta + it->second->migratingFrom_N <= it->second->migrate_threshold
				&& delta <= it->second->migrate_capacity) {

			DstOHName = it->second->ObjectHostName;
			success = true;
			break;
		}
	}
	if (success)
		informOHMigrationTo(DstOHName, entity_id, oh_conn_id);
}

bool Server::rebalance(const UUID& entity_id, const ObjectHostConnectionID& oh_conn_id)
{
	String DstOHName;
	bool unbalance = false ;
	uint32 delta = mObjectsDistribution[oh_conn_id.shortID()]->entityMap[entity_id].ObjectSet.size();
	uint32 min = mObjectsDistribution[oh_conn_id.shortID()]->counter - mObjectsDistribution[oh_conn_id.shortID()]->migratingTo_N;
	uint32 min_org = min;

	ObjectsDistributionMap::iterator it;
	for (it = mObjectsDistribution.begin(); it != mObjectsDistribution.end(); it++) {
		if( it->first != oh_conn_id.shortID()
			&& it->second->counter + it->second->migratingFrom_N - it->second->migratingTo_N < min)
		{
			DstOHName = it->second->ObjectHostName;
			min = it->second->counter + it->second->migratingFrom_N - it->second->migratingTo_N;
		}
	}

	if (min + 2*delta < min_org) {
		unbalance = true;
		informOHMigrationTo(DstOHName, entity_id, oh_conn_id);
		SPACE_LOG(info, "Unbalance! move entity "<<entity_id.rawHexData()<<" from "
				<<mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName<<" to "<<DstOHName);
	}

	return unbalance;
}

//Feng
bool Server::rebalance(const String& SrcOHName, String& DstOHName1, const UUID& entity_id) {
	ObjectsDistributionMap::iterator it;
	uint32 min = mObjectsDistribution[mOHNameConnections[SrcOHName].shortID()]->counter2; // we are going to find a OH has few objects
	uint32 delta = mObjectsDistribution[mOHNameConnections[SrcOHName].shortID()]->entityMap[entity_id].ObjectSet.size();
	uint32 min_org = min;
	String minOHName = SrcOHName;
	for (it = mObjectsDistribution.begin(); it != mObjectsDistribution.end(); it++) {
		if (it->second->counter2 < min) {
			min = it->second->counter2;
			minOHName = it->second->ObjectHostName;
		}
	}
	if ((min_org - min) > 2 * delta) {  // this is about the policy; And it can be changed.
		DstOHName1 = minOHName;
		return true;
	}
	return false;
}

//Feng
bool Server::rebalance(const String& SrcOHName, String& DstOHName) {
	ObjectsDistributionMap::iterator it;
	uint32 max, min;
	max = min = 0;
	String maxOHName = mObjectsDistribution.begin()->second->ObjectHostName;
	String minOHName = mObjectsDistribution.begin()->second->ObjectHostName;
	for (it = mObjectsDistribution.begin(); it != mObjectsDistribution.end(); it++) {
		if (it->second->counter2 > max) {
			max = it->second->counter2;
			maxOHName = it->second->ObjectHostName;
		}
		if (it->second->counter2 < min) {
			min = it->second->counter2;
			minOHName = it->second->ObjectHostName;
		}
	}
	if (max > 2) {  // this is about the policy; And it can be changed.
		DstOHName = minOHName;
		return true;
	}
	return false;
}

//Feng
void Server::informOHMigrationFrom(const String& SrcOHName, const UUID& uuid, const ObjectHostConnectionID& oh_conn_id) {
	Sirikata::Protocol::Session::Container session_msg;
	Sirikata::Protocol::Session::Coordinate oh_coordinate_msg = session_msg.mutable_coordinate();
	session_msg.coordinate().set_entity(uuid);
	session_msg.coordinate().set_oh_name(SrcOHName);
	session_msg.coordinate().set_type(Sirikata::Protocol::Session::Coordinate::MigrateFrom);
	Sirikata::Protocol::Object::ObjectMessage* migration_req = createObjectMessage(
			mContext->id(),
			UUID::null(), OBJECT_PORT_SESSION,
			UUID::null(), OBJECT_PORT_SESSION,
			serializePBJMessage(session_msg)
			);
	// Sent directly via object host connection manager
    String DstOHName = mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName;
	SPACE_LOG(info, "Send migration request to OH "<<DstOHName<<" < type:From, entity:"
                        << uuid.rawHexData() << ", src:" << SrcOHName<<" >");
	sendSessionMessageWithRetry(oh_conn_id, migration_req, Duration::seconds(0.05));
}

void Server::informOHMigrationTo(const String& DstOHName, const UUID& uuid, const ObjectHostConnectionID& oh_conn_id) {
	Sirikata::Protocol::Session::Container session_msg;
	Sirikata::Protocol::Session::Coordinate oh_coordinate_msg = session_msg.mutable_coordinate();
    session_msg.coordinate().set_entity(uuid);
	session_msg.coordinate().set_oh_name(DstOHName);
	session_msg.coordinate().set_type(Sirikata::Protocol::Session::Coordinate::MigrateTo);
	Sirikata::Protocol::Object::ObjectMessage* migration_req = createObjectMessage(
			mContext->id(),
			UUID::null(), OBJECT_PORT_SESSION,
			UUID::null(), OBJECT_PORT_SESSION,
			serializePBJMessage(session_msg)
			);
	// Sent directly via object host connection manager

	String SrcOHName = mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName;
	SPACE_LOG(info, "Send migration request to OH "<< SrcOHName <<" < type:To, entity:"
                        << uuid.rawHexData() << ", dst:" << DstOHName <<" >");
	sendSessionMessageWithRetry(oh_conn_id, migration_req, Duration::seconds(0.05));

	mObjectsDistribution[oh_conn_id.shortID()]->entityMap[uuid].MigrationDstOHName = DstOHName;
    mObjectsDistribution[oh_conn_id.shortID()]->entityMap[uuid].Migrating = true;
    std::set<UUID>::iterator it;
    for (it=mObjectsDistribution[oh_conn_id.shortID()]->entityMap[uuid].ObjectSet.begin();
    		it!=mObjectsDistribution[oh_conn_id.shortID()]->entityMap[uuid].ObjectSet.end();it++) {
    	mObjectInfo[*it].Migrating = true;
    	mObjectInfo[*it].MigratingTo = DstOHName;
    	mObjectsDistribution[oh_conn_id.shortID()]->migratingTo_N++;
    	mObjectsDistribution[mOHNameConnections[DstOHName].shortID()]->migratingFrom_N++;
    	//SPACE_LOG(info, "OH: "<<mObjectsDistribution[mOHNameConnections[DstOHName].shortID()]->ObjectHostName
    	//		<<", migratingFrom_N: "<<mObjectsDistribution[mOHNameConnections[DstOHName].shortID()]->migratingFrom_N);
    }
}


void Server::handleObjectHostConnectionClosed(const ObjectHostConnectionID& oh_conn_id) {
    for(ObjectConnectionMap::iterator it = mObjects.begin(); it != mObjects.end(); ) {
        UUID obj_id = it->first;
        ObjectConnection* obj_conn = it->second;

        it++; // Iterator might get erased in handleDisconnect

        if (obj_conn->connID() != oh_conn_id)
            continue;

        handleDisconnect(obj_id, obj_conn);
    }
}

void Server::retryHandleConnect(const ObjectHostConnectionID& oh_conn_id, Sirikata::Protocol::Object::ObjectMessage* obj_response) {
    if (!mObjectHostConnectionManager->send(oh_conn_id,obj_response)) {
        mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&Server::retryHandleConnect,this,oh_conn_id,obj_response));
    }else {

    }
}

void Server::sendConnectError(const ObjectHostConnectionID& oh_conn_id, const UUID& obj_id) {
    Sirikata::Protocol::Session::Container response_container;
    Sirikata::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
    fillVersionInfo(response.mutable_version(), mContext);
    response.set_response( Sirikata::Protocol::Session::ConnectResponse::Error );

    Sirikata::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(response_container)
    );

    // Sent directly via object host connection manager because we don't have an ObjectConnection
    if (!mObjectHostConnectionManager->send( oh_conn_id, obj_response )) {
        mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&Server::retryHandleConnect,this,oh_conn_id,obj_response));
    }
}

void Server::sendDisconnect(const ObjectHostConnectionID& oh_conn_id, const UUID& obj_id, const String& reason) {
    Sirikata::Protocol::Session::Container msg_container;
    Sirikata::Protocol::Session::IDisconnect disconnect = msg_container.mutable_disconnect();
    disconnect.set_object(obj_id);
    disconnect.set_reason(reason);

    Sirikata::Protocol::Object::ObjectMessage* obj_disconnect = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(msg_container)
    );

    // Sent directly via object host connection manager
    if (!mObjectHostConnectionManager->send( oh_conn_id, obj_disconnect )) {
        mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&Server::retryHandleConnect,this,oh_conn_id,obj_disconnect));
    }
}

// Handle Connect message from object
void Server::handleConnect(const ObjectHostConnectionID& oh_conn_id, const Sirikata::Protocol::Object::ObjectMessage& container, const Sirikata::Protocol::Session::Connect& connect_msg) {
    UUID obj_id = container.source_object();

    // FIXME sanity check the new connection
    // -- verify object may connect, i.e. not already in system (e.g. check oseg)

    String auth_data = "";
    if (connect_msg.has_auth())
        auth_data = connect_msg.auth();
    mAuthenticator->authenticate(
        obj_id, MemoryReference(auth_data),
        std::tr1::bind(&Server::handleConnectAuthResponse, this, oh_conn_id, obj_id, connect_msg, std::tr1::placeholders::_1)
    );
}

void Server::handleConnectAuthResponse(const ObjectHostConnectionID& oh_conn_id, const UUID& obj_id, const Sirikata::Protocol::Session::Connect& connect_msg, bool authenticated) {
    if (!authenticated) {
        sendConnectError(oh_conn_id, obj_id);
        return;
    }

    // Because of unreliable messaging, we might get a double connect request
    // (if we got the initial request but the response was dropped). In that
    // case, just send them another one and ignore this
    // request. Alternatively, someone might just be trying to use the
    // same object ID.
    if (isObjectConnected(obj_id) || isObjectConnecting(obj_id)) {
        // Decide whether this is a conflict or a retry
        if  //was already connected and it was the same oh sending msg
            (isObjectConnected(obj_id) &&
                (mObjects[obj_id]->connID() == oh_conn_id))
        {
            // retry, tell them they're fine.
            sendConnectSuccess(oh_conn_id, obj_id);

        }
        else if
            // or was connecting and was the same oh sending message
            (isObjectConnecting(obj_id) &&
                mStoredConnectionData[obj_id].conn_id == oh_conn_id)
        {

        }
        else
        {
        	sendConnectError(oh_conn_id, obj_id);
        }
        return;

    }

    StoredConnection sc;
    sc.conn_id = oh_conn_id;
    sc.conn_msg = connect_msg;
    mStoredConnectionData[obj_id] = sc;

    // Feng: For each object connection, the distribution map will keep a record.
    mObjectHostConnectionManager->addObject(oh_conn_id.shortID(), obj_id);  


    mObjectSessionManager->addSession(new ObjectSession(ObjectReference(obj_id)));
    ObjectConnection* conn = new ObjectConnection(obj_id, mObjectHostConnectionManager, sc.conn_id);
    mObjects[obj_id] = conn;
    mOHNameConnections[sc.conn_msg.oh_name()]=sc.conn_id;
    sendConnectSuccess(conn->connID(), obj_id);

    /*Feng: this is a successful oh connection, then this oh should be added to the record */
    ObjectsDistribution* dist = new ObjectsDistribution;
    dist->ObjectHostName = sc.conn_msg.oh_name();
    dist->counter = 0;
    dist->counter2 = 0;
    dist->migrate_threshold = 0;
    dist->migratingFrom_N = 0;
    dist->migratingTo_N = 0;
    mObjectsDistribution[oh_conn_id.shortID()] = dist;

    SPACE_LOG(info, "New object host connected: < name: "<<mObjectsDistribution[oh_conn_id.shortID()]->ObjectHostName
    				<<", connection id: "<<oh_conn_id.shortID()<<" >");
}

void Server::sendConnectSuccess(const ObjectHostConnectionID& oh_conn_id, const UUID& obj_id) {

    // Send reply back indicating that the connection was successful
    Sirikata::Protocol::Session::Container response_container;
    Sirikata::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
    fillVersionInfo(response.mutable_version(), mContext);
    response.set_response( Sirikata::Protocol::Session::ConnectResponse::Success );

    Sirikata::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(response_container)
    );
    // Sent directly via object host connection manager because ObjectConnection isn't enabled yet
    sendSessionMessageWithRetry(oh_conn_id, obj_response, Duration::seconds(0.05));
}

// Note that the obj_id is intentionally not a const & so that we're sure it is
// valid throughout this method.
void Server::handleDisconnect(UUID obj_id, ObjectConnection* conn) {
    assert(conn->id() == obj_id);

    mObjects.erase(obj_id);
    // Num objects is reported by the caller

    ObjectReference obj(obj_id);
    mObjectSessionManager->removeSession(obj);

    // Feng:
    // mObjectHostConnectionManager->deleteObject(short_conn_id, obj_id);

    delete conn;
}

void Server::handleEntityOHMigraion(const UUID& uuid, const ObjectHostConnectionID& oh_conn_id) {
    Sirikata::Protocol::Session::Container oh_migration;
    Sirikata::Protocol::Session::IOHMigration oh_migration_msg = oh_migration.mutable_oh_migration();
    oh_migration_msg.set_id(uuid);
    oh_migration_msg.set_type(Sirikata::Protocol::Session::OHMigration::Entity);
    Sirikata::Protocol::Object::ObjectMessage* migration_req = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(oh_migration)
    );
    // Sent directly via object host connection manager
    sendSessionMessageWithRetry(oh_conn_id, migration_req, Duration::seconds(0.05));
}

void Server::start() {
}

void Server::stop() {
    mObjectHostConnectionManager->shutdown();
    mShutdownRequested = true;
}

} // namespace Sirikata
