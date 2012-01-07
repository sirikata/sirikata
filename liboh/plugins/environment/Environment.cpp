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
   mPresence(pres),
   mEnvironment(),
   mStream(),
   mRecordStream(),
   mListener(NULL)
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
    // Decode the command. First argument is the "function name"
    if (params.empty() || !Invokable::anyIsString(params[0]))
        return boost::any();

    String name = Invokable::anyAsString(params[0]);

    if (name == "get") {
        if (params.size() < 2 || !Invokable::anyIsString(params[1]))
            return boost::any();
        String key = Invokable::anyAsString(params[1]);
        String val;
        try {
            val = mEnvironment.get<std::string>(key);
        } catch (ptree_bad_path exc) {
            return boost::any();
        }
        return Invokable::asAny(val);
    }
    else if (name == "set") {
        if (params.size() < 3
            || !Invokable::anyIsString(params[1])
            || !Invokable::anyIsString(params[2]))
            return boost::any();
        String key = Invokable::anyAsString(params[1]);
        String val = Invokable::anyAsString(params[2]);
        mEnvironment.put(key, val);
        sendUpdate();
        return Invokable::asAny(true);
    }
    else if (name == "listen") {
        // This gets triggers callbacks when we receive updates
        if (params.size() < 2 || !Invokable::anyIsInvokable(params[1]))
            return boost::any();
        Invokable* listener = Invokable::anyAsInvokable(params[1]);
        mListener = listener;
        // Also trigger a (delayed) callback immediately
        mParent->context()->mainStrand->post(
            std::tr1::bind(&EnvironmentSimulation::notifyListener, this)
        );
        return Invokable::asAny(true);
    }

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

    notifyListener();
}

void EnvironmentSimulation::notifyListener() {
    // Trigger update to listener if we have one. We currently don't
    // pass anything like a delta. The user is responsible for getting
    // keys they care about and checking whether they've been updated.
    if (mListener)
        mListener->invoke();
}

void EnvironmentSimulation::sendUpdate() {
    // Currently we just always serialize and send the whole thing
    std::stringstream data_json;
    write_json(data_json, mEnvironment);
    String serialized = data_json.str();
    ENV_LOG(insane, "Sending update: " << serialized);
    mRecordStream.write(MemoryReference(serialized));
}

} // namespace Sirikata
