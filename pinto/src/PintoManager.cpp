/*  Sirikata
 *  PintoManager.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include "PintoManager.hpp"
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include <sirikata/core/network/Message.hpp> // parse/serializePBJMessage
#include "Protocol_MasterPinto.pbj.hpp"

#include <prox/BruteForceQueryHandler.hpp>
#include <prox/RTreeQueryHandler.hpp>

using namespace Sirikata::Network;

#define PINTO_LOG(lvl, msg) SILOG(pinto,lvl,"[PINTO] " << msg)

namespace Sirikata {

PintoManager::PintoManager(PintoContext* ctx)
 : mContext(ctx),
   mLastTime(Time::null()),
   mDt(Duration::milliseconds((int64)1))
{
    String listener_protocol = GetOptionValue<String>(OPT_PINTO_PROTOCOL);
    String listener_protocol_options = GetOptionValue<String>(OPT_PINTO_PROTOCOL_OPTIONS);

    OptionSet* listener_protocol_optionset = StreamListenerFactory::getSingleton().getOptionParser(listener_protocol)(listener_protocol_options);

    mListener = StreamListenerFactory::getSingleton().getConstructor(listener_protocol)(mContext->ioService, listener_protocol_optionset);

    String listener_host = GetOptionValue<String>(OPT_PINTO_HOST);
    String listener_port = GetOptionValue<String>(OPT_PINTO_PORT);
    Address listenAddress(listener_host, listener_port);
    PINTO_LOG(debug, "Listening on " << listenAddress.toString());
    mListener->listen(
        listenAddress,
        std::tr1::bind(&PintoManager::newStreamCallback,this,_1,_2)
    );

    mLocCache = new PintoManagerLocationServiceCache();
    mQueryHandler = new Prox::BruteForceQueryHandler<ServerProxSimulationTraits>();
    mQueryHandler->initialize(mLocCache);
}

PintoManager::~PintoManager() {
    delete mListener;

    delete mQueryHandler;
}

void PintoManager::start() {
}

void PintoManager::stop() {
    mListener->close();
}

void PintoManager::newStreamCallback(Stream* newStream, Stream::SetCallbacks& setCallbacks) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    PINTO_LOG(debug,"New space server connection.");

    setCallbacks(
        std::tr1::bind(&PintoManager::handleClientConnection, this, newStream, _1, _2),
        std::tr1::bind(&PintoManager::handleClientReceived, this, newStream, _1, _2),
        std::tr1::bind(&PintoManager::handleClientReadySend, this, newStream)
    );

    mClients[newStream] = ClientData();
}

void PintoManager::handleClientConnection(Sirikata::Network::Stream* stream, Network::Stream::ConnectionStatus status, const std::string &reason) {
    if (status == Network::Stream::Disconnected) {
        // Unregister region and query, remove from clients list.
        mLocCache->removeSpaceServer( mClients[stream].server);
        mClientsByQuery.erase( mClients[stream].query );
        delete mClients[stream].query;
        mClients.erase(stream);

        tick();
    }
}

void PintoManager::handleClientReceived(Sirikata::Network::Stream* stream, Chunk& data, const Network::Stream::PauseReceiveCallback& pause) {
    Sirikata::Protocol::MasterPinto::PintoMessage msg;
    bool parsed = parsePBJMessage(&msg, data);

    if (!parsed) {
        PINTO_LOG(error, "Couldn't parse message from client.");
        return;
    }

    ClientData& cdata = mClients[stream];

    if (msg.has_server()) {
        PINTO_LOG(debug, "Associated connection with space server " << msg.server().server());
        cdata.server = msg.server().server();

        TimedMotionVector3f default_loc( Time::null(), MotionVector3f( Vector3f::nil(), Vector3f::nil() ) );
        BoundingSphere3f default_region(BoundingSphere3f::null());
        float32 default_max = 0.f;
        SolidAngle default_min_angle(SolidAngle::Max);

        mLocCache->addSpaceServer(cdata.server, default_loc, default_region, default_max);

        Query* query = mQueryHandler->registerQuery(default_loc, default_region, default_max, default_min_angle);
        cdata.query = query;
        mClientsByQuery[query] = stream;
        query->setEventListener(this);
    }
    else {
        if (cdata.server == NullServerID) {
            PINTO_LOG(error, "Received initial message from client without a ServerID.");
            stream->close();
            mClients.erase(stream);
            return;
        }
    }

    if (msg.has_region()) {
        PINTO_LOG(debug, "Received region update from " << cdata.server << ": " << msg.region().bounds());
        mLocCache->updateSpaceServerRegion(cdata.server, msg.region().bounds());
        cdata.query->region( msg.region().bounds() );
    }

    if (msg.has_largest()) {
        PINTO_LOG(debug, "Received largest object update from " << cdata.server << ": " << msg.largest().radius());
        mLocCache->updateSpaceServerMaxSize(cdata.server, msg.largest().radius());
        cdata.query->maxSize( msg.largest().radius() );
    }

    if (msg.has_query()) {
        PINTO_LOG(debug, "Received query update from " << cdata.server << ": " << msg.query().min_angle());
        cdata.query->angle( SolidAngle(msg.query().min_angle()) );
    }

    tick();
}

void PintoManager::handleClientReadySend(Sirikata::Network::Stream* stream) {
}

void PintoManager::tick() {
    mLastTime += mDt;
    mQueryHandler->tick(mLastTime);
}

void PintoManager::queryHasEvents(Query* query) {
    typedef std::deque<QueryEvent> QueryEventList;

    Network::Stream* stream = mClientsByQuery[query];
    ClientData& cdata = mClients[stream];

    QueryEventList evts;
    query->popEvents(evts);

    Sirikata::Protocol::MasterPinto::PintoResponse msg;

    while(!evts.empty()) {
        const QueryEvent& evt = evts.front();
        //PINTO_LOG(debug, "Event generated for server " << cdata.server << ": " << evt.id() << (evt.type() == QueryEvent::Added ? " added" : " removed"));

        Sirikata::Protocol::MasterPinto::IPintoUpdate update = msg.add_update();
        for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
            Sirikata::Protocol::MasterPinto::IPintoResult result = update.add_change();
            result.set_addition(true);
            result.set_server(evt.additions()[aidx].id());
        }
        for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
            Sirikata::Protocol::MasterPinto::IPintoResult result = update.add_change();
            result.set_addition(false);
            result.set_server(evt.removals()[ridx].id());
        }

        evts.pop_front();
    }

    String serialized = serializePBJMessage(msg);
    stream->send( MemoryReference(serialized), ReliableOrdered );
}

} // namespace Sirikata
