/*  Sirikata
 *  OrphanLocUpdateManager.hpp
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

#ifndef _SIRIKATA_ORPHAN_LOC_UPDATE_MANAGER_HPP_
#define _SIRIKATA_ORPHAN_LOC_UPDATE_MANAGER_HPP_

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>

namespace Sirikata {

namespace Protocol {
namespace Loc {
class LocationUpdate;
}
}

/** OrphanLocUpdateManager keeps track of loc updates that arrived before the
 *  corresponding prox update so they can be delivered when the prox update does
 *  arrive. Without an OrphanLocUpdateManager, a loc update could be lost due to
 *  incorrect ordering, resulting in the correct information never being updated
 *  to its correct value.  Loc updates are saved for short time and then
 *  discarded.
 */
class SIRIKATA_PROXYOBJECT_EXPORT OrphanLocUpdateManager : public PollingService {
public:
    typedef Sirikata::Protocol::Loc::LocationUpdate LocUpdate;
    typedef std::vector<LocUpdate> UpdateList;

    OrphanLocUpdateManager(Context* ctx, Network::IOStrand* strand, const Duration& timeout);
    ~OrphanLocUpdateManager();

    /** Add an orphan update to the queue and set a timeout for it to be cleared
     *  out.
     */
    void addOrphanUpdate(const SpaceObjectReference& obj, const LocUpdate& update);

    /** Gets all orphan updates for a given object. */
    UpdateList getOrphanUpdates(const SpaceObjectReference& obj);

private:
    virtual void poll();

    struct UpdateInfo {
        LocUpdate* value;
        Time expiresAt;
    };
    typedef std::vector<UpdateInfo> UpdateInfoList;

    typedef std::tr1::unordered_map<SpaceObjectReference, UpdateInfoList, SpaceObjectReference::Hasher> ObjectUpdateMap;

    Context* mContext;
    Duration mTimeout;
    ObjectUpdateMap mUpdates;
}; // class OrphanLocUpdateManager


} // namespace Sirikata

#endif
