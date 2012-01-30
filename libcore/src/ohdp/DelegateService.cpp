// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/ohdp/DelegateService.hpp>
#include <sirikata/core/network/ObjectMessage.hpp>
#include <boost/functional/hash.hpp>

namespace Sirikata {
namespace OHDP {


bool DelegateService::SpaceIDNodeID::operator==(const SpaceIDNodeID& rhs) const {
    return (space == rhs.space && node == rhs.node);
}

bool DelegateService::SpaceIDNodeID::operator<(const SpaceIDNodeID& rhs) const {
    return (space < rhs.space ||
        (space == rhs.space && node < rhs.node)
    );
}

size_t DelegateService::SpaceIDNodeID::Hasher::operator()(const SpaceIDNodeID& snid) const {
    size_t seed = 0;
    boost::hash_combine(seed, SpaceID::Hasher()(snid.space));
    boost::hash_combine(seed, NodeID::Hasher()(snid.node));
    return seed;
}



DelegateService::DelegateService(PortCreateFunction create_func)
 : Service(),
   mCreator(create_func),
   mSpacePortMap(),
   mDefaultHandler()
{
}

DelegateService::~DelegateService() {
    while(!mSpacePortMap.empty()) {
        SpacePortMap::iterator spm_it = mSpacePortMap.begin();
        PortMap* pm = spm_it->second;

        while(!pm->empty()) {
            PortMap::iterator pm_it = pm->begin();
            DelegatePort* prt = pm_it->second;
            prt->invalidate(); // Will remove from pm_it via deallocatePort
        }

        mSpacePortMap.erase(spm_it);
        delete pm;
    }
}

Port* DelegateService::bindOHDPPort(const SpaceID& space, const NodeID& node, PortID port) {
    PortMap* pm = getOrCreatePortMap(SpaceIDNodeID(space, node));

    PortMap::iterator it = pm->find(port);
    if (it != pm->end()) {
        // Already allocated
        return NULL;
    }

    Endpoint ept(space, node, port);
    DelegatePort* new_port = mCreator(this, ept);
    if (new_port != NULL) {
        (*pm)[port] = new_port;
    }
    return new_port;
}

Port* DelegateService::bindOHDPPort(const SpaceID& space, const NodeID& node) {
    // FIXME we should probably do some more intelligent tracking here, maybe
    // keeping track of free blocks of ports...

    PortID port_id = unusedOHDPPort(space, node);
    if (port_id == PortID::null()) return NULL;
    return bindOHDPPort(space, node, port_id);
}

PortID DelegateService::unusedOHDPPort(const SpaceID& space, const NodeID& node) {
    // We want to make this efficient in most cases, but need to make it
    // exhaustively search for available ports when we can't easily find a free
    // one. The approach is simple: choose a random port to start with and check
    // if it's free. From there, perform a linear search (which must wrap and
    // avoid reserved ports). This won't perform well in extreme cases where we
    // the port space is almost completely full...

    PortID starting_port_id = (rand() % (OBJECT_PORT_SYSTEM_MAX-OBJECT_PORT_SYSTEM_RESERVED_MAX)) + (OBJECT_PORT_SYSTEM_RESERVED_MAX+1);

    // If we don't have a PortMap yet, then its definitely not allocated
    SpaceIDNodeID snid(space, node);
    PortMap* pm = getPortMap(snid);
    if (pm == NULL) return starting_port_id;

    // Loop until we hit the starting point again
    PortID port_id = starting_port_id;
    do {
        // This port may be allocated already
        PortMap::iterator it = pm->find(port_id);
        if (it == pm->end())
            return port_id;

        // Otherwise we need to move on, ensuring looping of Port IDs.
        port_id++;
        if (port_id > PortID(OBJECT_PORT_SYSTEM_MAX))
            port_id = OBJECT_PORT_SYSTEM_RESERVED_MAX+1;
    } while(port_id != starting_port_id);

    // If we get here, we're really out of ports
    SILOG(odp-delegate-service, error, "Exhausted OHDP ports for " << space << ":" << node);
    return PortID::null();
}

void DelegateService::registerDefaultOHDPHandler(const MessageHandler& cb) {
    mDefaultHandler = cb;
}

bool DelegateService::deliver(const Endpoint& src, const Endpoint& dst, MemoryReference data) const {
    // Check from most to least specific
    SpaceIDNodeID snid(dst.space(), dst.node());
    PortMap const* pm = getPortMap(snid);
    if (pm != NULL) {
        PortMap::const_iterator it = pm->find(dst.port());
        
        if (it != pm->end())
        {
            DelegatePort* port = it->second;
            bool delivered = port->deliver(src, dst, data);
            if (delivered)
                return true;
        }
    }

    // And finally, the default handler
    if (mDefaultHandler != 0) {
        mDefaultHandler(src, dst, data);
        return true;
    }

    return false;
}

void DelegateService::deallocatePort(DelegatePort* port) {
    SpaceIDNodeID snid(port->endpoint().space(), port->endpoint().node());
    PortMap* pm = getPortMap(snid);
    if (pm == NULL)
        return;

    pm->erase(port->endpoint().port());
}

DelegateService::PortMap* DelegateService::getPortMap(const SpaceIDNodeID& snid) const {
    SpacePortMap::const_iterator it = mSpacePortMap.find(snid);

    if (it != mSpacePortMap.end())
        return it->second;

    return NULL;
}

DelegateService::PortMap* DelegateService::getOrCreatePortMap(const SpaceIDNodeID& snid) {
    PortMap* pm = getPortMap(snid);
    if (pm != NULL)
        return pm;

    PortMap* new_port_map = new PortMap();
    mSpacePortMap[snid] = new_port_map;
    return new_port_map;
}

} // namespace OHDP
} // namespace Sirikata
