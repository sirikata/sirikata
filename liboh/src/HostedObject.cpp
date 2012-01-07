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
#include <sirikata/oh/ObjectQueryProcessor.hpp>

#include <sirikata/core/util/ThreadId.hpp>
#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/core/odp/Exceptions.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>
#include <list>
#include <vector>
#include <sirikata/oh/SimulationFactory.hpp>
#include "PerPresenceData.hpp"

#include <sirikata/oh/LocUpdate.hpp>
#include <sirikata/oh/ProtocolLocUpdate.hpp>
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"

// Property tree for old API for queries
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


#define HO_LOG(lvl,msg) SILOG(ho,lvl,msg);

namespace Sirikata {



HostedObject::HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID & _id)
 : mContext(ctx),
   mID(_id),
   mObjectHost(parent),
   mObjectScript(NULL),
   mNextSubscriptionID(0),
   destroyed(false)
{
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


Simulation* HostedObject::runSimulation(const SpaceObjectReference& sporef, const String& simName)
{
    Simulation* sim = NULL;

    if (stopped()) return sim;

    PresenceDataMap::iterator psd_it = mPresenceData.find(sporef);
    if (psd_it == mPresenceData.end())
    {
        HO_LOG(error, "Error requesting to run a simulation for a presence that does not exist.");
        return NULL;
    }

    PerPresenceData& pd =  *psd_it->second;
    bool newSimListener = addSimListeners(pd,simName,sim);

    if ((sim != NULL) && (newSimListener))
    {
        HO_LOG(detailed, "Adding simulation to context");
        mContext->add(sim);
    }
    return sim;
}


HostedObject::~HostedObject() {
    destroy(false);
    for (PresenceDataMap::iterator i=mPresenceData.begin();i!=mPresenceData.end();++i) {
        delete i->second;
    }

    delete mDelegateODPService;
    getObjectHost()->hostedObjectDestroyed(id());
    stop();
}

const UUID& HostedObject::id() const {
    return mID;
}

void HostedObject::start() {
    // Not really any need for the start call -- its always going to be invoked
    // immediately after construction.
}

void HostedObject::stop() {
    if (mObjectScript)
        mObjectScript->stop();
}
bool HostedObject::stopped() const {
    return (mContext->stopped() || destroyed);
}
namespace {
void nop (const HostedObjectPtr&) {

}
}

void HostedObject::destroy(bool need_self) {
    // Avoid recursive destruction
    if (destroyed) return;

    // Make sure that we survive the entire duration of this call. Otherwise all
    // references may be lost, resulting in the destructor getting called
    // (e.g. when the ObjectScript removes all references) and then we return
    // here to do more work and we've already been deleted.
    HostedObjectPtr self_ptr = need_self ? getSharedPtr() : HostedObjectPtr();

    destroyed = true;

    if (mObjectScript) {
        delete mObjectScript;
        mObjectScript=NULL;
    }

    for (PresenceDataMap::iterator iter = mPresenceData.begin(); iter != mPresenceData.end(); ++iter) {
        // Make sure we explicitly. Other paths don't necessarily do this,
        // e.g. if the call to destroy happens between receiving a connection
        // success and the actual creation of the stream from the space, leaving
        // other components unaware that the connection has been made. Worst
        // case, the object host/session manager ignore the request.
        mObjectHost->disconnectObject(iter->first.space(),iter->first.object());
        delete iter->second;
        // And just clear the ref out from the ObjectHost
        mObjectHost->unregisterHostedObject(iter->first,this);
    }

    mPresenceData.clear();
}

Time HostedObject::spaceTime(const SpaceID& space, const Time& t) {
    return mObjectHost->spaceTime(space, t);
}

Time HostedObject::currentSpaceTime(const SpaceID& space) {
    return mObjectHost->currentSpaceTime(space);
}

Time HostedObject::localTime(const SpaceID& space, const Time& t) {
    return mObjectHost->localTime(space, t);
}

Time HostedObject::currentLocalTime() {
    return mObjectHost->currentLocalTime();
}


ProxyManagerPtr HostedObject::getProxyManager(const SpaceID& space, const ObjectReference& oref)
{
    SpaceObjectReference toFind(space,oref);
    PresenceDataMap::const_iterator it = mPresenceData.find(toFind);
    if (it == mPresenceData.end())
        return ProxyManagerPtr();

    return it->second->proxyManager;
}


//returns all the spaceobjectreferences associated with the presence with id sporef
void HostedObject::getProxySpaceObjRefs(const SpaceObjectReference& sporef,SpaceObjRefVec& ss) const
{
    PresenceDataMap::const_iterator smapIter = mPresenceData.find(sporef);

    if (smapIter != mPresenceData.end())
    {
        //means that we actually did have a connection with this sporef
        //load the proxy objects the sporef'd connection has actually seen into
        //ss.
        smapIter->second->proxyManager->getAllObjectReferences(ss);
    }
}



//returns all the spaceobjrefs associated with all presences of this object.
//They are returned in ss.
void HostedObject::getSpaceObjRefs(SpaceObjRefVec& ss) const
{

    PresenceDataMap::const_iterator smapIter;
    for (smapIter = mPresenceData.begin(); smapIter != mPresenceData.end(); ++smapIter)
        ss.push_back(SpaceObjectReference(smapIter->second->space,smapIter->second->object));
}



static ProxyObjectPtr nullPtr;
const ProxyObjectPtr &HostedObject::getProxyConst(const SpaceID &space, const ObjectReference& oref) const
{
    PresenceDataMap::const_iterator iter = mPresenceData.find(SpaceObjectReference(space,oref));
    if (iter == mPresenceData.end()) {
        return nullPtr;
    }
    return iter->second->mProxyObject;
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
    if (stopped()) {
        HO_LOG(warn,"Ignoring script initialization after stop was requested.");
        return;
    }

    if (mObjectScript) {
        HO_LOG(warn,"Ignored initializeScript because script already exists for object");
        return;
    }

    HO_LOG(detailed,"Creating a script object for object");

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
            HO_LOG(debug,"[HO] Failed to create script for object because incorrect script type");
        }
    }
    ObjectScriptManager *mgr = mObjectHost->getScriptManager(script_type);
    if (mgr) {
        HO_LOG(insane,"[HO] Creating script for object with args of "<<args);
        mObjectScript = mgr->createObjectScript(this->getSharedPtr(), args, script);
        mObjectScript->start();
        mObjectScript->scriptTypeIs(script_type);
        mObjectScript->scriptOptionsIs(args);
    }
}

bool HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& phy,
        const UUID&object_uuid_evidence,
        PresenceToken token)
{
    return connect(spaceID, startingLocation, meshBounds, mesh, phy, SolidAngle::Max, 0, object_uuid_evidence,ObjectReference::null(),token);
}

String HostedObject::encodeDefaultQuery(const SolidAngle& qangle, const uint32 max_count) {
    // For old format, assume we just encode the query parameters assuming the
    // standard, basic, solid angle query

    String query;
    using namespace boost::property_tree;
    bool with_query = qangle != SolidAngle::Max;
    if (with_query) {
        try {
            ptree pt;
            pt.put("angle", qangle.asFloat());
            pt.put("max_results", max_count);
            std::stringstream data_json;
            write_json(data_json, pt);
            query = data_json.str();
        }
        catch(json_parser::json_parser_error exc) {
            return false;
        }
    }
    return query;
}

bool HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& phy,
        const SolidAngle& queryAngle,
        uint32 queryMaxResults,
        const UUID&object_uuid_evidence,
        const ObjectReference& orefID,
        PresenceToken token)
{
    DEPRECATED(ho);
    return connect(
        spaceID, startingLocation, meshBounds, mesh, phy,
        encodeDefaultQuery(queryAngle, queryMaxResults),
        object_uuid_evidence,
        orefID, token
    );
}

bool HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& physics,
        const String& query,
        const UUID&object_uuid_evidence,
        const ObjectReference& orefID,
        PresenceToken token) {
    if (stopped()) {
        HO_LOG(warn,"Ignoring HostedObject connection request after system stop requested.");
        return false;
    }

    if (spaceID == SpaceID::null())
        return false;

    ObjectReference oref = (orefID == ObjectReference::null()) ? ObjectReference(UUID::random()) : orefID;

    SpaceObjectReference connectingSporef (spaceID,oref);

    // Note: we always use Time::null() here.  The server will fill in the
    // appropriate value.  When we get the callback, we can fix this up.
    Time approx_server_time = Time::null();
    if (mObjectHost->connect(
            getSharedPtr(),
        connectingSporef, spaceID,
        TimedMotionVector3f(approx_server_time, MotionVector3f( Vector3f(startingLocation.getPosition()), startingLocation.getVelocity()) ),
        TimedMotionQuaternion(approx_server_time,MotionQuaternion(startingLocation.getOrientation().normal(),Quaternion(startingLocation.getAxisOfRotation(),startingLocation.getAngularSpeed()))),  //normalize orientations
        meshBounds,
        mesh,
        physics,
        query,
        std::tr1::bind(&HostedObject::handleConnected, getWeakPtr(), _1, _2, _3),
        std::tr1::bind(&HostedObject::handleMigrated, getWeakPtr(), _1, _2, _3),
        std::tr1::bind(&HostedObject::handleStreamCreated, getWeakPtr(), _1, _2, token),
        std::tr1::bind(&HostedObject::handleDisconnected, getWeakPtr(), _1, _2)
        )) {
        mObjectHost->registerHostedObject(connectingSporef,getSharedPtr());
        return true;
    }else {
        return false;
    }
}




//returns true if sim gets an already-existing listener.  false otherwise
bool HostedObject::addSimListeners(PerPresenceData& pd, const String& simName, Simulation*& sim)
{
    if (pd.sims.find(simName) != pd.sims.end()) {
        sim = pd.sims[simName];
        return false;
    }

    HO_LOG(info,String("[OH] Initializing ") + simName);
    sim = SimulationFactory::getSingleton().getConstructor ( simName ) ( mContext, static_cast<ConnectionEventProvider*>(mObjectHost), getSharedPtr(), pd.id(), getObjectHost()->getSimOptions(simName));
    if (!sim)
    {
        HO_LOG(error, "Unable to load " << simName << " plugin.");
        return true;
    }
    else
    {
        pd.sims[simName] = sim;
        HO_LOG(info,String("Successfully initialized ") + simName);
    }
    return true;
}



void HostedObject::handleConnected(const HostedObjectWPtr& weakSelf, const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info)
{
    HostedObjectPtr self(weakSelf.lock());
    if ((!self)||self->stopped()) {
        HO_LOG(detailed,"Ignoring connection success after system stop requested.");
        return;
    }
    if (info.server == NullServerID)
    {
        HO_LOG(warning,"Earlier failure to connect object:" << obj << " to space " << space);
        return;

    }
    BaseDatagramLayerPtr baseDatagramLayer (self->mContext->sstConnMgr()->createDatagramLayer(
              SpaceObjectReference(space, obj),
              self->mContext, self->mDelegateODPService
           )   );

    // We have to manually do what mContext->mainStrand->wrap( ... ) should be
    // doing because it can't handle > 5 arguments.
    self->mContext->mainStrand->post(
        std::tr1::bind(&HostedObject::handleConnectedIndirect, weakSelf, space, obj, info, baseDatagramLayer)
    );
}


void HostedObject::handleConnectedIndirect(const HostedObjectWPtr& weakSelf, const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info, const BaseDatagramLayerPtr& baseDatagramLayer)
{
    if (info.server == NullServerID)
    {
        HO_LOG(warning,"Failed to connect object:" << obj << " to space " << space);
        return;
    }
    HostedObjectPtr self(weakSelf.lock());
    if (!self)
        return;
    SpaceObjectReference self_objref(space, obj);
    if(self->mPresenceData.find(self_objref) == self->mPresenceData.end())
    {
        self->mPresenceData.insert(
            PresenceDataMap::value_type(
                self_objref,
                new PerPresenceData(self.get(), space, obj, baseDatagramLayer, info.query)
            )
        );
    }

    // Convert back to local time
    TimedMotionVector3f local_loc(self->localTime(space, info.loc.updateTime()), info.loc.value());
    TimedMotionQuaternion local_orient(self->localTime(space, info.orient.updateTime()), info.orient.value());
    ProxyObjectPtr self_proxy = self->createProxy(self_objref, self_objref, Transfer::URI(info.mesh), local_loc, local_orient, info.bnds, info.physics, info.query, 0);

    // Use to initialize PerSpaceData
    PresenceDataMap::iterator psd_it = self->mPresenceData.find(self_objref);
    PerPresenceData& psd = *psd_it->second;
    self->initializePerPresenceData(psd, self_proxy);

    HO_LOG(detailed,"Connected object " << obj << " to space " << space << " waiting on notice");
}

void HostedObject::handleMigrated(const HostedObjectWPtr& weakSelf, const SpaceID& space, const ObjectReference& obj, ServerID server)
{
    HostedObjectPtr self(weakSelf.lock());
    if (!self)
        return;
    // When we switch space servers, the ProxyObject's sequence
    // numbers will no longer match because this information isn't
    // moved with the object. Since we shouldn't get more updates from
    // the original server, we reset all the ProxyObjects *owned* by
    // this object to have seqno = 0 so they will start fresh for the
    // new space server.
    ProxyManagerPtr proxy_manager = self->getProxyManager(space, obj);
    if (!proxy_manager) {
        HO_LOG(error, "Got migrated message but don't have a ProxyManager for the object.");
        return;
    }
    std::vector<SpaceObjectReference> proxy_names;
    proxy_manager->getAllObjectReferences(proxy_names);
    for(std::vector<SpaceObjectReference>::iterator it = proxy_names.begin(); it != proxy_names.end(); it++) {
        ProxyObjectPtr proxy = proxy_manager->getProxyObject(*it);
        proxy->reset();
    }
}



void HostedObject::handleStreamCreated(const HostedObjectWPtr& weakSelf, const SpaceObjectReference& spaceobj, SessionManager::ConnectionEvent after, PresenceToken token) {
    HO_LOG(detailed,"Handling new SST stream from space server for " << spaceobj);
    HostedObjectPtr self(weakSelf.lock());
    if (!self)
        return;

    HO_LOG(detailed,"Notifying of connected object " << spaceobj.object() << " to space " << spaceobj.space());
    if (after == SessionManager::Connected)
        self->notify(&SessionEventListener::onConnected, self, spaceobj, token);
    else if (after == SessionManager::Migrated)
        self->notify(&SessionEventListener::onMigrated, self, spaceobj, token);
}




void HostedObject::initializePerPresenceData(PerPresenceData& psd, ProxyObjectPtr selfproxy) {
    psd.initializeAs(selfproxy);
}

void HostedObject::disconnectFromSpace(const SpaceID &spaceID, const ObjectReference& oref)
{
    if (stopped()) {
        HO_LOG(detailed,"Ignoring disconnection request after system stop requested.");
        return;
    }

    SpaceObjectReference sporef(spaceID, oref);

    PresenceDataMap::iterator where;
    where=mPresenceData.find(sporef);
    if (where!=mPresenceData.end()) {
        // Need to actually send a disconnection request to the space. Note that
        // this occurse *before* getting rid of the other data so callbacks
        // invoked as a result still work.
        mObjectHost->disconnectObject(spaceID,oref);
        delete where->second;
        mPresenceData.erase(where);
        mObjectHost->unregisterHostedObject(sporef, this);
    } else {
        SILOG(cppoh,error,"Attempting to disconnect from space "<<spaceID<<" and object: "<< oref<<" when not connected to it...");
    }
}

void HostedObject::handleDisconnected(const HostedObjectWPtr& weakSelf, const SpaceObjectReference& spaceobj, Disconnect::Code cc) {
    HostedObjectPtr self(weakSelf.lock());
    if ((!self)||self->stopped()) {
        HO_LOG(detailed,"Ignoring disconnection callback after system stop requested.");
        return;
    }

    self->notify(&SessionEventListener::onDisconnected, self, spaceobj);

    // Only invoke disconnectFromSpace if we weren't already aware of the
    // disconnection, i.e. if the disconnect was due to the space and we haven't
    // cleaned up yet.
    if (cc == Disconnect::Forced)
        self->disconnectFromSpace(spaceobj.space(), spaceobj.object());
    if (cc == Disconnect::LoginDenied) {
        assert(self->mPresenceData.find(spaceobj)==self->mPresenceData.end());
        self->mObjectHost->unregisterHostedObject(spaceobj, self.get());
    }
}


void HostedObject::receiveMessage(const SpaceID& space, const Protocol::Object::ObjectMessage* msg) {
    if (stopped()) {
        HO_LOG(detailed,"Ignoring received message after system stop requested.");
        return;
    }

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


void HostedObject::processLocationUpdate(const SpaceObjectReference& sporef, ProxyObjectPtr proxy_obj, const LocUpdate& update) {
    TimedMotionVector3f loc;
    TimedMotionQuaternion orient;
    BoundingSphere3f bounds;
    String mesh;
    String phy;

    TimedMotionVector3f* locptr = NULL;
    uint64 location_seqno = update.location_seqno();
    TimedMotionQuaternion* orientptr = NULL;
    uint64 orient_seqno = update.orientation_seqno();
    BoundingSphere3f* boundsptr = NULL;
    uint64 bounds_seqno = update.bounds_seqno();
    String* meshptr = NULL;
    uint64 mesh_seqno = update.mesh_seqno();
    String* phyptr = NULL;
    uint64 phy_seqno = update.physics_seqno();

    if (update.has_location()) {
        loc = update.locationWithLocalTime(this, sporef.space());

        CONTEXT_OHTRACE(objectLoc,
            sporef.object().getAsUUID(),
            //getUUID(),
            update.object().getAsUUID(),
            loc
        );

        locptr = &loc;
    }

    if (update.has_orientation()) {
        orient = update.orientationWithLocalTime(this, sporef.space());
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
    processLocationUpdate(
        sporef.space(), proxy_obj, false,
        locptr, location_seqno, orientptr, orient_seqno,
        boundsptr, bounds_seqno, meshptr, mesh_seqno, phyptr, phy_seqno
    );
}


void HostedObject::processLocationUpdate(
        const SpaceID& space, ProxyObjectPtr proxy_obj, bool predictive,
        TimedMotionVector3f* loc, uint64 loc_seqno,
        TimedMotionQuaternion* orient, uint64 orient_seqno,
        BoundingSphere3f* bounds, uint64 bounds_seqno,
        String* mesh, uint64 mesh_seqno,
        String* phy, uint64 phy_seqno
) {
    if (loc)
        proxy_obj->setLocation(*loc, loc_seqno);

    if (orient)
        proxy_obj->setOrientation(*orient, orient_seqno);

    if (bounds)
        proxy_obj->setBounds(*bounds, bounds_seqno);

    if (mesh)
        proxy_obj->setMesh(Transfer::URI(*mesh), mesh_seqno);

    if (phy && *phy != "")
        proxy_obj->setPhysics(*phy, phy_seqno);
}

void HostedObject::handleLocationUpdate(const SpaceObjectReference& observer, const LocUpdate& lu) {
    ProxyManagerPtr proxy_manager = this->getProxyManager(observer.space(), observer.object());
    if (!proxy_manager) {
        HO_LOG(warn,"Hosted Object received a message for a presence without a proxy manager.");
        return;
    }

    SpaceObjectReference observed(observer.space(), ObjectReference(lu.object()));
    ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(observed);
    assert(proxy_obj);

    this->processLocationUpdate( observer, proxy_obj, lu);
}

void HostedObject::handleProximityUpdate(const SpaceObjectReference& spaceobj, const Sirikata::Protocol::Prox::ProximityUpdate& update) {
    HostedObject* self = this;
    SpaceID space = spaceobj.space();

    ProxyManagerPtr proxy_manager = self->getProxyManager(spaceobj.space(),spaceobj.object());
    if (!proxy_manager) {
        HO_LOG(warn,"Hosted Object received a message for a presence without a proxy manager.");
        return;
    }

    for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
        Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
        ProxProtocolLocUpdate add(addition);

        SpaceObjectReference proximateID(spaceobj.space(), add.object());

        TimedMotionVector3f loc(add.locationWithLocalTime(this, spaceobj.space()));

        CONTEXT_OHTRACE(prox,
            spaceobj.object().getAsUUID(),
            //getUUID(),
            addition.object(),
            true,
            loc
        );

        TimedMotionQuaternion orient(add.orientationWithLocalTime(this, spaceobj.space()));
        BoundingSphere3f bnds = add.bounds();
        String mesh = add.meshOrDefault();
        String phy = add.physicsOrDefault();

        ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(proximateID);
        if (!proxy_obj) {
            Transfer::URI meshuri;
            if (addition.has_mesh()) meshuri = Transfer::URI(addition.mesh());

            // FIXME use weak_ptr instead of raw
            uint64 proxyAddSeqNo = add.location_seqno();
            assert( add.location_seqno() == add.orientation_seqno() &&
                 add.location_seqno() == add.bounds_seqno() &&
                add.location_seqno() == add.mesh_seqno() &&
                add.location_seqno() == add.physics_seqno());
            proxy_obj = self->createProxy(proximateID, spaceobj, meshuri, loc, orient, bnds, phy, "", proxyAddSeqNo);
        }
        else {
            // We need to handle optional values properly -- they
            // shouldn't get overwritten.
            String* mesh_ptr = (addition.has_mesh() ? &mesh : NULL);
            String* phy_ptr = (addition.has_physics() ? &phy : NULL);

            self->processLocationUpdate(space, proxy_obj, false,
                &loc, add.location_seqno(),
                &orient, add.orientation_seqno(),
                &bnds, add.bounds_seqno(),
                mesh_ptr, add.mesh_seqno(),
                phy_ptr, add.physics_seqno()
            );
        }

        // Always mark the object as valid (either revalidated, or just
        // valid for the first time)
        if (proxy_obj) proxy_obj->validate();

        //tells the object script that something that was close has come
        //into view
        if(self->mObjectScript)
            self->mObjectScript->notifyProximate(proxy_obj,spaceobj);
    }

    for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
        Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);

        ProxyManagerPtr proxy_manager = self->getProxyManager(spaceobj.space(), spaceobj.object());

        if (!proxy_manager)
            continue;

        SpaceObjectReference removed_obj_ref(spaceobj.space(),
            ObjectReference(removal.object()));
        bool permanent = (removal.has_type() && (removal.type() == Sirikata::Protocol::Prox::ObjectRemoval::Permanent));

        if (self->mPresenceData.find(removed_obj_ref) != self->mPresenceData.end()) {
            SILOG(oh,detailed,"Ignoring self removal from proximity results.");
        }
        else {
            ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(removed_obj_ref);

            if (proxy_obj) {
                // NOTE: We *don't* reset the proxy object
                // here. Resetting it puts the seqnos back at 0,
                // but if we get an addition while still on this
                // space server, we actually want the old ones to
                // stay in place, in case of unordered prox/loc
                // updates. Resetting only happens when we move
                // across space servers (see handleMigrated).
                proxy_manager->destroyObject(proxy_obj);

                if (self->mObjectScript)
                    self->mObjectScript->notifyProximateGone(proxy_obj,spaceobj);

                proxy_obj->invalidate(permanent);
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


ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmq, const BoundingSphere3f& bs, const String& phy, const String& query, uint64 seqNo)
{
    ProxyManagerPtr proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());

    if (!proxy_manager)
    {
        mPresenceData.insert(
            PresenceDataMap::value_type(
                owner_objref,
                new PerPresenceData(this, owner_objref.space(),owner_objref.object(), BaseDatagramLayerPtr(), query)
            )
        );
        proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());
    }

    ProxyObjectPtr proxy_obj = ProxyObject::construct(proxy_manager.get(),objref,getSharedPtr(),owner_objref);

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

    proxy_obj->setLocation(tmv, seqNo);
    proxy_obj->setOrientation(tmq, seqNo);
    proxy_obj->setBounds(bs, seqNo);
    if(meshuri)
        proxy_obj->setMesh(meshuri, seqNo);
    if(phy.size() > 0)
        proxy_obj->setPhysics(phy, seqNo);

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
    ObjectReference oref = mPresenceData.begin()->first.object();
    return  getProxy(space, oref);
}

ProxyManagerPtr HostedObject::getDefaultProxyManager(const SpaceID& space)
{
    ObjectReference oref = mPresenceData.begin()->first.object();
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
    if (stopped()) return NULL;
    return mDelegateODPService->bindODPPort(space, objref, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor, ODP::PortID port) {
    if (stopped()) return NULL;
    return mDelegateODPService->bindODPPort(sor, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref) {
    if (stopped()) return NULL;
    return mDelegateODPService->bindODPPort(space, objref);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor) {
    if (stopped()) return NULL;
    return mDelegateODPService->bindODPPort(sor);
}

ODP::PortID HostedObject::unusedODPPort(const SpaceID& space, const ObjectReference& objref) {
    if (stopped()) return NULL;
    return mDelegateODPService->unusedODPPort(space, objref);
}

ODP::PortID HostedObject::unusedODPPort(const SpaceObjectReference& sor) {
    if (stopped()) return NULL;
    return mDelegateODPService->unusedODPPort(sor);
}

void HostedObject::registerDefaultODPHandler(const ODP::Service::MessageHandler& cb) {
    if (stopped()) return;
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
    if (stopped()) return false;

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
    if (!proxy_obj)
    {
        HO_LOG(warn,"Requesting quat vel for missing proxy.  Returning blank.");
        return Quaternion();
    }

    return proxy_obj->getOrientationSpeed();
}


Quaternion HostedObject::requestCurrentOrientation(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    if (!proxy_obj)
    {
        HO_LOG(warn,"Requesting orientation for missing proxy.  Returning blank.");
        return Quaternion();
    }


    Location curLoc = proxy_obj->extrapolateLocation(currentLocalTime());
    return curLoc.getOrientation();
}

Quaternion HostedObject::requestCurrentOrientationVel(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    if (!proxy_obj)
    {
        HO_LOG(warn,"Requesting current orientation for missing proxy.  Returning blank.");
        return Quaternion();
    }


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
        return Vector3d::zero();
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

    if (!proxy_manager)
    {
        HO_LOG(warn,"Requesting mesh without proxy manager. Doing nothing");
        return false;
    }

    ProxyObjectPtr  proxy_obj     = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));

    if (! proxy_obj)
    {
        HO_LOG(warn,"Requesting mesh for disconnected. Doing nothing");
        return false;
    }

    tUri = proxy_obj->mesh();
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
        return Vector3f::zero();
    }

    return requestCurrentVelocity(proxy_obj);
}

void HostedObject::requestOrientationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionQuaternion& orient) {
    updateLocUpdateRequest(space, oref, NULL, &orient, NULL, NULL, NULL);
}



BoundingSphere3f HostedObject::requestCurrentBounds(const SpaceID& space,const ObjectReference& oref) {
    ProxyObjectPtr proxy_obj = getProxy(space,oref);

    if (!proxy_obj)
    {
        HO_LOG(warn,"Requesting bounding sphere for missing proxy.  Returning blank.");
        return BoundingSphere3f();
    }


    return proxy_obj->bounds();
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
    if (!proxy_obj)
    {
        HO_LOG(warn,"Requesting physics for missing proxy.  Returning blank.");
        static String empty;
        return empty;
    }


    return proxy_obj->physics();
}


const String& HostedObject::requestQuery(const SpaceID& space, const ObjectReference& oref)
{
    PresenceDataMap::iterator iter = mPresenceData.find(SpaceObjectReference(space,oref));
    if (iter == mPresenceData.end())
    {
        SILOG(cppoh, error, "Error in cppoh, requesting solid angle for presence that doesn't exist in your presence map.  Returning max solid angle instead.");
        static String empty_query("");
        return empty_query;
    }
    return iter->second->query;
}

void HostedObject::requestPhysicsUpdate(const SpaceID& space, const ObjectReference& oref, const String& phy)
{
    updateLocUpdateRequest(space, oref, NULL, NULL, NULL, NULL, &phy);
}


void HostedObject::requestQueryUpdate(const SpaceID& space, const ObjectReference& oref, const String& new_query) {
    if (stopped()) {
        HO_LOG(detailed,"Ignoring query update request after system stop.");
        return;
    }

    SpaceObjectReference sporef(space,oref);
    PresenceDataMap::iterator pdmIter = mPresenceData.find(sporef);
    if (pdmIter != mPresenceData.end()) {
        pdmIter->second->query = new_query;
    }
    else {
        SILOG(cppoh,error,"Error in cppoh, requesting solid angle update for presence that doesn't exist in your presence map.");
    }

    mObjectHost->getQueryProcessor()->updateQuery(getSharedPtr(), sporef, new_query);
}

void HostedObject::requestQueryUpdate(const SpaceID& space, const ObjectReference& oref, const SolidAngle& sa, uint32 max_count) {
    DEPRECATED(ho);
    requestQueryUpdate(space, oref, encodeDefaultQuery(sa, max_count));
}

void HostedObject::requestQueryRemoval(const SpaceID& space, const ObjectReference& oref) {
    requestQueryUpdate(space, oref, "");
}

void HostedObject::updateLocUpdateRequest(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh, const String* const phy) {

    if (stopped()) {
        HO_LOG(detailed,"Ignoring loc update request after system stop.");
        return;
    }

    assert(mPresenceData.find(SpaceObjectReference(space, oref)) != mPresenceData.end());
    PerPresenceData& pd = *(mPresenceData.find(SpaceObjectReference(space, oref)))->second;

    if (loc != NULL) { pd.requestLocation = *loc; pd.updateFields |= PerPresenceData::LOC_FIELD_LOC; }
    if (orient != NULL) { pd.requestOrientation = *orient; pd.updateFields |= PerPresenceData::LOC_FIELD_ORIENTATION; }
    if (bounds != NULL) { pd.requestBounds = *bounds; pd.updateFields |= PerPresenceData::LOC_FIELD_BOUNDS; }
    if (mesh != NULL) { pd.requestMesh = *mesh; pd.updateFields |= PerPresenceData::LOC_FIELD_MESH; }
    if (phy != NULL) { pd.requestPhysics = *phy; pd.updateFields |= PerPresenceData::LOC_FIELD_PHYSICS; }

    // Cancel the re-request timer if it was active
    pd.rerequestTimer->cancel();

    sendLocUpdateRequest(space, oref);
}


namespace {
void discardChildStream(int success, SST::Stream<SpaceObjectReference>::Ptr sptr) {
    if (success != SST_IMPL_SUCCESS) return;
    sptr->close(false);
}
}

void HostedObject::sendLocUpdateRequest(const SpaceID& space, const ObjectReference& oref) {
    assert(mPresenceData.find(SpaceObjectReference(space, oref)) != mPresenceData.end());
    PerPresenceData& pd = *(mPresenceData.find(SpaceObjectReference(space, oref)))->second;

    ProxyObjectPtr self_proxy = getProxy(space, oref);

    if (!self_proxy)
    {
        HO_LOG(warn,"Requesting sendLocUpdateRequest for missing self proxy.  Doing nothing.");
        return;
    }


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
        /* datagram update, still supported but gets dropped
        SSTConnectionPtr conn = spaceStream->connection().lock();
        assert(conn);
        send_succeeded = conn->datagram( (void*)payload.data(), payload.size(), OBJECT_PORT_LOCATION,
            OBJECT_PORT_LOCATION, NULL);
        */
        spaceStream->createChildStream(
            std::tr1::bind(discardChildStream, _1, _2),
            (void*)payload.data(), payload.size(),
            OBJECT_PORT_LOCATION, OBJECT_PORT_LOCATION
        );
        send_succeeded = true;
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
    if (!proxy)
    {
        HO_LOG(warn,"Requesting getLocation for missing proxy.  Doing nothing.");
        return Location();
    }

    Location currentLoc = proxy->globalLocation(currentSpaceTime(space));
    return currentLoc;
}

}
