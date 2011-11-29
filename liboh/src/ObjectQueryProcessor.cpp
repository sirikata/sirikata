// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/ObjectQueryProcessor.hpp>
#include "Protocol_Loc.pbj.hpp"

AUTO_SINGLETON_INSTANCE(Sirikata::OH::ObjectQueryProcessorFactory);

namespace Sirikata {
namespace OH {

ObjectQueryProcessor::~ObjectQueryProcessor() {
}

// Implementations may or may not actually need these callbacks, so we
// provide dummy implementations for when they are not needed.

void ObjectQueryProcessor::presenceConnected(HostedObjectPtr ho, const SpaceObjectReference& sporef) {
}

void ObjectQueryProcessor::presenceConnectedStream(HostedObjectPtr ho, const SpaceObjectReference& sporef, HostedObject::SSTStreamPtr strm) {
}

void ObjectQueryProcessor::presenceDisconnected(HostedObjectPtr ho, const SpaceObjectReference& sporef) {
}

String ObjectQueryProcessor::connectRequest(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& query) {
    return query;
}


void ObjectQueryProcessor::deliverProximityUpdate(HostedObjectPtr ho, const SpaceObjectReference& sporef, const Sirikata::Protocol::Prox::ProximityUpdate& update) {
    ho->handleProximityUpdate(sporef, update);
}

void ObjectQueryProcessor::deliverLocationUpdate(HostedObjectPtr ho, const SpaceObjectReference& sporef, const LocUpdate& lu) {
    ho->handleLocationUpdate(sporef, lu);
}


ObjectQueryProcessorFactory& ObjectQueryProcessorFactory::getSingleton() {
    return AutoSingleton<ObjectQueryProcessorFactory>::getSingleton();
}

void ObjectQueryProcessorFactory::destroy() {
    AutoSingleton<ObjectQueryProcessorFactory>::destroy();
}

} // namespace OH
} // namespace Sirikata
