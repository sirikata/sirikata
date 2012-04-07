/*  Sirikata liboh -- Object Host
 *  ObjectHost.cpp
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <boost/thread.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/oh/ConnectionEventListener.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/oh/ObjectQueryProcessor.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>

#define OH_LOG(lvl,msg) SILOG(oh,lvl,msg)

namespace Sirikata {

ObjectHost::ObjectHost(ObjectHostContext* ctx, Network::IOService *ioServ, const String&options)
 : mContext(ctx),
   mStorage(NULL),
   mPersistentSet(NULL),
   mQueryProcessor(NULL),
   mActiveHostedObjects(0)
{
    mContext->objectHost = this;
    OptionValue *protocolOptions;
    OptionValue *scriptManagers;
    OptionValue *simOptions;
    InitializeClassOptions ico("objecthost",this,
                           protocolOptions=new OptionValue("protocols","",OptionValueType<std::map<std::string,std::string> >(),"passes options into protocol specific libraries like \"tcpsst:{--send-buffer-size=1440 --parallel-sockets=1},udp:{--send-buffer-size=1500}\""),
                           scriptManagers=new OptionValue("scriptManagers","simplecamera:{},js:{}",OptionValueType<std::map<std::string,std::string> >(),"Instantiates script managers with specified options like \"simplecamera:{},js:{--import-paths=/path/to/scripts}\""),
                           simOptions=new OptionValue("simOptions","ogregraphics:{}",OptionValueType<std::map<std::string,std::string> >(),"Passes initialization strings to simulations, by name"),

                           NULL);

    OptionSet* oh_options = OptionSet::getOptions("objecthost",this);
    oh_options->parse(options);
    mSimOptions=simOptions->as<std::map<std::string,std::string> > ();
    {
        std::map<std::string,std::string> *options=&protocolOptions->as<std::map<std::string,std::string> > ();
        for (std::map<std::string,std::string>::iterator i=options->begin(),ie=options->end();i!=ie;++i) {
            mSpaceConnectionProtocolOptions[i->first]=Network::StreamFactory::getSingleton().getOptionParser(i->first)(i->second);
        }
    }

    {
        std::map<std::string,std::string> *options=&scriptManagers->as<std::map<std::string,std::string> > ();
        for (std::map<std::string,std::string>::iterator i=options->begin(),ie=options->end();i!=ie;++i) {
            if (!ObjectScriptManagerFactory::getSingleton().hasConstructor(i->first)) continue;
            ObjectScriptManager* newmgr = ObjectScriptManagerFactory::getSingleton().getConstructor(i->first)(mContext, i->second);
            if (newmgr)
                mScriptManagers[i->first] = newmgr;
        }
    }

    if (mContext->commander() != NULL) {
        mContext->commander()->registerCommand(
            "oh.objects.list",
            mContext->mainStrand->wrap(std::tr1::bind(&ObjectHost::commandListObjects, this, _1, _2, _3))
        );
        mContext->commander()->registerCommand(
            "oh.objects.create",
            mContext->mainStrand->wrap(std::tr1::bind(&ObjectHost::commandCreateObject, this, _1, _2, _3))
        );
        mContext->commander()->registerCommand(
            "oh.objects.destroy",
            mContext->mainStrand->wrap(std::tr1::bind(&ObjectHost::commandDestroyObject, this, _1, _2, _3))
        );

        // To register only one copy of the command, we register HO commands
        // here and dispatch them to the appropriate HO.
        mContext->commander()->registerCommand(
            "oh.objects.presences",
            mContext->mainStrand->wrap(std::tr1::bind(&ObjectHost::commandObjectPresences, this, _1, _2, _3))
        );

    }
}

ObjectHost::~ObjectHost()
{
    {
        HostedObjectMap objs;
        mHostedObjects.swap(objs);
        for (HostedObjectMap::iterator iter = objs.begin();
                 iter != objs.end();
                 ++iter) {
            iter->second->destroy();
        }
        objs.clear(); // The HostedObject destructor will attempt to delete from mHostedObjects
    }
}

HostedObjectPtr ObjectHost::createObject(const String& script_type, const String& script_opts, const String& script_contents) {
    // FIXME we could ensure that the random ID isn't actually in use already
    return createObject(UUID::random(), script_type, script_opts, script_contents);
}

HostedObjectPtr ObjectHost::createObject(const UUID &uuid, const String& script_type, const String& script_opts, const String& script_contents) {
    mActiveHostedObjects++;
    HostedObjectPtr ho = HostedObject::construct<HostedObject>(mContext, this, uuid);

    // Safe weak reference by internal id. This lets us use the internal ID to
    // uniquely reference the object and look it up, e.g. for external commands
    assert(mHostedObjectsByID.find(uuid) == mHostedObjectsByID.end());
    mHostedObjectsByID[uuid] = ho;

    ho->start();
    // NOTE: This condition has been carefully thought through. Since you can
    // get a script into a dead state anyway, we only trigger defaults when the
    // script type is empty. This basically covers the case where an object
    // factory didn't request a specific script and, for simplicity, we just
    // want to setup a default. In the case of dynamically created entities, we
    // always respect the settings and the script_type should *always* be
    // set. If they request a dead object, we let it through -- the object may
    // still be useful (as it can still get automatically connected to a space
    // and have its properties set upon connection) -- even if it will result in
    // an object which can never execute additional scripted code.
    if (!script_type.empty())
        ho->initializeScript(script_type, script_opts, script_contents);
    else
        ho->initializeScript(this->defaultScriptType(), this->defaultScriptOptions(), this->defaultScriptContents());

    return ho;
}

// Space API - Provide info for ObjectHost to communicate with spaces
void ObjectHost::addServerIDMap(const SpaceID& space_id, ServerIDMap* sidmap) {
    SessionManager* smgr = new SessionManager(
        mContext, space_id, sidmap,
        std::tr1::bind(&ObjectHost::handleObjectConnected, this, _1, _2),
        std::tr1::bind(&ObjectHost::handleObjectMigrated, this, _1, _2, _3),
        std::tr1::bind(&ObjectHost::handleObjectMessage, this, _1, space_id, _2),
        std::tr1::bind(&ObjectHost::handleObjectDisconnected, this, _1, _2)
    );
    smgr->registerDefaultOHDPHandler(
        std::tr1::bind(&ObjectHost::handleDefaultOHDPMessageHandler, this, _1, _2, _3)
    );
    smgr->addListener(static_cast<SpaceNodeSessionListener*>(this));
    mSessionManagers[space_id] = smgr;
    smgr->start();
}

void ObjectHost::handleObjectConnected(const SpaceObjectReference& sporef_objid, ServerID server) {
    ObjectNodeSessionProvider::notify(&ObjectNodeSessionListener::onObjectNodeSession, sporef_objid.space(), sporef_objid.object(), OHDP::NodeID(server));
}

void ObjectHost::handleObjectMigrated(const SpaceObjectReference& sporef_objid, ServerID from, ServerID to) {
    ObjectNodeSessionProvider::notify(&ObjectNodeSessionListener::onObjectNodeSession, sporef_objid.space(), sporef_objid.object(), OHDP::NodeID(to));
}

void ObjectHost::handleObjectDisconnected(const SpaceObjectReference& sporef_objid, Disconnect::Code) {
    ObjectNodeSessionProvider::notify(&ObjectNodeSessionListener::onObjectNodeSession, sporef_objid.space(), sporef_objid.object(), OHDP::NodeID::null());
}

//use this function to request the object host to send a disconnect message
//to space for object
void ObjectHost::disconnectObject(const SpaceID& space, const ObjectReference& oref)
{
    SpaceSessionManagerMap::iterator iter = mSessionManagers.find(space);
    if (iter == mSessionManagers.end())
        return;

    iter->second->disconnect(SpaceObjectReference(space,oref));
}

void ObjectHost::handleObjectMessage(const SpaceObjectReference& sporef_internalID, const SpaceID& space, Sirikata::Protocol::Object::ObjectMessage* msg) {
    // Either we know the object and deliver, or somethings gone wacky
    HostedObjectPtr obj = getHostedObject(sporef_internalID);
    if (obj) {
        obj->receiveMessage(space, msg);
    }
    else {
        OH_LOG(warn, "Got message for " << sporef_internalID << " but no such object exists.");
        delete msg;
    }
}

// Primary HostedObject API

bool ObjectHost::connect(
    HostedObjectPtr ho,
    const SpaceObjectReference& sporef, const SpaceID& space,
    const TimedMotionVector3f& loc,
    const TimedMotionQuaternion& orient,
    const BoundingSphere3f& bnds,
    const String& mesh,
    const String& phy,
    const String& query,
    const String& zernike,
    ConnectedCallback connected_cb,
    MigratedCallback migrated_cb,
    StreamCreatedCallback stream_created_cb,
    DisconnectedCallback disconnected_cb
)
{
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    if (mHostedObjects.find(sporef)!=mHostedObjects.end())
        return false;
    SessionManager *sm = mSessionManagers[space];

    String filtered_query = mQueryProcessor->connectRequest(ho, sporef, query);
    return sm->connect(
        sporef, loc, orient, bnds, mesh, phy, filtered_query, zernike,
        std::tr1::bind(&ObjectHost::wrappedConnectedCallback, this, HostedObjectWPtr(ho), _1, _2, _3, connected_cb),
        migrated_cb,
        std::tr1::bind(&ObjectHost::wrappedStreamCreatedCallback, this, HostedObjectWPtr(ho), _1, _2, stream_created_cb),
        std::tr1::bind(&ObjectHost::wrappedDisconnectedCallback, this, HostedObjectWPtr(ho), _1, _2, disconnected_cb)
    );
}

void ObjectHost::wrappedConnectedCallback(HostedObjectWPtr ho_weak, const SpaceID& space, const ObjectReference& obj, const SessionManager::ConnectionInfo& ci, ConnectedCallback cb) {
    if (mQueryProcessor != NULL) {
        HostedObjectPtr ho(ho_weak);
        if (ho)
            mQueryProcessor->presenceConnected(ho, SpaceObjectReference(space, obj));
    }

    ConnectionInfo info;
    info.server = ci.server;
    info.loc = ci.loc;
    info.orient = ci.orient;
    info.bnds = ci.bounds;
    info.mesh = ci.mesh;
    info.physics = ci.physics;
    info.query = ci.query;
    cb(space, obj, info);
}

void ObjectHost::wrappedStreamCreatedCallback(HostedObjectWPtr ho_weak, const SpaceObjectReference& sporef, SessionManager::ConnectionEvent after, StreamCreatedCallback cb) {
    if (mQueryProcessor != NULL) {
        HostedObjectPtr ho(ho_weak);
        if (ho) {
            SSTStreamPtr strm = getSpaceStream(sporef.space(), sporef.object());
            // This had better be OK here since we're just getting the callback
            assert(strm);
            mQueryProcessor->presenceConnectedStream(ho, sporef, strm);
        }
    }
    cb(sporef, after);
}

void ObjectHost::wrappedDisconnectedCallback(HostedObjectWPtr ho_weak, const SpaceObjectReference& sporef, Disconnect::Code cause, DisconnectedCallback cb) {
    if (mQueryProcessor != NULL) {
        HostedObjectPtr ho(ho_weak);
        if (ho)
            mQueryProcessor->presenceDisconnected(ho, sporef);
    }
    cb(sporef, cause);
}

Duration ObjectHost::serverTimeOffset(const SpaceID& space) const {
    assert(mSessionManagers.find(space) != mSessionManagers.end());
    return mSessionManagers.find(space)->second->serverTimeOffset();
}

Duration ObjectHost::clientTimeOffset(const SpaceID& space) const {
    assert(mSessionManagers.find(space) != mSessionManagers.end());
    return mSessionManagers.find(space)->second->clientTimeOffset();
}

Time ObjectHost::spaceTime(const SpaceID& space, const Time& t) {
    Duration off = serverTimeOffset(space);
    // FIXME we should probably return a negative time and force the code using
    // this (e.g. the loc update stuff) to make sure it handles it correctly by
    // extrapolating to a current time.
    // This is kinda gross, but we need to make sure result >= 0
    if ( (int64)t.raw() + off.toMicro() < 0) return Time::null();
    return t + off;
}

Time ObjectHost::currentSpaceTime(const SpaceID& space) {
    return spaceTime(space, mContext->simTime());
}

Time ObjectHost::localTime(const SpaceID& space, const Time& t) {
    Duration off = clientTimeOffset(space);
    // FIXME we should probably return a negative time and force the code using
    // this (e.g. the loc update stuff) to make sure it handles it correctly by
    // extrapolating to a current time.
    // This is kinda gross, but we need to make sure result >= 0
    if ( (int64)t.raw() + off.toMicro() < 0) return Time::null();
    return t + off;
}

Time ObjectHost::currentLocalTime() {
    return mContext->simTime();
}


bool ObjectHost::send(SpaceObjectReference& sporef_src, const SpaceID& space, const ObjectMessagePort src_port, const UUID& dest, const ObjectMessagePort dest_port, MemoryReference payload) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);

    std::string payload_str( (char*)payload.begin(), (char*)payload.end() );
    return send(sporef_src, space, src_port, dest, dest_port, payload_str);
}

bool ObjectHost::send(SpaceObjectReference& sporef_src, const SpaceID& space, const ObjectMessagePort src_port, const UUID& dest, const ObjectMessagePort dest_port, const std::string& payload) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    return mSessionManagers[space]->send(sporef_src, src_port, dest, dest_port, payload);
}

void ObjectHost::registerHostedObject(const SpaceObjectReference &sporef_uuid, const HostedObjectPtr& obj)
{
    HostedObjectMap::iterator iter = mHostedObjects.find(sporef_uuid);
    if (iter != mHostedObjects.end()) {
        SILOG(oh,error,"Two objects having the same internal name in the mHostedObjects map on connect"<<sporef_uuid.toString());
    }
    mHostedObjects[sporef_uuid]=obj;
}
void ObjectHost::unregisterHostedObject(const SpaceObjectReference& sporef_uuid, HostedObject* key_obj)
{
    HostedObjectMap::iterator iter = mHostedObjects.find(sporef_uuid);
    if (iter != mHostedObjects.end()) {
        HostedObjectPtr obj (iter->second);
        if (obj.get()==key_obj)
            mHostedObjects.erase(iter);
        else
            SILOG(oh,error,"Two objects having the same internal name in the mHostedObjects map on disconnect "<<sporef_uuid.toString());
    }
}

void ObjectHost::hostedObjectDestroyed(const UUID& objid) {
    // Remove our weak reference to the object
    assert(mHostedObjectsByID.find(objid) != mHostedObjectsByID.end());
    mHostedObjectsByID.erase(objid);

    // This may not always be the best policy, but for now, if we run out of
    // all objects then its safe to try to shutdown. Without a remote admin
    // interface or something, its going to be impossible for any *new* work
    // to get started (although some remaining work may finish out).  We
    // actually check for no presences and no HostedObjects. The former should
    // always be zero if the latter is.
    mActiveHostedObjects--;
    if (mHostedObjects.empty() && mActiveHostedObjects == 0 && !mContext->stopped())
        mContext->mainStrand->post(std::tr1::bind(&Context::shutdown, mContext), "Shutdown after last object destroyed");
}


HostedObjectPtr ObjectHost::getHostedObject(const SpaceObjectReference& sporef) const {
    HostedObjectMap::const_iterator iter = mHostedObjects.find(sporef);
    if (iter != mHostedObjects.end()) {
        return iter->second;
    }
    return HostedObjectPtr();
}

HostedObjectPtr ObjectHost::getHostedObject(const UUID& internal_id) const {
    InternalIDHostedObjectMap::const_iterator iter = mHostedObjectsByID.find(internal_id);
    if (iter == mHostedObjectsByID.end()) return HostedObjectPtr();
    HostedObjectPtr ho = iter->second.lock();
    return ho;
}


ObjectHost::SSTStreamPtr ObjectHost::getSpaceStream(const SpaceID& space, const ObjectReference& oref)
{
    return mSessionManagers[space]->getSpaceStream(oref);
}


void ObjectHost::start()
{
}

void ObjectHost::stop() {
    for(SpaceSessionManagerMap::iterator it = mSessionManagers.begin(); it != mSessionManagers.end(); it++) {
        SessionManager* sm = it->second;
        sm->stop();
    }

    for(InternalIDHostedObjectMap::const_iterator it = mHostedObjectsByID.begin(); it != mHostedObjectsByID.end(); it++) {
        HostedObjectPtr ho = it->second.lock();
        if (ho) ho->stop();
    }
}

OHDP::Port* ObjectHost::bindOHDPPort(const SpaceID& space, const OHDP::NodeID& node, OHDP::PortID port) {
    SpaceSessionManagerMap::iterator iter = mSessionManagers.find(space);
    if (iter == mSessionManagers.end())
        return NULL;
    return iter->second->bindOHDPPort(space, node, port);
}

OHDP::Port* ObjectHost::bindOHDPPort(const SpaceID& space, const OHDP::NodeID& node) {
    SpaceSessionManagerMap::iterator iter = mSessionManagers.find(space);
    if (iter == mSessionManagers.end())
        return NULL;
    return iter->second->bindOHDPPort(space, node);
}

OHDP::PortID ObjectHost::unusedOHDPPort(const SpaceID& space, const OHDP::NodeID& node) {
    SpaceSessionManagerMap::iterator iter = mSessionManagers.find(space);
    if (iter == mSessionManagers.end())
        return OHDP::PortID::null();
    return iter->second->unusedOHDPPort(space, node);
}

void ObjectHost::registerDefaultOHDPHandler(const OHDP::MessageHandler& cb) {
    mDefaultOHDPMessageHandler = cb;
}

void ObjectHost::handleDefaultOHDPMessageHandler(const OHDP::Endpoint& src, const OHDP::Endpoint& dst, MemoryReference payload) {
    if (mDefaultOHDPMessageHandler)
        mDefaultOHDPMessageHandler(src, dst, payload);
}

ObjectScriptManager* ObjectHost::getScriptManager(const String& id) {
    ScriptManagerMap::iterator it = mScriptManagers.find(id);
    if (it == mScriptManagers.end())
        return NULL;
    return it->second;
}

String ObjectHost::getSimOptions(const String&simName){
    std::string nam=simName;
    std::map<std::string,std::string>::iterator where=mSimOptions.find(nam);
    if (where==mSimOptions.end()) return String();
    std::string retval=where->second;
    return String(retval);
}



void ObjectHost::commandListObjects(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    // Make sure we return the objects key set even if there are none
    result.put( String("objects"), Command::Array());
    Command::Array& objects_ary = result.getArray("objects");

    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);

    for(InternalIDHostedObjectMap::const_iterator it = mHostedObjectsByID.begin(); it != mHostedObjectsByID.end(); it++) {
        HostedObjectPtr ho = it->second.lock();
        if (ho) objects_ary.push_back( ho->id().toString() );
    }
    cmdr->result(cmdid, result);
}

void ObjectHost::commandCreateObject(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    if (!cmd.contains("script.type"))
    {
        result.put("error", "Must specify at least script.type");
        cmdr->result(cmdid, result);
        return;
    }

    String scriptType = cmd.getString("script.type");
    String scriptOpts = cmd.getString("script.opts", "");
    String scriptContents = cmd.getString("script.contents", "");

    HostedObjectPtr obj;
    obj = createObject(scriptType, scriptOpts, scriptContents);

    result.put("id", obj->id().toString());
    cmdr->result(cmdid, result);
}

HostedObjectPtr ObjectHost::getCommandObject(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    String obj_string = cmd.getString("object", "");
    UUID objid(obj_string, UUID::HumanReadable());
    if (objid == UUID::null()) { // not specified, not parsed
        Command::Result result = Command::EmptyResult();
        result.put("error", "Ill-formatted request: no object specified for disconnect.");
        cmdr->result(cmdid, result);
        return HostedObjectPtr();
    }

    HostedObjectPtr ho = getHostedObject(objid);
    if (!ho) {
        Command::Result result = Command::EmptyResult();
        result.put("error", "Object not found");
        cmdr->result(cmdid, result);
        return HostedObjectPtr();
    }

    return ho;
}

void ObjectHost::commandDestroyObject(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    HostedObjectPtr ho = getCommandObject(cmd, cmdr, cmdid);
    if (!ho) return;

    Command::Result result = Command::EmptyResult();
    ho->stop();
    ho->destroy();
    result.put("success", true);
    cmdr->result(cmdid, result);
}

void ObjectHost::commandObjectPresences(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    HostedObjectPtr ho = getCommandObject(cmd, cmdr, cmdid);
    if (!ho) return;

    ho->commandPresences(cmd, cmdr, cmdid);
}

} // namespace Sirikata
