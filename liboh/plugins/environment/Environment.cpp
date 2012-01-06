// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "Environment.hpp"
#include <sirikata/oh/HostedObject.hpp>

#define ENVIRONMENT_SERVICE_PORT 23
#define ENV_LOG(lvl,msg) SILOG(environment,lvl,msg);

namespace Sirikata {

using namespace boost::property_tree;

EnvironmentSimulation::EnvironmentSimulation(HostedObjectPtr ho, const SpaceObjectReference& pres)
 : mParent(ho),
   mPresence(pres)
{
}

void EnvironmentSimulation::start() {
    ODPSST::Stream::Ptr stream = mParent->getSpaceStream(mPresence);
    if (!stream) return;

    stream->createChildStream(
        std::tr1::bind(&EnvironmentSimulation::handleCreatedStream, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2),
        NULL, 0,
        ENVIRONMENT_SERVICE_PORT, ENVIRONMENT_SERVICE_PORT
    );
}

void EnvironmentSimulation::stop() {
}

boost::any EnvironmentSimulation::invoke(std::vector<boost::any>& params) {
    return boost::any();
}

void EnvironmentSimulation::handleCreatedStream(int err, ODPSST::Stream::Ptr strm) {
    mStream = strm;
    mRecordStream.initialize(
        mStream,
        std::tr1::bind(&EnvironmentSimulation::handleMessage, this, std::tr1::placeholders::_1)
    );
}

void EnvironmentSimulation::handleMessage(MemoryReference data) {
    ENV_LOG(insane, "Received message: " << String((char*)data.begin(), data.size()));

    // Currently just receiving whole thing every time
    std::stringstream env_json(std::string((char*)data.begin(), data.size()));
    read_json(env_json, mEnvironment);
}

} // namespace Sirikata
