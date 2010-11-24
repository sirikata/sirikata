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

#include <sirikata/core/service/Context.hpp>

#define OH_LOG(lvl,msg) SILOG(oh,lvl,"[OH] " << msg)

namespace Sirikata {

ObjectHost::ObjectHost(ObjectHostContext* ctx, SpaceIDMap *spaceMap, Network::IOService *ioServ, const String&options)
 : mContext(ctx)
{
    mScriptPlugins=new PluginManager;
    mSpaceIDMap = spaceMap;
    OptionValue *protocolOptions;
    InitializeClassOptions ico("objecthost",this,
                           protocolOptions=new OptionValue("protocols","",OptionValueType<std::map<std::string,std::string> >(),"passes options into protocol specific libraries like \"tcpsst:{--send-buffer-size=1440 --parallel-sockets=1},udp:{--send-buffer-size=1500}\""),
                           NULL);
    {
        std::map<std::string,std::string> *options=&protocolOptions->as<std::map<std::string,std::string> > ();
        for (std::map<std::string,std::string>::iterator i=options->begin(),ie=options->end();i!=ie;++i) {
            mSpaceConnectionProtocolOptions[i->first]=Network::StreamFactory::getSingleton().getOptionParser(i->first)(i->second);
        }
    }

}

ObjectHost::~ObjectHost() {
    {
        HostedObjectMap objs;
        mHostedObjects.swap(objs);
        for (HostedObjectMap::iterator iter = mHostedObjects.begin();
                 iter != mHostedObjects.end();
                 ++iter) {
            iter->second->destroy();
        }
        objs.clear(); // The HostedObject destructor will attempt to delete from mHostedObjects
    }
    delete mScriptPlugins;
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

void ObjectHost::handleObjectConnected(const UUID& objid, ServerID server) {
    // ignored
}

void ObjectHost::handleObjectMigrated(const UUID& objid, ServerID from, ServerID to) {
    // ignored
}

void ObjectHost::handleObjectMessage(const UUID& internalID, const SpaceID& space, Sirikata::Protocol::Object::ObjectMessage* msg) {
    // Either we know the object and deliver, or somethings gone wacky
    HostedObjectPtr obj = getHostedObject(internalID);
    if (obj) {
        obj->receiveMessage(space, msg);
    }
    else {
        OH_LOG(warn, "Got message for " << internalID.toString() << " but no such object exists.");
        delete msg;
    }
}

void ObjectHost::handleObjectDisconnected(const UUID& internalID, Disconnect::Code) {
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
    HostedObjectPtr obj, const SpaceID& space,
    const TimedMotionVector3f& loc,
    const TimedMotionQuaternion& orient,
    const BoundingSphere3f& bnds,
    const String& mesh,
    const SolidAngle& init_sa,
    ConnectedCallback connected_cb,
    MigratedCallback migrated_cb, StreamCreatedCallback stream_created_cb,
    DisconnectedCallback disconnected_cb
)
{
    bool with_query = init_sa != SolidAngle::Max;
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    mSessionManagers[space]->connect(
        obj->getUUID(), loc, orient, bnds, with_query, init_sa, mesh,
        std::tr1::bind(&ObjectHost::wrappedConnectedCallback, this, _1, _2, _3, _4, _5, _6, _7, connected_cb),
        migrated_cb,
        stream_created_cb,
        disconnected_cb
    );
}

void ObjectHost::wrappedConnectedCallback(const SpaceID& space, const ObjectReference& obj, ServerID server, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& mesh, ConnectedCallback cb) {
    ConnectionInfo info;
    info.server = server;
    info.loc = loc;
    info.orient = orient;
    info.bnds = bnds;
    info.mesh = mesh;
    cb(space, obj, info);
}

void ObjectHost::disconnect(HostedObjectPtr obj, const SpaceID& space) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    mSessionManagers[space]->disconnect(obj->getUUID());
}

Duration ObjectHost::serverTimeOffset(const SpaceID& space) const {
    assert(mSessionManagers.find(space) != mSessionManagers.end());
    return mSessionManagers.find(space)->second->serverTimeOffset();
}

Duration ObjectHost::clientTimeOffset(const SpaceID& space) const {
    assert(mSessionManagers.find(space) != mSessionManagers.end());
    return mSessionManagers.find(space)->second->clientTimeOffset();
}

bool ObjectHost::send(HostedObjectPtr obj, const SpaceID& space, const uint16 src_port, const UUID& dest, const uint16 dest_port, MemoryReference payload) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);

    std::string payload_str( (char*)payload.begin(), (char*)payload.end() );
    return send(obj, space, src_port, dest, dest_port, payload_str);
}

bool ObjectHost::send(HostedObjectPtr obj, const SpaceID& space, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    Sirikata::SerializationCheck::Scoped sc(&mSessionSerialization);
    return mSessionManagers[space]->send(obj->getUUID(), src_port, dest, dest_port, payload);
}

void ObjectHost::registerHostedObject(const HostedObjectPtr &obj) {
    mHostedObjects.insert(HostedObjectMap::value_type(obj->getUUID(), obj));
}
void ObjectHost::unregisterHostedObject(const UUID &objID) {
    HostedObjectMap::iterator iter = mHostedObjects.find(objID);
    if (iter != mHostedObjects.end()) {
        HostedObjectPtr obj (iter->second);
        mHostedObjects.erase(iter);
    }
}
HostedObjectPtr ObjectHost::getHostedObject(const UUID &id) const {
    HostedObjectMap::const_iterator iter = mHostedObjects.find(id);
    if (iter != mHostedObjects.end()) {
//        HostedObjectPtr obj(iter->second.lock());
//        return obj;
        return iter->second;
    }
    return HostedObjectPtr();
}

ObjectHost::SSTStreamPtr ObjectHost::getSpaceStream(const SpaceID& space, const UUID& objectID) {
    return mSessionManagers[space]->getSpaceStream(ObjectReference(objectID));
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

ProxyManager *ObjectHost::getProxyManager(const SpaceID&space) const
{
    DEPRECATED();
    NOT_IMPLEMENTED(oh);
    return NULL;
}


void ObjectHost::persistEntityState( const String& filename)
{
    std::ofstream fp(filename.c_str());


    fp << "\"objtype\",\"subtype\",\"name\",\"pos_x\",\"pos_y\",\"pos_z\",\"orient_x\",\"orient_y\",\"orient_z\",\"orient_w\",\"vel_x\",\"vel_y\",\"vel_z\",\"rot_axis_x\",\"rot_axis_y\",\"rot_axis_z\",\"rot_speed\",\"meshURI\",\"scale\",\"objid\",\"script_type\",\"script_options\"" << std::endl;



    HostedObjectMap::iterator it = mHostedObjects.begin();
    for( ; it != mHostedObjects.end(); it++)
    {
        HostedObjectPtr objPtr = (*it).second;
        objPtr->persistToFile(fp);
    }
}





} // namespace Sirikata
