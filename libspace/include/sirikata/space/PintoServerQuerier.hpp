/*  Sirikata
 *  PintoServerQuerier.hpp
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

#ifndef _SIRIKATA_LIBSPACE_PINTO_SERVER_QUERIER_HPP_
#define _SIRIKATA_LIBSPACE_PINTO_SERVER_QUERIER_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/space/SpaceContext.hpp>
#include <sirikata/core/util/Factory.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/pintoloc/LocUpdate.hpp>

namespace Sirikata {

namespace Protocol {
namespace Prox {
class ProximityUpdate;
}
}

/** Listener interface for PintoServerQuerier. Receives updates on which
 *  servers need to be queried based on the submitted query information. These
 *  updates are encoded in the form of ProximityUpdates, which could be used for
 *  individual results (e.g. add/remove server) or for tree replication. This
 *  gives *some* flexibility in the format of the results without being
 *  completely generic (i.e. without just using a byte string).
 */
class SIRIKATA_SPACE_EXPORT PintoServerQuerierListener {
public:
    virtual ~PintoServerQuerierListener() {}

    virtual void onPintoServerResult(const Sirikata::Protocol::Prox::ProximityUpdate& update) = 0;
    virtual void onPintoServerLocUpdate(const LocUpdate& update) = 0;
};

/** PintoServerQuerier is an interface for discovering other space servers which
 *  must be queried for Pinto results. To support multiple implementations
 *  (e.g. just getting a list of servers to query based on aggregate solid angle
 *  query, getting a cut including very high level aggregates based on aggregate
 *  solid angle query, or replicating part or all of the top-level Pinto tree),
 *  it only assumes you'll keep the server up-to-date about the region covered
 *  by this server, the largest object on the server, and send query
 *  updates. Query updates are flexibly formatted. They may be 'static' queries,
 *  e.g. a solid angle query containing an angle and maximum number of results,
 *  or be commands, e.g. to refine or coarsen the current cut.
 */
class SIRIKATA_SPACE_EXPORT PintoServerQuerier : public Provider<PintoServerQuerierListener*>, public Service {
public:

    virtual ~PintoServerQuerier() {}

    /** Update this server's parameters.
     *  \param region bounding box of the region covered by this server
     */
    virtual void updateRegion(const BoundingBox3f& region) = 0;

    /** Update this server's parameters.
     *  \param max_radius size of the largest object in the region
     */
    virtual void updateLargestObject(float max_radius) = 0;

    /** Update this server's extra query data.
     *  \param qd the serialized query data for the region
     */
    virtual void updateQueryData(const String& qd) = 0;

    /** Update query parameters with the server.
     *  \param update a string containing the data to send to the server as an update
     */
    virtual void updateQuery(const String& update) = 0;

}; // PintoServerQuerier

class SIRIKATA_SPACE_EXPORT PintoServerQuerierFactory
    : public AutoSingleton<PintoServerQuerierFactory>,
      public Factory2<PintoServerQuerier*, SpaceContext*, const String &>
{
  public:
    static PintoServerQuerierFactory& getSingleton();
    static void destroy();
}; // class PintoServerQuerierFactory

} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_PINTO_SERVER_QUERIER_HPP_
