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

#ifndef _SIRIKATA_OHCOORDINATOR_PROXIMITY_HPP_
#define _SIRIKATA_OHCOORDINATOR_PROXIMITY_HPP_

#include <sirikata/ohcoordinator/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/util/SolidAngle.hpp>

#include <sirikata/core/service/PollingService.hpp>

#include <sirikata/ohcoordinator/LocationService.hpp>
#include <sirikata/ohcoordinator/CoordinateSegmentation.hpp>
#include <sirikata/ohcoordinator/MigrationDataClient.hpp>
#include <sirikata/ohcoordinator/SpaceNetwork.hpp>
#include <sirikata/ohcoordinator/ObjectHostSession.hpp>

#include <sirikata/core/util/Factory.hpp>

namespace Sirikata {

class ProximityInputEvent;
class ProximityOutputEvent;
class AggregateManager;

class SIRIKATA_OHCOORDINATOR_EXPORT Proximity :
        public PollingService,
        MigrationDataClient,
        CoordinateSegmentation::Listener,
        SpaceNetworkConnectionListener,
        public ObjectSessionListener,
        public ObjectHostSessionListener,
        protected LocationServiceListener,
        protected MessageRecipient
{
  public:
    Proximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr, const Duration& poll_freq);
    virtual ~Proximity();

    // ** These interfaces are required

    // Service Interface overrides
    virtual void start();
    virtual void stop();

    // Objects
    virtual void addQuery(UUID obj, SolidAngle sa, uint32 max_results) = 0;
    virtual void addQuery(UUID obj, const String& params) = 0;
    virtual void removeQuery(UUID obj) = 0;

    // PollingService Interface
    virtual void poll();

    // MigrationDataClient Interface
    virtual std::string migrationClientTag() = 0;
    virtual std::string generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server) = 0;
    virtual void receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data) = 0;

    // ** These interfaces are stubbed out because you don't necessarily need to
    // ** override them. Some subsets, however, are required, i.e. at least
    // ** either ObjectSessionListener or ObjectHostSessionListener

    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_seg) { }

    // SpaceNetworkConnectionListener Interface
    virtual void onSpaceNetworkConnected(ServerID sid) { }
    virtual void onSpaceNetworkDisconnected(ServerID sid) { }

    // ObjectSessionListener Interface
    virtual void newSession(ObjectSession* session) { }
    virtual void sessionClosed(ObjectSession* session) { }

    // ObjectHostSessionListener Interface
    virtual void onObjectHostSession(const OHDP::NodeID& id, ObjectHostSessionPtr oh_sess) { }
    virtual void onObjectHostSessionEnded(const OHDP::NodeID& id) { }

    // LocationServiceListener Interface
    // Implement as necessary, some updates may be ignored

    // MessageRecipient Interface
    virtual void receiveMessage(Message* msg) { }

  protected:

    // MAIN Thread

    // Update stats server
    void reportStats();

    virtual int32 objectQueries() const { return 0; }
    virtual int32 objectHostQueries() const { return 0; }
    virtual int32 serverQueries() const { return 0; }

    // Fields
    // Thread safe:

    SpaceContext* mContext;

    // NOTE: MAIN Thread access only unless you know the methods are
    // thread-safe

    LocationService* mLocService;
    CoordinateSegmentation* mCSeg;

    AggregateManager* mAggregateManager;

    // Stats
    Poller mStatsPoller;
    const String mTimeSeriesObjectQueryCountName;
    const String mTimeSeriesObjectHostQueryCountName;
    const String mTimeSeriesServerQueryCountName;

}; //class Proximity


class SIRIKATA_OHCOORDINATOR_EXPORT ProximityFactory
    : public AutoSingleton<ProximityFactory>,
      public Factory6<Proximity*, SpaceContext*, LocationService*, CoordinateSegmentation*, SpaceNetwork*, AggregateManager*, const String&>
{
  public:
    static ProximityFactory& getSingleton();
    static void destroy();
}; // class ProximityFactory


} // namespace Sirikata

#endif //_SIRIKATA_OHCOORDINATOR_PROXIMITY_HPP_
