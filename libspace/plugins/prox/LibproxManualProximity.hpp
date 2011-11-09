// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPROX_MANUAL_PROXIMITY_HPP_
#define _SIRIKATA_LIBPROX_MANUAL_PROXIMITY_HPP_

#include "LibproxProximityBase.hpp"

namespace Sirikata {

/** Implementation of Proximity using manual traversal of the object tree. The
 *  Proximity library provides the tree and a simple interface to manage cuts and
 *  the client manages the cut.
 */
class LibproxManualProximity : public LibproxProximityBase {
public:
    LibproxManualProximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr);
    ~LibproxManualProximity();

    // Service Interface overrides
    virtual void start();

    // Objects
    virtual void addQuery(UUID obj, SolidAngle sa, uint32 max_results);
    virtual void addQuery(UUID obj, const String& params);
    virtual void removeQuery(UUID obj);

    // PollingService Interface
    virtual void poll();

    // MigrationDataClient Interface
    virtual std::string migrationClientTag();
    virtual std::string generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server);
    virtual void receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data);

    virtual void onObjectHostSession(const OHDP::NodeID& id, OHDPSST::Stream::Ptr oh_stream);
    virtual void onObjectHostSessionEnded(const OHDP::NodeID& id);

private:

    // MAIN Thread:

    // ObjectHost message management
    void handleObjectHostSubstream(int success, OHDPSST::Stream::Ptr substream);
    void handleObjectHostProxMessage(const OHDP::NodeID& id, String& data);


    // PROX Thread:

}; // class LibproxManualProximity

} // namespace Sirikata

#endif //_SIRIKATA_LIBPROX_MANUAL_PROXIMITY_HPP_
