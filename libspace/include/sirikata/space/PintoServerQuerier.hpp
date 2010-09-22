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

namespace Sirikata {

/** Listener interface for PintoServerQuerier. Receives updates on which
 *  servers need to be queried based on the submitted query information.
 */
class SIRIKATA_SPACE_EXPORT PintoServerQuerierListener {
public:
    virtual ~PintoServerQuerierListener() {}

    virtual void addRelevantServer(ServerID sid) = 0;
    virtual void removeRelevantServer(ServerID sid) = 0;
};

/** PintoServerQuerier is an interface for discovering other space servers which
 *  must be queried for Pinto results. It looks a lot like Pinto itself since it
 *  performs similar queries but for discovering servers instead of objects. For
 *  a single server setup, the implementation would be trivial.  Other
 *  implementations might be a single server that the PintoServerQuerier submits
 *  queries to, or perhaps a multiple server service that is fault tolerant.
 */
class SIRIKATA_SPACE_EXPORT PintoServerQuerier : public Provider<PintoServerQuerierListener*> {
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

    /** Update query parameters with the server.
     *  \param min_angle the smallest query angle requested
     */
    virtual void updateQuery(const SolidAngle& min_angle) = 0;

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
