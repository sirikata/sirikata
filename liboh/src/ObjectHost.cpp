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
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/proxyobject/ConnectionEventListener.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/service/Context.hpp>

#define OH_LOG(lvl,msg) SILOG(oh,lvl,msg)

namespace Sirikata {

ObjectHost::ObjectHost(ObjectHostContext* ctx, Network::IOService *ioServ, const String&options)
 : mContext(ctx),
   mStorage(NULL),
   mPersistentSet(NULL)
{
    mScriptPlugins=new PluginManager;
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
            ObjectScriptManager* newmgr = ObjectScriptManagerFactory::getSingleton().getConstructor(i->first)(i->second);
            if (newmgr)
                mScriptManagers[i->first] = newmgr;
        }
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
    delete mScriptPlugins;
}

HostedObjectPtr ObjectHost::createObject(const String& script_type, const String& script_opts, const String& script_contents) {
    // FIXME we could ensure that the random ID isn't actually in use already
    return createObject(UUID::random(), script_type, script_opts, script_contents);
}

HostedObjectPtr ObjectHost::createObject(const UUID &uuid, const String& script_type, const String& script_opts, const String& script_contents) {
    HostedObjectPtr ho = HostedObject::construct<HostedObject>(mContext, this, uuid);
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
    mSessionManagers[space_id] = smgr;
    smgr->start();
}

void ObjectHost::handleObjectConnected(const SpaceObjectReference& sporef_objid, ServerID server) {
    // ignored
}

void ObjectHost::handleObjectMigrated(const SpaceObjectReference& sporef_objid, ServerID from, ServerID to) {
    // ignored
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

void ObjectHost::handleObjectDisconnected(const SpaceObjectReference& sporef_internalID, Disconnect::Code) {
    // ignored
}

//This function just returns the first space id in the unordered map
//associated with mSessionManagers.
SpaceID ObjectHost::getDefaultSpace()
{
    if (mSessionManagers.size() == 0)
    {
        std::cout<<"\n\nERROR: no record of space in object host\n\n";
        assert(false);
    }

    return mSessionManagers.begin()->first;
}

// Primary HostedObject API

void ObjectHost::connect(
    const SpaceObjectReference& sporef, const SpaceID& space,
    const TimedMotionVector3f& loc,
    const TimedMotionQuaternion& orient,
    const BoundingSphere3f& bnds,
    const String& mesh,
    const String& phy,
    const SolidAngle& init_sa,
    ConnectedCallback connected_cb,
    MigratedCallback migrated_cb,
    StreamCreatedCallback stream_created_cb,
    DisconnectedCallback disconnected_cb
)
{
    bool with_query = init_sa != SolidAngle::Max;

    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);

    mSessionManagers[space]->connect(
        sporef, loc, orient, bnds, with_query, init_sa, mesh, phy,
        std::tr1::bind(&ObjectHost::wrappedConnectedCallback, this, _1, _2, _3, connected_cb),
        migrated_cb,
        stream_created_cb,
        disconnected_cb
    );
}

  void ObjectHost::wrappedConnectedCallback(const SpaceID& space, const ObjectReference& obj, const SessionManager::ConnectionInfo& ci, ConnectedCallback cb) {
    ConnectionInfo info;
    info.server = ci.server;
    info.loc = ci.loc;
    info.orient = ci.orient;
    info.bnds = ci.bounds;
    info.mesh = ci.mesh;
    info.physics = ci.physics;
    info.queryAngle   = ci.queryAngle;
    cb(space, obj, info);
}

void ObjectHost::disconnect(SpaceObjectReference& sporef, const SpaceID& space) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    mSessionManagers[space]->disconnect(sporef);
}

Duration ObjectHost::serverTimeOffset(const SpaceID& space) const {
    assert(mSessionManagers.find(space) != mSessionManagers.end());
    return mSessionManagers.find(space)->second->serverTimeOffset();
}

Duration ObjectHost::clientTimeOffset(const SpaceID& space) const {
    assert(mSessionManagers.find(space) != mSessionManagers.end());
    return mSessionManagers.find(space)->second->clientTimeOffset();
}

bool ObjectHost::send(SpaceObjectReference& sporef_src, const SpaceID& space, const uint16 src_port, const UUID& dest, const uint16 dest_port, MemoryReference payload) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);

    std::string payload_str( (char*)payload.begin(), (char*)payload.end() );
    return send(sporef_src, space, src_port, dest, dest_port, payload_str);
}

bool ObjectHost::send(SpaceObjectReference& sporef_src, const SpaceID& space, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    return mSessionManagers[space]->send(sporef_src, src_port, dest, dest_port, payload);
}

void ObjectHost::registerHostedObject(const SpaceObjectReference &sporef_uuid, const HostedObjectPtr& obj)
{
    mHostedObjects.insert(HostedObjectMap::value_type(sporef_uuid, obj));
}
void ObjectHost::unregisterHostedObject(const SpaceObjectReference& sporef_uuid)
{
    HostedObjectMap::iterator iter = mHostedObjects.find(sporef_uuid);
    if (iter != mHostedObjects.end()) {
        HostedObjectPtr obj (iter->second);
        mHostedObjects.erase(iter);
    }
}


HostedObjectPtr ObjectHost::getHostedObject(const SpaceObjectReference& sporef) const {
    HostedObjectMap::const_iterator iter = mHostedObjects.find(sporef);
    if (iter != mHostedObjects.end()) {
        return iter->second;
    }
    return HostedObjectPtr();
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
}

ObjectScriptManager* ObjectHost::getScriptManager(const String& id) {
    ScriptManagerMap::iterator it = mScriptManagers.find(id);
    if (it == mScriptManagers.end())
        return NULL;
    return it->second;
}

ProxyManager *ObjectHost::getProxyManager(const SpaceID&space) const
{
    DEPRECATED();
    NOT_IMPLEMENTED(oh);
    return NULL;
}
String ObjectHost::getSimOptions(const String&simName){
    std::string nam=simName;
    std::map<std::string,std::string>::iterator where=mSimOptions.find(nam);
    if (where==mSimOptions.end()) return String();
    std::string retval=where->second;
    return String(retval);
}
} // namespace Sirikata
