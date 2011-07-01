/*  Sirikata liboh -- Object Host
 *  HostedObject.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>

#include <sirikata/core/util/ThreadId.hpp>
#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/core/odp/Exceptions.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>
#include <list>
#include <vector>
#include <sirikata/proxyobject/SimulationFactory.hpp>
#include <sirikata/core/network/Frame.hpp>
#include "PerPresenceData.hpp"
#include "Protocol_Frame.pbj.hpp"

#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"


#define HO_LOG(lvl,msg) SILOG(ho,lvl,msg);

namespace Sirikata {



HostedObject::HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID & _id)
 : mContext(ctx),
   mID(_id),
   mObjectHost(parent),
   mObjectScript(NULL),
   mPresenceData(new PresenceDataMap),
   mNextSubscriptionID(0),
   mOrphanLocUpdates(ctx, ctx->mainStrand, Duration::seconds(10))
{
    mContext->add(&mOrphanLocUpdates);

    mDelegateODPService = new ODP::DelegateService(
        std::tr1::bind(
            &HostedObject::createDelegateODPPort, this,
            _1, _2, _3
        )
    );
}


//Need to define this function so that can register timeouts in jscript
Network::IOService* HostedObject::getIOService()
{
    return mContext->ioService;
}


TimeSteppedSimulation* HostedObject::runSimulation(const SpaceObjectReference& sporef, const String& simName)
{
    TimeSteppedSimulation* sim = NULL;

    PresenceDataMap::iterator psd_it = mPresenceData->find(sporef);
    assert (psd_it != mPresenceData->end());

    PerPresenceData& pd =  psd_it->second;
    addSimListeners(pd,simName,sim);

    if (sim != NULL)
    {
        HO_LOG(detailed, "Adding simulation to context");
        mContext->add(sim);
    }
    return sim;
}


HostedObject::~HostedObject() {
    destroy();

    if (mPresenceData != NULL)
        delete mPresenceData;
}

const UUID& HostedObject::id() const {
    return mID;
}

void HostedObject::destroy()
{
    if (mObjectScript) {
        delete mObjectScript;
        mObjectScript=NULL;
    }

    for (PresenceDataMap::iterator iter = mPresenceData->begin(); iter != mPresenceData->end(); ++iter)
        mObjectHost->unregisterHostedObject(iter->first);

    mPresenceData->clear();
}

Time HostedObject::spaceTime(const SpaceID& space, const Time& t) {
    return t + mObjectHost->serverTimeOffset(space);
}

Time HostedObject::currentSpaceTime(const SpaceID& space) {
    return spaceTime(space, mContext->simTime());
}

Time HostedObject::localTime(const SpaceID& space, const Time& t) {
    return t + mObjectHost->clientTimeOffset(space);
}

Time HostedObject::currentLocalTime() {
    return mContext->simTime();
}


ProxyManagerPtr HostedObject::getProxyManager(const SpaceID& space, const ObjectReference& oref)
{
    SpaceObjectReference toFind(space,oref);
    PresenceDataMap::const_iterator it = mPresenceData->find(toFind);
    if (it == mPresenceData->end())
    {
        ProxyManagerPtr returner;
        return returner;

    }
    return it->second.proxyManager;
}


//returns all the spaceobjectreferences associated with the presence with id sporef
void HostedObject::getProxySpaceObjRefs(const SpaceObjectReference& sporef,SpaceObjRefVec& ss) const
{
    PresenceDataMap::iterator smapIter = mPresenceData->find(sporef);

    if (smapIter != mPresenceData->end())
    {
        //means that we actually did have a connection with this sporef
        //load the proxy objects the sporef'd connection has actually seen into
        //ss.
        smapIter->second.proxyManager->getAllObjectReferences(ss);
    }
}



//returns all the spaceobjrefs associated with all presences of this object.
//They are returned in ss.
void HostedObject::getSpaceObjRefs(SpaceObjRefVec& ss) const
{
    // must be connected to at least one space
    assert(mPresenceData != NULL);

    PresenceDataMap::const_iterator smapIter;
    for (smapIter = mPresenceData->begin(); smapIter != mPresenceData->end(); ++smapIter)
        ss.push_back(SpaceObjectReference(smapIter->second.space,smapIter->second.object));
}



static ProxyObjectPtr nullPtr;
const ProxyObjectPtr &HostedObject::getProxyConst(const SpaceID &space, const ObjectReference& oref) const
{
    PresenceDataMap::const_iterator iter = mPresenceData->find(SpaceObjectReference(space,oref));
    if (iter == mPresenceData->end()) {
        return nullPtr;
    }
    return iter->second.mProxyObject;
}


//first checks to see if have a presence associated with spVisTo.  If do, then
//checks if have a proxy object associated with sporef, sets p to the associated
//proxy object, and returns true.  Otherwise, returns false.
bool HostedObject::getProxyObjectFrom(const SpaceObjectReference*   spVisTo, const SpaceObjectReference*   sporef, ProxyObjectPtr& p)
{
    ProxyManagerPtr ohpmp = getProxyManager(spVisTo->space(),spVisTo->object());
    if (ohpmp.get() == NULL)
        return false;

    p = ohpmp->getProxyObject(*sporef);

    if (p.get() == NULL)
        return false;

    return true;
}


static ProxyManagerPtr nullManPtr;
ProxyObjectPtr HostedObject::getProxy(const SpaceID& space, const ObjectReference& oref)
{
    ProxyManagerPtr proxy_manager = getProxyManager(space,oref);
    if (proxy_manager == nullManPtr)
    {
        SILOG(oh,info, "[HO] In getProxy of HostedObject, have no proxy manager associated with "<<space<<"-"<<oref);
        return nullPtr;
    }

    ProxyObjectPtr  proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));
    return proxy_obj;
}

bool HostedObject::getProxy(const SpaceObjectReference* sporef, ProxyObjectPtr& p)
{
    p = getProxy(sporef->space(), sporef->object());

    if (p.get() == NULL)
    {
        SILOG(oh,info, "[HO] In getProxy of HostedObject, have no proxy presence associated with "<<*sporef);
        return false;
    }

    return true;
}


namespace {
bool myisalphanum(char c) {
    if (c>='a'&&c<='z') return true;
    if (c>='A'&&c<='Z') return true;
    if (c>='0'&&c<='9') return true;
    return false;
}
}


void HostedObject::initializeScript(const String& script_type, const String& args, const String& script)
{
    if (mObjectScript) {
        SILOG(oh,warn,"[HO] Ignored initializeScript because script already exists for object");
        return;
    }

    SILOG(oh,detailed,"[HO] Creating a script object for object");

    static ThreadIdCheck scriptId=ThreadId::registerThreadGroup(NULL);
    assertThreadGroup(scriptId);
    if (!ObjectScriptManagerFactory::getSingleton().hasConstructor(script_type)) {
        bool passed=true;
        for (std::string::const_iterator i=script_type.begin(),ie=script_type.end();i!=ie;++i) {
            if (!myisalphanum(*i)) {
                if (*i!='-'&&*i!='_') {
                    passed=false;
                }
            }
        }
        if (passed) {
            mObjectHost->getScriptPluginManager()->load(script_type);
        }
        else
        {
            SILOG(oh,debug,"[HO] Failed to create script for object because incorrect script type");
        }
    }
    ObjectScriptManager *mgr = mObjectHost->getScriptManager(script_type);
    if (mgr) {
        SILOG(oh,insane,"[HO] Creating script for object with args of "<<args);
        mObjectScript = mgr->createObjectScript(this->getSharedPtr(), args, script);
        mObjectScript->scriptTypeIs(script_type);
        mObjectScript->scriptOptionsIs(args);
    }
}

void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& phy,
        const UUID&object_uuid_evidence,
        PresenceToken token)
{
    connect(spaceID, startingLocation, meshBounds, mesh, phy, SolidAngle::Max, object_uuid_evidence,ObjectReference::null(),token);
}




void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& phy,
        const SolidAngle& queryAngle,
        const UUID&object_uuid_evidence,
        const ObjectReference& orefID,
        PresenceToken token)
{
    if (spaceID == SpaceID::null())
        return;

    ObjectReference oref = (orefID == ObjectReference::null()) ? ObjectReference(UUID::random()) : orefID;

    SpaceObjectReference connectingSporef (spaceID,oref);
    mObjectHost->registerHostedObject(connectingSporef,getSharedPtr());


    // Note: we always use Time::null() here.  The server will fill in the
    // appropriate value.  When we get the callback, we can fix this up.
    Time approx_server_time = Time::null();
    mObjectHost->connect(
        connectingSporef, spaceID,
        TimedMotionVector3f(approx_server_time, MotionVector3f( Vector3f(startingLocation.getPosition()), startingLocation.getVelocity()) ),
        TimedMotionQuaternion(approx_server_time,MotionQuaternion(startingLocation.getOrientation().normal(),Quaternion(startingLocation.getAxisOfRotation(),startingLocation.getAngularSpeed()))),  //normalize orientations
        meshBounds,
        mesh,
        phy,
        queryAngle,
        std::tr1::bind(&HostedObject::handleConnected, this, _1, _2, _3),
        std::tr1::bind(&HostedObject::handleMigrated, this, _1, _2, _3),
        std::tr1::bind(&HostedObject::handleStreamCreated, this, _1, token),
        std::tr1::bind(&HostedObject::handleDisconnected, this, _1, _2)
    );
}




void HostedObject::addSimListeners(PerPresenceData& pd, const String& simName,TimeSteppedSimulation*& sim)
{
    if (pd.sims.find(simName) != pd.sims.end()) {
        sim = pd.sims[simName];
        return;
    }

    HO_LOG(info,String("[OH] Initializing ") + simName);
    sim = SimulationFactory::getSingleton().getConstructor ( simName ) ( mContext, getSharedPtr(), pd.id(), getObjectHost()->getSimOptions(simName));
    if (!sim)
    {
        HO_LOG(error,String("Unable to load ") + simName + String(" plugin. The PATH environment variable is ignored, so make sure you have copied the DLLs from dependencies/ogre/bin/ into the current directory. Sorry about this!"));
        std::cerr << "Press enter to continue" << std::endl;
        fgetc(stdin);
        exit(0);
    }
    else
    {
        pd.sims[simName] = sim;
        mObjectHost->addListener(sim);
        HO_LOG(info,String("Successfully initialized ") + simName);
    }
}



void HostedObject::handleConnected(const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info)
{
    // FIXME this never gets cleaned out on disconnect
    mSSTDatagramLayers.push_back(
        BaseDatagramLayerType::createDatagramLayer(
            SpaceObjectReference(space, obj),
            mContext, mDelegateODPService
        )
    );

    // We have to manually do what mContext->mainStrand->wrap( ... ) should be
    // doing because it can't handle > 5 arguments.
    mContext->mainStrand->post(
        std::tr1::bind(&HostedObject::handleConnectedIndirect, this, space, obj, info)
    );



}


void HostedObject::handleConnectedIndirect(const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info)
{
    if (info.server == NullServerID)
    {
        HO_LOG(warning,"Failed to connect object:" << obj << " to space " << space);
        return;
    }

    SpaceObjectReference self_objref(space, obj);
    if(mPresenceData->find(self_objref) == mPresenceData->end())
    {
        mPresenceData->insert(
            PresenceDataMap::value_type(self_objref,PerPresenceData(this,space,obj,info.queryAngle))
        );
    }

    // Convert back to local time
    TimedMotionVector3f local_loc(localTime(space, info.loc.updateTime()), info.loc.value());
    TimedMotionQuaternion local_orient(localTime(space, info.orient.updateTime()), info.orient.value());
    ProxyObjectPtr self_proxy = createProxy(self_objref, self_objref, Transfer::URI(info.mesh), local_loc, local_orient, info.bnds, info.physics,info.queryAngle);

    // Use to initialize PerSpaceData
    PresenceDataMap::iterator psd_it = mPresenceData->find(self_objref);
    PerPresenceData& psd = psd_it->second;
    initializePerPresenceData(psd, self_proxy);


    //bind an odp port to listen for the begin scripting signal.  if have
    //receive the scripting signal for the first time, that means that we create
    //a JSObjectScript for this hostedobject
    bindODPPort(space,obj,Services::LISTEN_FOR_SCRIPT_BEGIN);
    HO_LOG(detailed,"Connected object " << obj << " to space " << space << " waiting on notice");

}

void HostedObject::handleMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server)
{
    NOT_IMPLEMENTED(ho);
}



void HostedObject::handleStreamCreated(const SpaceObjectReference& spaceobj, PresenceToken token) {
    HO_LOG(detailed,"Handling new SST stream from space server for " << spaceobj);
    SSTStreamPtr sstStream = mObjectHost->getSpaceStream(spaceobj.space(), spaceobj.object());

    if (sstStream != SSTStreamPtr() ) {
        sstStream->listenSubstream(OBJECT_PORT_LOCATION,
            std::tr1::bind(&HostedObject::handleLocationSubstream, this, spaceobj, _1, _2)
        );
        sstStream->listenSubstream(OBJECT_PORT_PROXIMITY,
            std::tr1::bind(&HostedObject::handleProximitySubstream, this, spaceobj, _1, _2)
        );
    }
    HO_LOG(detailed,"Notifying of connected object " << spaceobj.object() << " to space " << spaceobj.space());
    notify(&SessionEventListener::onConnected, getSharedPtr(), spaceobj, token);
}




void HostedObject::initializePerPresenceData(PerPresenceData& psd, ProxyObjectPtr selfproxy) {
    psd.initializeAs(selfproxy);
}

void HostedObject::disconnectFromSpace(const SpaceID &spaceID, const ObjectReference& oref)
{
    PresenceDataMap::iterator where;
    where=mPresenceData->find(SpaceObjectReference(spaceID, oref));
    if (where!=mPresenceData->end()) {
        mPresenceData->erase(where);
        //need to actually send a disconnection request to the space;
        mObjectHost->disconnectObject(spaceID,oref);
    } else {
        SILOG(cppoh,error,"Attempting to disconnect from space "<<spaceID<<" and object: "<< oref<<" when not connected to it...");
    }
}

void HostedObject::handleDisconnected(const SpaceObjectReference& spaceobj, Disconnect::Code cc) {
    notify(&SessionEventListener::onDisconnected, getSharedPtr(), spaceobj);
    disconnectFromSpace(spaceobj.space(), spaceobj.object());
}


void HostedObject::receiveMessage(const SpaceID& space, const Protocol::Object::ObjectMessage* msg) {
    // Convert to ODP runtime format
    ODP::Endpoint src_ep(space, ObjectReference(msg->source_object()), msg->source_port());
    ODP::Endpoint dst_ep(space, ObjectReference(msg->dest_object()), msg->dest_port());

    if (mDelegateODPService->deliver(src_ep, dst_ep, MemoryReference(msg->payload()))) {
        // if this was true, it got delivered
        delete msg;
    }
    else {
        SILOG(cppoh,detailed,"[HO] Undelivered message from " << src_ep << " to " << dst_ep);
        delete msg;
    }

}



void HostedObject::handleLocationSubstream(const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s) {
    s->registerReadCallback( std::tr1::bind(&HostedObject::handleLocationSubstreamRead, this, spaceobj, s, new std::stringstream(), _1, _2) );
}

void HostedObject::handleProximitySubstream(const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s) {
    std::stringstream** prevdataptr = new std::stringstream*;
    *prevdataptr = new std::stringstream();
    s->registerReadCallback( std::tr1::bind(&HostedObject::handleProximitySubstreamRead, this, spaceobj, s, prevdataptr, _1, _2) );
}

void HostedObject::handleLocationSubstreamRead(const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length) {
    prevdata->write((const char*)buffer, length);
    if (handleLocationMessage(spaceobj, prevdata->str())) {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to s
        s->registerReadCallback(0);
    }
}

void HostedObject::handleProximitySubstreamRead(const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream** prevdataptr, uint8* buffer, int length) {
    std::stringstream* prevdata = *prevdataptr;
    prevdata->write((const char*)buffer, length);

    while(true) {
        std::string msg = Network::Frame::parse(*prevdata);
        if (prevdata->eof()) {
            delete prevdata;
            prevdata = new std::stringstream();
            *prevdataptr = prevdata;
        }

        // If we don't have a full message, just wait for more
        if (msg.empty()) return;

        // Otherwise, try to handle it
        handleProximityMessage(spaceobj, msg);
    }

    // FIXME we should be getting a callback on stream close so we can clean up!
    //s->registerReadCallback(0);
}

void HostedObject::processLocationUpdate( const SpaceObjectReference& sporef,ProxyObjectPtr proxy_obj, const Sirikata::Protocol::Loc::LocationUpdate& update) {
    uint64 seqno = (update.has_seqno() ? update.seqno() : 0);

    TimedMotionVector3f loc;
    TimedMotionQuaternion orient;
    BoundingSphere3f bounds;
    String mesh;
    String phy;

    TimedMotionVector3f* locptr = NULL;
    TimedMotionQuaternion* orientptr = NULL;
    BoundingSphere3f* boundsptr = NULL;
    String* meshptr = NULL;
    String* phyptr = NULL;


    if (update.has_location()) {
        Sirikata::Protocol::TimedMotionVector update_loc = update.location();
        Time locTime = localTime(sporef.space(),update_loc.t());
        loc = TimedMotionVector3f(locTime, MotionVector3f(update_loc.position(), update_loc.velocity()));

        CONTEXT_OHTRACE(objectLoc,
            sporef.object().getAsUUID(),
            //getUUID(),
            update.object(),
            loc
        );

        locptr = &loc;
    }

    if (update.has_orientation()) {
        Sirikata::Protocol::TimedMotionQuaternion update_orient = update.orientation();
        orient = TimedMotionQuaternion(localTime(sporef.space(), update_orient.t()), MotionQuaternion(update_orient.position(), update_orient.velocity()));
        orientptr = &orient;
    }

    if (update.has_bounds()) {
        bounds = update.bounds();
        boundsptr = &bounds;
    }

    if (update.has_mesh()) {
        mesh = update.mesh();
        meshptr = &mesh;
    }

    if (update.has_physics()) {
        phy = update.physics();
        phyptr = &phy;
    }

    processLocationUpdate(sporef.space(), proxy_obj, seqno, false, locptr, orientptr, boundsptr, meshptr, phyptr);
}

void HostedObject::processLocationUpdate(const SpaceID& space, ProxyObjectPtr proxy_obj, uint64 seqno, bool predictive, TimedMotionVector3f* loc, TimedMotionQuaternion* orient, BoundingSphere3f* bounds, String* mesh, String* phy) {

    if (loc)
        proxy_obj->setLocation(*loc, seqno);

    if (orient)
        proxy_obj->setOrientation(*orient, seqno);

    if (bounds)
        proxy_obj->setBounds(*bounds, seqno);

    if (mesh)
        proxy_obj->setMesh(Transfer::URI(*mesh), seqno);

    if (phy && *phy != "")
        proxy_obj->setPhysics(*phy, seqno);
}

bool HostedObject::handleLocationMessage(const SpaceObjectReference& spaceobj, const std::string& payload) {
    Sirikata::Protocol::Frame frame;
    bool parse_success = frame.ParseFromString(payload);
    if (!parse_success) return false;
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    contents.ParseFromString(frame.payload());

    SpaceID space = spaceobj.space();
    ObjectReference oref = spaceobj.object();
    ProxyManagerPtr proxy_manager = getProxyManager(space, oref);

    if (!proxy_manager)
    {
        return true;
    }

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

        ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(), spaceobj.object());
        SpaceObjectReference observed(spaceobj.space(), ObjectReference(update.object()));
        ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(observed);

        if (!proxy_obj) {
            mOrphanLocUpdates.addOrphanUpdate(observed, update);
            continue;
        }

        processLocationUpdate(spaceobj, proxy_obj, update);
    }

    return true;
}

bool HostedObject::handleProximityMessage(const SpaceObjectReference& spaceobj, const std::string& payload)
{
    Sirikata::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(payload);
    if (!parse_success) return false;


    SpaceID space = spaceobj.space();
    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Prox::ProximityUpdate update = contents.update(idx);

        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);

            SpaceObjectReference proximateID(spaceobj.space(), ObjectReference(addition.object()));

            TimedMotionVector3f loc(localTime(space, addition.location().t()), MotionVector3f(addition.location().position(), addition.location().velocity()));

            CONTEXT_OHTRACE(prox,
                spaceobj.object().getAsUUID(),
                //getUUID(),
                addition.object(),
                true,
                loc
            );

            TimedMotionQuaternion orient(localTime(space, addition.orientation().t()), MotionQuaternion(addition.orientation().position(), addition.orientation().velocity()));
            BoundingSphere3f bnds = addition.bounds();
            String mesh = (addition.has_mesh() ? addition.mesh() : "");
            String phy = (addition.has_physics() ? addition.physics() : "");

            ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(),spaceobj.object());
            if (!proxy_manager)
            {
                return true;
            }


            ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(proximateID);
            if (!proxy_obj) {
                Transfer::URI meshuri;
                if (addition.has_mesh()) meshuri = Transfer::URI(addition.mesh());

                // FIXME use weak_ptr instead of raw
                proxy_obj = createProxy(proximateID, spaceobj, meshuri, loc, orient, bnds, phy,SolidAngle::Max);
            }
            else {
                // Reset so that updates from this new "session" for this proxy
                // get applied
                proxy_obj->reset();

                // We need to handle optional values properly -- they
                // shouldn't get overwritten.
                String* mesh_ptr = (addition.has_mesh() ? &mesh : NULL);
                String* phy_ptr = (addition.has_physics() ? &phy : NULL);
                assert(mesh_ptr != NULL && mesh != "");
                processLocationUpdate(space, proxy_obj, 0, true, &loc, &orient, &bnds, mesh_ptr, phy_ptr);
            }
            // Always mark the object as valid (either revalidated, or just
            // valid for the first time)
            if (proxy_obj) proxy_obj->validate();


            // Notify of any out of order loc updates
            OrphanLocUpdateManager::UpdateList orphan_loc_updates = mOrphanLocUpdates.getOrphanUpdates(proximateID);
            for(OrphanLocUpdateManager::UpdateList::iterator orphan_it = orphan_loc_updates.begin(); orphan_it != orphan_loc_updates.end(); orphan_it++)
                processLocationUpdate(spaceobj,proxy_obj, *orphan_it);

            //tells the object script that something that was close has come
            //into view
            if(mObjectScript)
                mObjectScript->notifyProximate(proxy_obj,spaceobj);
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);


            ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(), spaceobj.object());

            if (!proxy_manager)
                continue;

            SpaceObjectReference removed_obj_ref(spaceobj.space(),
                ObjectReference(removal.object()));
            if (mPresenceData->find(removed_obj_ref) != mPresenceData->end()) {
                SILOG(oh,detailed,"Ignoring self removal from proximity results.");
            }
            else {
                ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(removed_obj_ref);
                if (proxy_obj) {
                    proxy_manager->destroyObject(proxy_obj);

                    if (mObjectScript)
                        mObjectScript->notifyProximateGone(proxy_obj,spaceobj);

                    proxy_obj->invalidate();
                }
            }

            CONTEXT_OHTRACE(prox,
                spaceobj.object().getAsUUID(),
                //getUUID(),
                removal.object(),
                false,
                TimedMotionVector3f()
            );
        }
    }



    return true;
}


ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmq, const BoundingSphere3f& bs, const String& phy,const SolidAngle& queryAngle)
{
    ProxyManagerPtr proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());

    if (!proxy_manager)
    {
        mPresenceData->insert(PresenceDataMap::value_type( owner_objref, PerPresenceData(this, owner_objref.space(),owner_objref.object(), queryAngle )));
        proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());
    }

    ProxyObjectPtr proxy_obj = ProxyObject::construct<ProxyObject> (proxy_manager.get(),objref,getSharedPtr(),owner_objref);

    // The redundancy here is confusing, but is for the sake of simplicity
    // elsewhere. First, we make sure all the values are set properly so that
    // when we call ProxyManager::createObject, the proxy passed to listeners
    // (for onCreateProxy) will be completely setup, making it valid for use:
    proxy_obj->setLocation(tmv, 0);
    proxy_obj->setOrientation(tmq, 0);
    proxy_obj->setBounds(bs, 0);
    if(meshuri)
        proxy_obj->setMesh(meshuri, 0);
    if(phy.size() > 0)
        proxy_obj->setPhysics(phy, 0);

    proxy_manager->createObject(proxy_obj);

    // Then we repeat it all for the sake of listeners who only pay attention to
    // updates from, e.g., PositionListener or MeshListener.
    proxy_obj->setLocation(tmv, 0);
    proxy_obj->setOrientation(tmq, 0);
    proxy_obj->setBounds(bs, 0);
    if(meshuri)
        proxy_obj->setMesh(meshuri, 0);
    if(phy.size() > 0)
        proxy_obj->setPhysics(phy, 0);

    return proxy_obj;
}


ProxyManagerPtr HostedObject::presence(const SpaceObjectReference& sor)
{
    //    ProxyManagerPtr proxyManPtr = getProxyManager(sor.space(),sor.object());
    //  return proxyManPtr;
    return getProxyManager(sor.space(), sor.object());
}
ProxyObjectPtr HostedObject::getDefaultProxyObject(const SpaceID& space)
{
    ObjectReference oref = mPresenceData->begin()->first.object();
    return  getProxy(space, oref);
}

ProxyManagerPtr HostedObject::getDefaultProxyManager(const SpaceID& space)
{
    ObjectReference oref = mPresenceData->begin()->first.object();
    return  getProxyManager(space, oref);
}



ProxyObjectPtr HostedObject::self(const SpaceObjectReference& sor)
{
    ProxyManagerPtr proxy_man = presence(sor);
    if (!proxy_man) return ProxyObjectPtr();
    ProxyObjectPtr proxy_obj = proxy_man->getProxyObject(sor);
    return proxy_obj;
}


// ODP::Service Interface
ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref, ODP::PortID port)
{
    return mDelegateODPService->bindODPPort(space, objref, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor, ODP::PortID port) {
    return mDelegateODPService->bindODPPort(sor, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref) {
    return mDelegateODPService->bindODPPort(space, objref);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor) {
    return mDelegateODPService->bindODPPort(sor);
}

ODP::PortID HostedObject::unusedODPPort(const SpaceID& space, const ObjectReference& objref) {
    return mDelegateODPService->unusedODPPort(space, objref);
}

ODP::PortID HostedObject::unusedODPPort(const SpaceObjectReference& sor) {
    return mDelegateODPService->unusedODPPort(sor);
}

void HostedObject::registerDefaultODPHandler(const ODP::MessageHandler& cb) {
    mDelegateODPService->registerDefaultODPHandler(cb);
}

ODP::DelegatePort* HostedObject::createDelegateODPPort(ODP::DelegateService* parentService, const SpaceObjectReference& spaceobj, ODP::PortID port) {
    assert(spaceobj.space() != SpaceID::any());
    assert(spaceobj.space() != SpaceID::null());

    // FIXME We used to check if we had a presence in the space, but are now
    // allocating based on the full SpaceObjectReference (presence ID).  We
    // should verify we have this presence.
    //SpaceDataMap::const_iterator space_data_it = mSpaceData->find(space);
    //if (space_data_it == mSpaceData->end())
    //    throw ODP::PortAllocationException("HostedObject::createDelegateODPPort can't allocate port because the HostedObject is not connected to the specified space.");

    ODP::Endpoint port_ep(spaceobj, port);
    return new ODP::DelegatePort(
        mDelegateODPService,
        port_ep,
        std::tr1::bind(
            &HostedObject::delegateODPPortSend, this,
            port_ep, _1, _2
        )
    );
}

bool HostedObject::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload)
{
    assert(source_ep.space() == dest_ep.space());
    SpaceObjectReference sporef(source_ep.space(), source_ep.object());
    return mObjectHost->send(sporef,source_ep.space(), source_ep.port(), dest_ep.object().getAsUUID(),dest_ep.port(), payload);
}



void HostedObject::requestLocationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f& loc)
{
    updateLocUpdateRequest(space, oref,&loc, NULL, NULL, NULL, NULL);
}

//only update the position of the object, leave the velocity and orientation unaffected
void HostedObject::requestPositionUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& pos)
{
    Vector3f curVel = requestCurrentVelocity(space,oref);
    //TimedMotionVector3f tmv
    //(currentSpaceTime(space),MotionVector3f(pos,curVel));
    TimedMotionVector3f tmv (currentLocalTime(),MotionVector3f(pos,curVel));
    requestLocationUpdate(space,oref,tmv);
}

//only update the velocity of the object, leave the position and the orientation
//unaffected
void HostedObject::requestVelocityUpdate(const SpaceID& space,  const ObjectReference& oref, const Vector3f& vel)
{
    Vector3f curPos = Vector3f(requestCurrentPosition(space,oref));

    TimedMotionVector3f tmv(currentLocalTime(),MotionVector3f(curPos,vel));
    requestLocationUpdate(space,oref,tmv);
}

//send a request to update the orientation of this object
void HostedObject::requestOrientationDirectionUpdate(const SpaceID& space, const ObjectReference& oref,const Quaternion& quat)
{
    Quaternion curQuatVel = requestCurrentQuatVel(space,oref);
    TimedMotionQuaternion tmq (currentLocalTime(),MotionQuaternion(quat,curQuatVel));
    requestOrientationUpdate(space,oref, tmq);
}


Quaternion HostedObject::requestCurrentQuatVel(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return proxy_obj->getOrientationSpeed();
}


Quaternion HostedObject::requestCurrentOrientation(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    Location curLoc = proxy_obj->extrapolateLocation(currentLocalTime());
    return curLoc.getOrientation();
}

Quaternion HostedObject::requestCurrentOrientationVel(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    Quaternion returner  = proxy_obj->getOrientationSpeed();
    return returner;
}

void HostedObject::requestOrientationVelocityUpdate(const SpaceID& space, const ObjectReference& oref, const Quaternion& quat)
{
    Quaternion curOrientQuat = requestCurrentOrientation(space,oref);
    TimedMotionQuaternion tmq (currentLocalTime(),MotionQuaternion(curOrientQuat,quat));
    requestOrientationUpdate(space, oref,tmq);
}



//goes into proxymanager and gets out the current location of the presence
//associated with
Vector3d HostedObject::requestCurrentPosition (const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj  = getProxy(space,oref);

    if (proxy_obj == nullPtr)
    {
        SILOG(cppoh,error,"[HO] Unknown space object reference looking for position for for  " << space<< "-"<<oref<<".");
        return Vector3d::nil();
    }

    return requestCurrentPosition(proxy_obj);
}

//skips the proxymanager.  can directly extrapolate position using current
//simulation time.
Vector3d HostedObject::requestCurrentPosition(ProxyObjectPtr proxy_obj)
{
    Location curLoc = proxy_obj->extrapolateLocation(currentLocalTime());
    Vector3d currentPosition = curLoc.getPosition();
    return currentPosition;
}


//apparently, will always return true now.  even if camera.
bool HostedObject::requestMeshUri(const SpaceID& space, const ObjectReference& oref, Transfer::URI& tUri)
{

    ProxyManagerPtr proxy_manager = getProxyManager(space,oref);
    ProxyObjectPtr  proxy_obj     = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));

    tUri = proxy_obj->getMesh();
    return true;
}

Vector3f HostedObject::requestCurrentVelocity(ProxyObjectPtr proxy_obj)
{
    return (Vector3f)proxy_obj->getVelocity();
}


Vector3f HostedObject::requestCurrentVelocity(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    if (proxy_obj == nullPtr)
    {
        SILOG(cppoh,error,"[HO] Unknown space object reference looking for velocity for for  " << space<< "-"<<oref<<".");
        return Vector3f::nil();
    }

    return requestCurrentVelocity(proxy_obj);
}

void HostedObject::requestOrientationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionQuaternion& orient) {
    updateLocUpdateRequest(space, oref, NULL, &orient, NULL, NULL, NULL);
}

BoundingSphere3f HostedObject::requestCurrentBounds(const SpaceID& space,const ObjectReference& oref) {
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return proxy_obj->getBounds();
}

void HostedObject::requestBoundsUpdate(const SpaceID& space, const ObjectReference& oref, const BoundingSphere3f& bounds) {
    updateLocUpdateRequest(space, oref,NULL, NULL, &bounds, NULL, NULL);
}

void HostedObject::requestMeshUpdate(const SpaceID& space, const ObjectReference& oref, const String& mesh)
{
    updateLocUpdateRequest(space, oref, NULL, NULL, NULL, &mesh, NULL);
}

const String& HostedObject::requestCurrentPhysics(const SpaceID& space,const ObjectReference& oref) {
    ProxyObjectPtr proxy_obj = getProxy(space, oref);
    return proxy_obj->getPhysics();
}


SolidAngle HostedObject::requestQueryAngle(const SpaceID& space, const ObjectReference& oref)
{
    PresenceDataMap::iterator iter = mPresenceData->find(SpaceObjectReference(space,oref));
    if (iter == mPresenceData->end())
    {
        SILOG(cppoh, error, "Error in cppoh, requesting solid angle for presence that doesn't exist in your presence map.  Returning max solid angle instead.");
        return SolidAngle::Max;
    }
    return iter->second.queryAngle;
}


void HostedObject::requestPhysicsUpdate(const SpaceID& space, const ObjectReference& oref, const String& phy)
{
    updateLocUpdateRequest(space, oref, NULL, NULL, NULL, NULL, &phy);
}


void HostedObject::requestQueryUpdate(const SpaceID& space, const ObjectReference& oref, SolidAngle new_angle) {
    Protocol::Prox::QueryRequest request;
    request.set_query_angle(new_angle.asFloat());
    std::string payload = serializePBJMessage(request);


    PresenceDataMap::iterator pdmIter = mPresenceData->find(SpaceObjectReference(space,oref));
    if (pdmIter != mPresenceData->end())
        pdmIter->second.queryAngle = new_angle;
    else
        SILOG(cppoh,error,"Error in cppoh, requesting solid angle update for presence that doesn't exist in your presence map.");


    SSTStreamPtr spaceStream = mObjectHost->getSpaceStream(space, oref);
    //SSTStreamPtr spaceStream = mObjectHost->getSpaceStream(space, getUUID());
    if (spaceStream != SSTStreamPtr()) {
        SSTConnectionPtr conn = spaceStream->connection().lock();
        assert(conn);

        conn->datagram(
            (void*)payload.data(), payload.size(),
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY,
            NULL
        );
    }
}

void HostedObject::requestQueryRemoval(const SpaceID& space, const ObjectReference& oref) {
    requestQueryUpdate(space, oref, SolidAngle::Max);
}

void HostedObject::updateLocUpdateRequest(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh, const String* const phy) {
    assert(mPresenceData->find(SpaceObjectReference(space, oref)) != mPresenceData->end());
    PerPresenceData& pd = (mPresenceData->find(SpaceObjectReference(space, oref)))->second;

    if (loc != NULL) { pd.requestLocation = *loc; pd.updateFields |= PerPresenceData::LOC_FIELD_LOC; }
    if (orient != NULL) { pd.requestOrientation = *orient; pd.updateFields |= PerPresenceData::LOC_FIELD_ORIENTATION; }
    if (bounds != NULL) { pd.requestBounds = *bounds; pd.updateFields |= PerPresenceData::LOC_FIELD_BOUNDS; }
    if (mesh != NULL) { pd.requestMesh = *mesh; pd.updateFields |= PerPresenceData::LOC_FIELD_MESH; }
    if (phy != NULL) { pd.requestPhysics = *phy; pd.updateFields |= PerPresenceData::LOC_FIELD_PHYSICS; }

    // Cancel the re-request timer if it was active
    pd.rerequestTimer->cancel();

    sendLocUpdateRequest(space, oref);
}





void HostedObject::sendLocUpdateRequest(const SpaceID& space, const ObjectReference& oref) {
    assert(mPresenceData->find(SpaceObjectReference(space, oref)) != mPresenceData->end());
    PerPresenceData& pd = (mPresenceData->find(SpaceObjectReference(space, oref)))->second;

    ProxyObjectPtr self_proxy = getProxy(space, oref);
    // Generate and send an update to Loc
    Protocol::Loc::Container container;
    Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
    if (pd.updateFields & PerPresenceData::LOC_FIELD_LOC) {
        self_proxy->setLocation(pd.requestLocation, 0, true);
        Protocol::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t( spaceTime(space, pd.requestLocation.updateTime()) );
        requested_loc.set_position(pd.requestLocation.position());
        requested_loc.set_velocity(pd.requestLocation.velocity());
    }
    if (pd.updateFields & PerPresenceData::LOC_FIELD_ORIENTATION) {
        self_proxy->setOrientation(pd.requestOrientation, 0, true);
        Protocol::ITimedMotionQuaternion requested_orient = loc_request.mutable_orientation();
        requested_orient.set_t( spaceTime(space, pd.requestOrientation.updateTime()) );
        //Normalize positions, which only make sense as unit quaternions.
        requested_orient.set_position(pd.requestOrientation.position().normal());
        requested_orient.set_velocity(pd.requestOrientation.velocity());
    }
    if (pd.updateFields & PerPresenceData::LOC_FIELD_BOUNDS) {
        self_proxy->setBounds(pd.requestBounds, 0, true);
        loc_request.set_bounds(pd.requestBounds);
    }
    if (pd.updateFields & PerPresenceData::LOC_FIELD_MESH) {
        self_proxy->setMesh(Transfer::URI(pd.requestMesh), 0, true);
        loc_request.set_mesh(pd.requestMesh);
    }
    if (pd.updateFields & PerPresenceData::LOC_FIELD_PHYSICS) {
        self_proxy->setPhysics(pd.requestPhysics, 0, true);
        loc_request.set_physics(pd.requestPhysics);
    }

    std::string payload = serializePBJMessage(container);


    bool send_succeeded = false;
    SSTStreamPtr spaceStream = mObjectHost->getSpaceStream(space, oref);
    if (spaceStream) {
        SSTConnectionPtr conn = spaceStream->connection().lock();
        assert(conn);

        send_succeeded = conn->datagram( (void*)payload.data(), payload.size(), OBJECT_PORT_LOCATION,
            OBJECT_PORT_LOCATION, NULL);
    }

    if (send_succeeded) {
        pd.updateFields = PerPresenceData::LOC_FIELD_NONE;
    }
    else {
        // Set up retry timer. Just rerun this method, but add no new
        // update fields.
        pd.rerequestTimer->wait(
            Duration::milliseconds((int64)10),
            std::tr1::bind(&HostedObject::sendLocUpdateRequest, this, space, oref)
        );
    }
}


Location HostedObject::getLocation(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy = getProxy(space, oref);
    assert(proxy);
    Location currentLoc = proxy->globalLocation(currentSpaceTime(space));
    return currentLoc;
}

}
