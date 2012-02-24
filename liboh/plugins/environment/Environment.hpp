// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_ENVIRONMENT_CLIENT_SIMULATION_HPP_
#define _SIRIKATA_ENVIRONMENT_CLIENT_SIMULATION_HPP_

#include <sirikata/oh/Simulation.hpp>
#include <sirikata/core/service/Context.hpp>

#include <json_spirit/value.h>

#include <sirikata/core/odp/SST.hpp>
#include <sirikata/core/network/RecordSSTStream.hpp>

namespace Sirikata {

class HostedObject;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;

/** Client for the Environment SpaceModule. It syncs data with the space and
 *  exposes it via the Invokable interface.
 */
class EnvironmentSimulation : public Simulation {
public:
    EnvironmentSimulation(HostedObjectPtr ho, const SpaceObjectReference& pres);

    // Service Interface
    virtual void start();
    virtual void stop();

    // Invokable Interface
    virtual boost::any invoke(std::vector<boost::any>& params);

private:
    void handleCreatedStream(int err, ODPSST::Stream::Ptr strm);
    void handleMessage(MemoryReference data);

    void notifyListener();

    void sendUpdate();

    HostedObjectPtr mParent;
    SpaceObjectReference mPresence;

    // The environment data
    json_spirit::Value mEnvironment;

    // Communication with the space
    ODPSST::Stream::Ptr mStream;
    RecordSSTStream<ODPSST::Stream::Ptr> mRecordStream;

    // Listener for updates
    Invokable* mListener;
};

} // namespace Sirikata

#endif //_SIRIKATA_ENVIRONMENT_CLIENT_SIMULATION_HPP_
