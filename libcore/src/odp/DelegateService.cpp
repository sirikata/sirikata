/*  Sirikata
 *  DelegateService.cpp
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/odp/DelegateService.hpp>
#include <sirikata/core/odp/DelegatePort.hpp>

namespace Sirikata {
namespace ODP {

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

Port* DelegateService::bindODPPort(const SpaceID& space, const ObjectReference& objref, PortID port) {
    return bindODPPort(SpaceObjectReference(space, objref), port);
}

Port* DelegateService::bindODPPort(const SpaceObjectReference& sor, PortID port) {
    PortMap* pm = getOrCreatePortMap(sor);

    PortMap::iterator it = pm->find(port);
    if (it != pm->end()) {
        // Already allocated
        return NULL;
    }

    DelegatePort* new_port = mCreator(this, sor, port);
    if (new_port != NULL) {
        (*pm)[port] = new_port;
    }
    return new_port;
}

Port* DelegateService::bindODPPort(const SpaceID& space, const ObjectReference& objref) {
    return bindODPPort(SpaceObjectReference(space, objref));
}

Port* DelegateService::bindODPPort(const SpaceObjectReference& sor) {
    // FIXME we should probably do some more intelligent tracking here, maybe
    // keeping track of free blocks of ports...

    // 10000 is completely arbitrary and probably too high...
    for(uint32 i = 0; i < 10000; i++) {
        PortID port_id = rand() % 32767;
        Port* result = bindODPPort(sor, port_id);
        if (result != NULL) return result;
    }
    return NULL;
}

void DelegateService::registerDefaultODPHandler(const MessageHandler& cb) {
    mDefaultHandler = cb;
}

bool DelegateService::deliver(const Endpoint& src, const Endpoint& dst, MemoryReference data) const {
    // Check from most to least specific
    PortMap const* pm = getPortMap(dst.spaceObject());
    if (pm != NULL) {
        PortMap::const_iterator it = pm->find(dst.port());
        if (it != pm->end()) {
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
    PortMap* pm = getPortMap(port->endpoint().spaceObject());
    if (pm == NULL)
        return;

    pm->erase(port->endpoint().port());
}

DelegateService::PortMap* DelegateService::getPortMap(const SpaceObjectReference& sor) const {
    SpacePortMap::const_iterator it = mSpacePortMap.find(sor);

    if (it != mSpacePortMap.end())
        return it->second;

    return NULL;
}

DelegateService::PortMap* DelegateService::getOrCreatePortMap(const SpaceObjectReference& sor) {
    PortMap* pm = getPortMap(sor);
    if (pm != NULL)
        return pm;

    PortMap* new_port_map = new PortMap();
    mSpacePortMap[sor] = new_port_map;
    return new_port_map;
}

} // namespace ODP
} // namespace Sirikata
