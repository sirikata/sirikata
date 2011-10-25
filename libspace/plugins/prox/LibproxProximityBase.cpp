// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "LibproxProximityBase.hpp"

#include <sirikata/core/network/Frame.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>

#define PROXLOG(level,msg) SILOG(prox,level,"[PROX] " << msg)

namespace Sirikata {


LibproxProximityBase::LibproxProximityBase(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr)
 : Proximity(ctx, locservice, cseg, net, aggmgr, Duration::milliseconds((int64)100)),
   mProxStrand(ctx->ioService->createStrand()),
   mLocCache(NULL)
{
    mProxServerMessageService = mContext->serverRouter()->createServerMessageService("proximity");

    // Location cache, for both types of queries
    mLocCache = new CBRLocationServiceCache(mProxStrand, locservice, true);
}

LibproxProximityBase::~LibproxProximityBase() {
    delete mProxServerMessageService;
    delete mLocCache;
    delete mProxStrand;
}

void LibproxProximityBase::sendObjectResult(Sirikata::Protocol::Object::ObjectMessage* msg) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    // Try to find stream info for the object
    ObjectProxStreamMap::iterator prox_stream_it = mObjectProxStreams.find(msg->dest_object());
    if (prox_stream_it == mObjectProxStreams.end()) {
        prox_stream_it = mObjectProxStreams.insert( ObjectProxStreamMap::value_type(msg->dest_object(), ProxStreamInfoPtr(new ProxStreamInfo())) ).first;
        prox_stream_it->second->writecb = std::tr1::bind(
            &LibproxProximityBase::writeSomeObjectResults, this, ProxStreamInfoWPtr(prox_stream_it->second)
        );
    }
    ProxStreamInfoPtr prox_stream = prox_stream_it->second;

    // If we don't have a stream yet, try to build it
    if (!prox_stream->iostream_requested)
        requestProxSubstream(msg->dest_object(), prox_stream);

    // Build the result and push it into the stream
    // FIXME this is an infinite sized queue, but we don't really want to drop
    // proximity results....
    prox_stream->outstanding.push(Network::Frame::write(msg->payload()));

    // If writing isn't already in progress, start it up
    if (!prox_stream->writing)
        writeSomeObjectResults(prox_stream);
}

void LibproxProximityBase::requestProxSubstream(const UUID& objid, ProxStreamInfoPtr prox_stream) {
    using std::tr1::placeholders::_1;

    // Always mark this up front. This keeps further requests from
    // occuring since the first time this method is entered, even if
    // the request is deferred, should eventually result in a stream.
    prox_stream->iostream_requested = true;

    ObjectSession* session = mContext->objectSessionManager()->getSession(ObjectReference(objid));
    ProxObjectStreamPtr base_stream = session != NULL ? session->getStream() : ProxObjectStreamPtr();
    if (!base_stream) {
        mContext->mainStrand->post(
            Duration::milliseconds((int64)5),
            std::tr1::bind(&LibproxProximityBase::requestProxSubstream, this, objid, prox_stream)
        );
        return;
    }

    base_stream->createChildStream(
        mContext->mainStrand->wrap(
            std::tr1::bind(&LibproxProximityBase::proxSubstreamCallback, this, _1, objid, base_stream, _2, prox_stream)
        ),
        (void*)NULL, 0,
        OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY
    );
}

void LibproxProximityBase::proxSubstreamCallback(int x, const UUID& objid, ProxObjectStreamPtr parent_stream, ProxObjectStreamPtr substream, ProxStreamInfoPtr prox_stream_info) {
    if (!substream) {
        // If they disconnected, ignore
        bool valid_session = (mContext->objectSessionManager()->getSession(ObjectReference(objid)) != NULL);
        if (!valid_session) return;

        // Retry
        PROXLOG(warn,"Error opening Prox substream, retrying...");

        parent_stream->createChildStream(
            mContext->mainStrand->wrap(
                std::tr1::bind(&LibproxProximityBase::proxSubstreamCallback, this, _1, objid, parent_stream, _2, prox_stream_info)
            ),
            (void*)NULL, 0,
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY
        );

        return;
    }

    prox_stream_info->iostream = substream;
    assert(!prox_stream_info->writing);
    writeSomeObjectResults(prox_stream_info);
}


void LibproxProximityBase::writeSomeObjectResults(ProxStreamInfoWPtr w_prox_stream) {
    static Duration retry_rate = Duration::milliseconds((int64)1);

    ProxStreamInfoPtr prox_stream = w_prox_stream.lock();
    if (!prox_stream) return;

    prox_stream->writing = true;

    if (!prox_stream->iostream) {
        // We're still waiting on the iostream, proxSubstreamCallback will call
        // us when it gets the iostream.
        prox_stream->writing = false;
        return;
    }

    // Otherwise, keep sending until we run out or
    while(!prox_stream->outstanding.empty()) {
        std::string& framed_prox_msg = prox_stream->outstanding.front();
        int bytes_written = prox_stream->iostream->write((const uint8*)framed_prox_msg.data(), framed_prox_msg.size());
        if (bytes_written < 0) {
            // FIXME
            break;
        }
        else if (bytes_written < (int)framed_prox_msg.size()) {
            framed_prox_msg = framed_prox_msg.substr(bytes_written);
            break;
        }
        else {
            prox_stream->outstanding.pop();
        }
    }

    if (prox_stream->outstanding.empty())
        prox_stream->writing = false;
    else
        mContext->mainStrand->post(retry_rate, prox_stream->writecb);
}

} // namespace Sirikata
