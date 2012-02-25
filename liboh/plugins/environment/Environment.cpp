// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "Environment.hpp"
#include <sirikata/oh/HostedObject.hpp>
#include <json_spirit/json_spirit.h>

#define ENVIRONMENT_SERVICE_PORT 23
#define ENV_LOG(lvl,msg) SILOG(environment,lvl,msg);

namespace Sirikata {

namespace json = json_spirit;

EnvironmentSimulation::EnvironmentSimulation(HostedObjectPtr ho, const SpaceObjectReference& pres)
 : mParent(ho),
   mPresence(pres),
   mEnvironment(json::Object()),
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
        if (!mEnvironment.contains(key))
            return boost::any();
        json::Value val = mEnvironment.get(key);
        switch(val.type()) {
          case json::Value::NULL_TYPE:
            return boost::any(); break;
          case json::Value::OBJECT_TYPE:
            ENV_LOG(error, "Got object for key " << key << " but can't currently return objects.");
            return boost::any(); break;
          case json::Value::ARRAY_TYPE:
            ENV_LOG(error, "Got array for key " << key << " but can't currently return arrays.");
            return boost::any(); break;
          case json::Value::STRING_TYPE:
            return Invokable::asAny(val.getString()); break;
          case json::Value::BOOL_TYPE:
            return Invokable::asAny(val.getBool()); break;
          case json::Value::INT_TYPE:
            return Invokable::asAny(val.getInt()); break;
          case json::Value::REAL_TYPE:
            return Invokable::asAny(val.getReal()); break;
          default:
            return boost::any(); break;
        }
        return boost::any();
    }
    else if (name == "set") {
        if (params.size() < 3
            || !Invokable::anyIsString(params[1])
            || (!Invokable::anyIsString(params[2])
                && !Invokable::anyIsBoolean(params[2])
                && !Invokable::anyIsNumeric(params[2])))
            return boost::any();

        String key = Invokable::anyAsString(params[1]);
        try {
            if (Invokable::anyIsString(params[2]))
                mEnvironment.put(key, Invokable::anyAsString(params[2]));
            else if (Invokable::anyIsNumeric(params[2]))
                mEnvironment.put(key, Invokable::anyAsNumeric(params[2]));
            else if (Invokable::anyIsBoolean(params[2]))
                mEnvironment.put(key, Invokable::anyAsBoolean(params[2]));
            else
                return Invokable::asAny(false);

            sendUpdate();
        }
        catch (const json::Value::PathError& e) {
            // We couldn't insert to the path, e.g. because bar in
            // foo.bar.baz wasn't an object.
            return Invokable::asAny(false);
        }
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
            std::tr1::bind(&EnvironmentSimulation::notifyListener, this),
            "EnvironmentSimulation::notifyListener"
        );
        return Invokable::asAny(true);
    }

    return boost::any();
}

void EnvironmentSimulation::handleCreatedStream(int err, ODPSST::Stream::Ptr strm) {
    if (err != SST_IMPL_SUCCESS) {
        ENV_LOG(error, "Failed to create substream for environment messages. This might happen if the environment service isn't running on the space server.");
        return;
    }
    mStream = strm;
    mRecordStream.initialize(
        mStream,
        std::tr1::bind(&EnvironmentSimulation::handleMessage, this, std::tr1::placeholders::_1)
    );
}

void EnvironmentSimulation::handleMessage(MemoryReference data) {
    ENV_LOG(insane, "Received message: " << String((char*)data.begin(), data.size()));

    // Currently just receiving whole thing every time
    std::string new_env_json((char*)data.begin(), data.size());
    json::Value new_env;
    if (!json::read(new_env_json, new_env)) {
        ENV_LOG(error, "Couldn't parse new environment: " << new_env_json);
        return;
    }

    mEnvironment = new_env;
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
    std::string serialized = json::write(mEnvironment);
    ENV_LOG(insane, "Sending update: " << serialized);
    mRecordStream.write(MemoryReference(serialized));
}

} // namespace Sirikata
