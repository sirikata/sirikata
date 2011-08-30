/*  Sirikata
 *  Proximity.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_SPACE_PROXIMITY_HPP_
#define _SIRIKATA_SPACE_PROXIMITY_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/util/SolidAngle.hpp>

#include <sirikata/core/service/PollingService.hpp>

#include <sirikata/space/CoordinateSegmentation.hpp>
#include <sirikata/space/MigrationDataClient.hpp>
#include <sirikata/space/SpaceNetwork.hpp>

#include <sirikata/core/util/Factory.hpp>

namespace Sirikata {

class LocationService;
class ProximityInputEvent;
class ProximityOutputEvent;

class SIRIKATA_SPACE_EXPORT Proximity :
        public PollingService,
        MigrationDataClient,
        CoordinateSegmentation::Listener,
        SpaceNetworkConnectionListener,
        public ObjectSessionListener
{
  public:
    Proximity(SpaceContext* ctx, LocationService* locservice, SpaceNetwork* net, const Duration& poll_freq);
    virtual ~Proximity();

    // Initialize prox.  Must be called after everything else (specifically message router) is set up since it
    // needs to send messages.
    virtual void initialize(CoordinateSegmentation* cseg);
    // Shutdown the proximity thread.
    virtual void shutdown();

    // Objects
    virtual void addQuery(UUID obj, SolidAngle sa, uint32 max_results) = 0;
    virtual void removeQuery(UUID obj) = 0;

    // PollingService Interface
    virtual void poll();

    // MigrationDataClient Interface
    virtual std::string migrationClientTag() = 0;
    virtual std::string generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server) = 0;
    virtual void receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data) = 0;

    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_seg) = 0;

    // SpaceNetworkConnectionListener Interface
    virtual void onSpaceNetworkConnected(ServerID sid) = 0;
    virtual void onSpaceNetworkDisconnected(ServerID sid) = 0;

    // ObjectSessionListener Interface
    virtual void newSession(ObjectSession* session) = 0;
    virtual void sessionClosed(ObjectSession* session) = 0;

  protected:
    SpaceContext* mContext;

    // NOTE: MAIN Thread access only unless you know the methods are
    // thread-safe

    LocationService* mLocService;
    CoordinateSegmentation* mCSeg;

}; //class Proximity


class SIRIKATA_SPACE_EXPORT ProximityFactory
    : public AutoSingleton<ProximityFactory>,
      public Factory4<Proximity*, SpaceContext*, LocationService*, SpaceNetwork*, const String&>
{
  public:
    static ProximityFactory& getSingleton();
    static void destroy();
}; // class ProximityFactory


} // namespace Sirikata

#endif //_SIRIKATA_SPACE_PROXIMITY_HPP_
