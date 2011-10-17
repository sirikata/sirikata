// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "SimpleObjectQueryProcessor.hpp"
#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Prox.pbj.hpp"

namespace Sirikata {
namespace OH {
namespace Simple {

SimpleObjectQueryProcessor* SimpleObjectQueryProcessor::create(ObjectHostContext* ctx, const String& args) {
    return new SimpleObjectQueryProcessor(ctx);
}


SimpleObjectQueryProcessor::SimpleObjectQueryProcessor(ObjectHostContext* ctx)
 : mContext(ctx)
{
}

SimpleObjectQueryProcessor::~SimpleObjectQueryProcessor() {
}

void SimpleObjectQueryProcessor::start() {
}

void SimpleObjectQueryProcessor::stop() {
}

void SimpleObjectQueryProcessor::presenceConnectedStream(HostedObjectPtr ho, const SpaceObjectReference& sporef, HostedObject::SSTStreamPtr strm) {
    strm->listenSubstream(OBJECT_PORT_PROXIMITY,
        std::tr1::bind(&SimpleObjectQueryProcessor::handleProximitySubstream, this,
            HostedObjectWPtr(ho), sporef, _1, _2
        )
    );
}

void SimpleObjectQueryProcessor::handleProximitySubstream(const HostedObjectWPtr& weakHO, const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s) {
    String* prevdata = new String();
    s->registerReadCallback(
        std::tr1::bind(&SimpleObjectQueryProcessor::handleProximitySubstreamRead, this,
            weakHO, spaceobj, s, prevdata, _1, _2
        )
    );
}

void SimpleObjectQueryProcessor::handleProximitySubstreamRead(const HostedObjectWPtr& weakHO, const SpaceObjectReference& spaceobj, SSTStreamPtr s, String* prevdata, uint8* buffer, int length) {
    HostedObjectPtr self(weakHO.lock());
    if (!self)
        return;
    if (self->stopped()) {
        SILOG(simple-object-query-processor,detailed,"Ignoring proximity update after system stop requested.");
        return;
    }

    prevdata->append((const char*)buffer, length);

    while(true) {
        std::string msg = Network::Frame::parse(*prevdata);

        // If we don't have a full message, just wait for more
        if (msg.empty()) return;

        // Otherwise, try to handle it
        handleProximityMessage(self, spaceobj, msg);
    }

    // FIXME we should be getting a callback on stream close so we can clean up!
    //s->registerReadCallback(0);
}

bool SimpleObjectQueryProcessor::handleProximityMessage(HostedObjectPtr self, const SpaceObjectReference& spaceobj, const std::string& payload)
{
    Sirikata::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(payload);
    if (!parse_success)
        return false;

    deliverResults(self, spaceobj, contents);
    return true;
}


void SimpleObjectQueryProcessor::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query) {
    Protocol::Prox::QueryRequest request;
    request.set_query_parameters(new_query);
    std::string payload = serializePBJMessage(request);

    SSTStreamPtr spaceStream = mContext->objectHost->getSpaceStream(sporef.space(), sporef.object());
    if (spaceStream != SSTStreamPtr()) {
        SSTConnectionPtr conn = spaceStream->connection().lock();
        assert(conn);

        conn->datagram(
            (void*)payload.data(), payload.size(),
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY,
            NULL
        );
    }
}

} // namespace Simple
} // namespace OH
} // namespace Sirikata
