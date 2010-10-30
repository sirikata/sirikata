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
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include <sirikata/proxyobject/ProxyLightObject.hpp>
#include <sirikata/proxyobject/ProxyWebViewObject.hpp>
#include <sirikata/proxyobject/ProxyCameraObject.hpp>
#include <sirikata/proxyobject/LightInfo.hpp>
#include <Protocol_Sirikata.pbj.hpp>
#include <Protocol_Subscription.pbj.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/core/util/RoutableMessage.hpp>
#include <sirikata/core/persistence/PersistenceSentMessage.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/core/util/SentMessage.hpp>
#include <sirikata/oh/ObjectHost.hpp>

#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/util/KnownMessages.hpp>

#include <sirikata/core/util/ThreadId.hpp>
#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/core/odp/Exceptions.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>
#include <list>
#include <vector>
#include <sirikata/proxyobject/SimulationFactory.hpp>
#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Frame.pbj.hpp"

#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"

#define HO_LOG(lvl,msg) SILOG(ho,lvl,"[HO] " << msg);

namespace Sirikata {



HostedObject::HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID &objectName, bool is_camera)
 : mContext(ctx),
   mInternalObjectReference(objectName),
   mIsCamera(is_camera)
{
    mSpaceData = new SpaceDataMap;
    mNextSubscriptionID = 0;
    mObjectHost=parent;
    mObjectScript=NULL;
    mSendService.ho = this;

    
    mDelegateODPService = new ODP::DelegateService(
        std::tr1::bind(
            &HostedObject::createDelegateODPPort, this,
            _1, _2, _3
        )
    );


    mHasScript = false;
    mSSTDatagramLayer = BaseDatagramLayer<UUID>::createDatagramLayer(getUUID(), ctx,this, this);
}

HostedObject::~HostedObject() {
    destroy();
    delete mSpaceData;
}

void HostedObject::init() {
    mObjectHost->registerHostedObject(getSharedPtr());
}

void HostedObject::destroy()
{
    if (mObjectScript) {
        delete mObjectScript;
        mObjectScript=NULL;
    }
    for (SpaceDataMap::iterator iter = mSpaceData->begin(); iter != mSpaceData->end(); ++iter)
    {
        iter->second.destroy(getTracker(iter->first.space(),iter->first.object()));
    }
    mSpaceData->clear();
    mObjectHost->unregisterHostedObject(mInternalObjectReference);
}

struct HostedObject::PrivateCallbacks {
    static void disconnectionEvent(const HostedObjectWPtr&weak_thus,const SpaceID&sid, const String&reason) {
        std::tr1::shared_ptr<HostedObject>thus=weak_thus.lock();
        if (thus) {
            //FIXME: need to pass object reference here as well.
            assert(false);
            
            // SpaceDataMap::iterator where=thus->mSpaceData->find(sid);
            // if (where!=thus->mSpaceData->end()) {
            //     where->second.destroy(thus->getTracker(sid));
            //     thus->mSpaceData->erase(where);//FIXME do we want to back this up to the database first?
            // }
        }
    }

    static void connectionEvent(const HostedObjectWPtr&thus,
                                const SpaceID&sid,
                                Network::Stream::ConnectionStatus ce,
                                const String&reason) {
        
        if (ce!=Network::Stream::Connected) {
            disconnectionEvent(thus,sid,reason);
        }
    }
};

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


QueryTracker* HostedObject::getTracker(const SpaceID& space, const ObjectReference& oref) {
    SpaceDataMap::iterator it = mSpaceData->find(SpaceObjectReference(space,oref));
    if (it == mSpaceData->end()) return NULL;
    return it->second.tracker;
}

const QueryTracker* HostedObject::getTracker(const SpaceID& space, const ObjectReference& oref) const {
    SpaceDataMap::const_iterator it = mSpaceData->find(SpaceObjectReference(space,oref));
    if (it == mSpaceData->end()) return NULL;
    return it->second.tracker;
}

ProxyManagerPtr HostedObject::getProxyManager(const SpaceID& space, const ObjectReference& oref)
{
    SpaceDataMap::const_iterator it = mSpaceData->find(SpaceObjectReference(space,oref));
    if (it == mSpaceData->end())
    {
        std::cout<<"\n\nClean getProxyManager function so that it's apparent may get back null\n\n";
        std::cout<<"\n\nLooking for: "<<space<<"   "<<oref<<"\n\n";
        // SpaceDataMap::const_iterator twoIt = mSpaceData->begin();
        // // for (twoIt = mSpaceData->begin(); twoIt != mSpaceData->end(); ++twoIt)
        // // {
        // //     SpaceObjectReference tmp = twoIt->first;
        // //     std::cout<<"values: "<<tmp<<"\n";
        // // }
        // it = mSpaceData->insert(
        //     SpaceDataMap::value_type( SpaceObjectReference(space,oref), PerPresenceData(this, space,oref) )
        // ).first;
        ProxyManagerPtr returner;
        return returner;
        
    }
    return it->second.proxyManager;
}


n
void HostedObject::getSpaceObjRefs(SpaceObjRefSet& ss) const
{
    if (mSpaceData == NULL)
    {
        std::cout<<"\n\n\nCalling getSpaceObjRefs when not connected to any spaces.  This really shouldn't happen\n\n\n";
        assert(false);
    }

    SpaceDataMap::const_iterator smapIter;
    for (smapIter = mSpaceData->begin(); smapIter != mSpaceData->end(); ++smapIter)
    {
        std::cout<<"\nin getSpaceObjRefs.  This is a space obj:  "<<smapIter->second.space<<smapIter->second.object<<"\n";
        ss.insert(SpaceObjectReference(smapIter->second.space,smapIter->second.object));
    }
}


static ProxyObjectPtr nullPtr;
const ProxyObjectPtr &HostedObject::getProxyConst(const SpaceID &space, const ObjectReference& oref) const
{
    SpaceDataMap::const_iterator iter = mSpaceData->find(SpaceObjectReference(space,oref));
    if (iter == mSpaceData->end()) {
        return nullPtr;
    }
    return iter->second.mProxyObject;
}

ProxyObjectPtr HostedObject::getProxy(const SpaceID& space, const ObjectReference& oref)
{
    ProxyManagerPtr proxy_manager = getProxyManager(space,oref);
    ProxyObjectPtr  proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));
    return proxy_obj;
}

namespace {
bool myisalphanum(char c) {
    if (c>='a'&&c<='z') return true;
    if (c>='A'&&c<='Z') return true;
    if (c>='0'&&c<='9') return true;
    return false;
}
}


// attaches a script to the entity. This is like running 
// the script after the entity is initialized
// the entity should have been intialized
void HostedObject::attachScript(const String& script_name)
{
  if(!mObjectScript)
  {
      SILOG(oh,warn,"[OH] Ignored attachScript because script is not initialized for " << getUUID().toString() << "(internal id)");
      return;
  }
  mObjectScript->attachScript(script_name);
}

void HostedObject::initializeScript(const String& script, const ObjectScriptManager::Arguments &args, const std::string& fileScriptToAttach) {
    if (mObjectScript) {
        SILOG(oh,warn,"[OH] Ignored initializeScript because script already exists for " << getUUID().toString() << "(internal id)");
        return;
    }

    SILOG(oh,debug,"[OH] Creating a script object for " << getUUID().toString() << "(internal id)");
    
    static ThreadIdCheck scriptId=ThreadId::registerThreadGroup(NULL);
    assertThreadGroup(scriptId);
    if (!ObjectScriptManagerFactory::getSingleton().hasConstructor(script)) {
        bool passed=true;
        for (std::string::const_iterator i=script.begin(),ie=script.end();i!=ie;++i) {
            if (!myisalphanum(*i)) {
                if (*i!='-'&&*i!='_') {
                    passed=false;
                }
            }
        }
        if (passed) {
            mObjectHost->getScriptPluginManager()->load(script);
        }
    }
    ObjectScriptManager *mgr = ObjectScriptManagerFactory::getSingleton().getConstructor(script)("");
    if (mgr) {
        mObjectScript = mgr->createObjectScript(this->getSharedPtr(), args);
        if (fileScriptToAttach != "")
        {
            std::cout<<"\n\nAttaching script: "<<fileScriptToAttach<<"\n\n";
            mObjectScript->attachScript(fileScriptToAttach);
        }
    }
}

void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const UUID&object_uuid_evidence,
        PerPresenceData* ppd,
        const String& scriptFile,
        const String& scriptType)
{
    connect(spaceID, startingLocation, meshBounds, mesh, SolidAngle::Max, object_uuid_evidence,ppd);
}

void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const SolidAngle& queryAngle,
        const UUID&object_uuid_evidence,
        PerPresenceData* ppd,
        const String& scriptFile,
        const String& scriptType)
{
    if (spaceID == SpaceID::null())
        return;

    
    // Note: we always use Time::null() here.  The server will fill in the
    // appropriate value.  When we get the callback, we can fix this up.
    Time approx_server_time = Time::null();
    mObjectHost->connect(
        getSharedPtr(), spaceID,
        TimedMotionVector3f(approx_server_time, MotionVector3f( Vector3f(startingLocation.getPosition()), startingLocation.getVelocity()) ),
        TimedMotionQuaternion(approx_server_time,MotionQuaternion(startingLocation.getOrientation(),Quaternion(startingLocation.getAxisOfRotation(),startingLocation.getAngularSpeed()))),
        meshBounds,
        mesh,
        queryAngle,
        std::tr1::bind(&HostedObject::handleConnected, this, _1, _2, _3, _4, _5, _6, scriptFile,scriptType,ppd),
        std::tr1::bind(&HostedObject::handleMigrated, this, _1, _2, _3),
        std::tr1::bind(&HostedObject::handleStreamCreated, this, _1) 
    );

}


void HostedObject::addSimListeners(PerPresenceData*& pd, const std::list<String>& oh_sims,    std::vector<TimeSteppedSimulation*>& sims)
{
    std::cout<<"\n\nFIXME: defaulting objects into first space available in addSimListeners\n\n";
    SpaceID space = mObjectHost->getDefaultSpace();
    
    for(std::list<String>::const_iterator it = oh_sims.begin(); it != oh_sims.end(); it++)
        SILOG(cppoh,error,*it);

    pd = new PerPresenceData (this,space);

    ObjectHostProxyManagerPtr proxyManPtr = pd->getProxyManager();
    
    for(std::list<String>::const_iterator it = oh_sims.begin(); it != oh_sims.end(); it++)
    {
        String simName = *it;
        HO_LOG(info,String("[OH] Initializing ") + simName);
        
        TimeSteppedSimulation *sim =SimulationFactory::getSingleton().getConstructor ( simName ) ( mContext, proxyManPtr.get(), "" );
        if (!sim) {
            HO_LOG(error,String("Unable to load ") + simName + String(" plugin. The PATH environment variable is ignored, so make sure you have copied the DLLs from dependencies/ogre/bin/ into the current directory. Sorry about this!"));
            std::cerr << "Press enter to continue" << std::endl;
            fgetc(stdin);
            exit(0);
        }
        else {
            mObjectHost->addListener(sim);
            HO_LOG(info,String("Successfully initialized ") + simName);
            sims.push_back(sim);
        }
    }
}






void HostedObject::handleConnected(const SpaceID& space, const ObjectReference& obj, ServerID server, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& scriptFile, const String& scriptType, PerPresenceData* ppd)
{
    // We have to manually do what mContext->mainStrand->wrap( ... ) should be
    // doing because it can't handle > 5 arguments.
    mContext->mainStrand->post(
        std::tr1::bind(&HostedObject::handleConnectedIndirect, this, space, obj, server, loc, orient, bnds,scriptFile,scriptType, ppd)
    );
}


void HostedObject::handleConnectedIndirect(const SpaceID& space, const ObjectReference& obj, ServerID server, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& scriptFile, const String& scriptType, PerPresenceData* ppd)
{
    std::cout.flush();
    
    if (server == NullServerID) {
        HO_LOG(warning,"Failed to connect object (internal:" << mInternalObjectReference.toString() << ") to space " << space);
        return;
    }

    SpaceObjectReference self_objref(space, obj);
    if(mSpaceData->find(self_objref) == mSpaceData->end())
    {
        if (ppd != NULL)
        {
            ppd->populateSpaceObjRef(SpaceObjectReference(space,obj));
            mSpaceData->insert(
                SpaceDataMap::value_type(self_objref, *ppd)
            );
        }
        else
        {
            PerPresenceData toInsert(this,space,obj);
            mSpaceData->insert(
                SpaceDataMap::value_type(self_objref,PerPresenceData(this,space,obj))
            );
        }
    }
    
    // Convert back to local time
    TimedMotionVector3f local_loc(localTime(space, loc.updateTime()), loc.value());
    TimedMotionQuaternion local_orient(localTime(space, orient.updateTime()), orient.value());
    ProxyObjectPtr self_proxy = createProxy(self_objref, self_objref, URI(), mIsCamera, local_loc, local_orient, bnds);


    // Use to initialize PerSpaceData
    SpaceDataMap::iterator psd_it = mSpaceData->find(self_objref);
    PerPresenceData& psd = psd_it->second;
    initializePerSpaceData(psd, self_proxy);

    // Special case for camera
    ProxyCameraObjectPtr cam = std::tr1::dynamic_pointer_cast<ProxyCameraObject, ProxyObject>(self_proxy);
    if (cam)
        cam->attach(String(), 0, 0);


    //bind an odp port to listen for the begin scripting signal.  if have
    //receive the scripting signal for the first time, that means that we create
    //a JSObjectScript for this hostedobject
    bindODPPort(space,obj,Services::LISTEN_FOR_SCRIPT_BEGIN);

    //attach script callback;
    if (scriptFile != "")
    {
        ObjectScriptManager::Arguments script_args;  //these can just be empty;
        this->initializeScript(scriptType,script_args,scriptFile);
    }
    
}

void HostedObject::handleMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server)
{
    NOT_IMPLEMENTED(ho);
}

void HostedObject::handleStreamCreated(const SpaceObjectReference& spaceobj) {
    boost::shared_ptr<Stream<UUID> > sstStream = mObjectHost->getSpaceStream(spaceobj.space(), getUUID());

    if (sstStream != boost::shared_ptr<Stream<UUID> >() ) {
        sstStream->listenSubstream(OBJECT_PORT_LOCATION,
            std::tr1::bind(&HostedObject::handleLocationSubstream, this, spaceobj, _1, _2)
        );
        sstStream->listenSubstream(OBJECT_PORT_PROXIMITY,
            std::tr1::bind(&HostedObject::handleProximitySubstream, this, spaceobj, _1, _2)
        );
    }
}

void HostedObject::initializePerSpaceData(PerPresenceData& psd, ProxyObjectPtr selfproxy) {
    psd.initializeAs(selfproxy);
}

void HostedObject::disconnectFromSpace(const SpaceID &spaceID, const ObjectReference& oref)
{
    SpaceDataMap::iterator where;
    where=mSpaceData->find(SpaceObjectReference(spaceID, oref));
    if (where!=mSpaceData->end()) {
        where->second.destroy(getTracker(spaceID,oref));
        mSpaceData->erase(where);
    } else {
        SILOG(cppoh,error,"Attempting to disconnect from space "<<spaceID<<" when not connected to it...");
    }
}

bool HostedObject::route(Sirikata::Protocol::Object::ObjectMessage* msg) {
    DEPRECATED(); // We need a SpaceID in here
    assert(mSpaceData->size() != 0);

    SpaceID space = mSpaceData->begin()->first.space();
    
    return mObjectHost->send(getSharedPtr(), space, msg->source_port(), msg->dest_object(), msg->dest_port(), msg->payload());
}


//returns true if this is a script init message.  returns false otherwise
bool HostedObject::handleScriptInitMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData)
{
    if (dst.port() != Services::LISTEN_FOR_SCRIPT_BEGIN)
        return false;

    //I don't really know what this segment of code does.  I copied it from processRPC
    RoutableMessageBody msg;
    RoutableMessageBody outer_msg;
    outer_msg.ParseFromArray(bodyData.data(), bodyData.length());
    if (outer_msg.has_payload())
    {
        assert( outer_msg.message_size() == 0 );
        msg.ParseFromString(outer_msg.payload());
    }
    else
        msg = outer_msg;


    int numNames = msg.message_size();
    if (numNames <= 0)
    {
        // Invalid message!
        //was a poorly formatted message to the listen_for_script_begin port.
        //send back a protocol error.
        RoutableMessageHeader replyHeader;
        replyHeader.set_return_status(RoutableMessageHeader::PROTOCOL_ERROR);
        sendViaSpace(replyHeader, MemoryReference::null());
        return true;
    }


    //if any of the names match, then we're going to go ahead an create a script
    //for it.
    for (int i = 0; i < numNames; ++i)
    {
        std::string name = msg.message_names(i);
        MemoryReference body(msg.message_arguments(i));
        
        //means that we are supposed to create a new scripted object
        if (name == KnownMessages::INIT_SCRIPT)
            processInitScriptMessage(body);
    }

    //it was on the script init port, so it was a scripting init message
    return true;
}


//The processInitScriptSetup takes in a message body that we know should be an
//init script message (from checks in handleScriptInitMessage).  
//Does some additional checking on the message body, and then sets a few global
//variables and calls the object's initializeScript function
void HostedObject::processInitScriptMessage(MemoryReference& body)
{
    Protocol::ScriptingInit si;
    si.ParseFromArray(body.data(),body.size());
    
    if (si.has_script())
    {
        String script_type = si.script();
        ObjectScriptManager::Arguments script_args;
        if (si.has_script_args())
        {
            
            Protocol::StringMapProperty args_map = si.script_args();
            assert(args_map.keys_size() == args_map.values_size());
            for (int i = 0; i < args_map.keys_size(); ++i)
                script_args[ args_map.keys(i) ] = args_map.values(i);
        }
        mHasScript = true;
        mScriptType = script_type;
        mScriptArgs = script_args;
        initializeScript(script_type, script_args);
    }
}



bool HostedObject::handleEntityCreateMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData)
{
    //if the message isn't on the create_entity port, then it's good.
    //Otherwise, it's bad.
    if (dst.port() != Services::CREATE_ENTITY)
        return false;

    return true;
}



void HostedObject::receiveMessage(const SpaceID& space, const Protocol::Object::ObjectMessage* msg) {
    // Convert to ODP runtime format
    ODP::Endpoint src_ep(space, ObjectReference(msg->source_object()), msg->source_port());
    ODP::Endpoint dst_ep(space, ObjectReference(msg->dest_object()), msg->dest_port());


    std::cout<<"\n\nReceived message \n\n";
    
    
    // FIXME to transition to real ODP instead of ObjectMessageRouter +
    // ObjectMessageDispatcher, we need to allow the old route as well.  First
    // we check if we can use ObjectMessageDispatcher::dispatchMessage, and if
    // not, we allow it through to the long term solution, ODP::Service
    if (ObjectMessageDispatcher::dispatchMessage(*msg)) {
        // Successfully delivered using old method
        
        delete msg;
        return;
    }
    if (mDelegateODPService->deliver(src_ep, dst_ep, MemoryReference(msg->payload()))) {
        // if this was true, it got delivered
        
        delete msg;
    }
    else if (handleScriptInitMessage(src_ep,dst_ep,MemoryReference(msg->payload())))
    {
        //if this was true, then that means that it was an init script command,
        //and we dealt with it.
        delete msg;
    }
    else if (handleEntityCreateMessage(src_ep,dst_ep,MemoryReference(msg->payload())))
    {
        //if this was true, then that means that we got a
        //handleEntityCreateMessage, and tried to create a new entity
        delete msg;
    }
    else {
        SILOG(cppoh,debug,"[HO] Undelivered message from " << src_ep << " to " << dst_ep);
        delete msg;
    }

}



void HostedObject::sendViaSpace(const RoutableMessageHeader &hdrOrig, MemoryReference body) {
    //DEPRECATED(HostedObject);
    ///// MessageService::processMessage
    assert(hdrOrig.has_destination_object());
    assert(hdrOrig.has_destination_space());
    assert(hdrOrig.has_source_object());
    SpaceDataMap::iterator where=mSpaceData->find(SpaceObjectReference(hdrOrig.destination_space(), hdrOrig.source_object()));
    if (where!=mSpaceData->end()) {
        mObjectHost->send(getSharedPtr(), hdrOrig.destination_space(), hdrOrig.source_port(), hdrOrig.destination_object().getAsUUID(), hdrOrig.destination_port(), body);
    }
    assert(where!=mSpaceData->end());
}

void HostedObject::send(const RoutableMessageHeader &hdrOrig, MemoryReference body) {
    //DEPRECATED(HostedObject);
    assert(hdrOrig.has_destination_object());
    if (!hdrOrig.has_destination_space() || hdrOrig.destination_space() == SpaceID::null()) {
        DEPRECATED(HostedObject); // QueryTracker still causes this case
        RoutableMessageHeader hdr (hdrOrig);
        hdr.set_destination_space(SpaceID::null());
        hdr.set_source_object(ObjectReference(mInternalObjectReference));
        mObjectHost->processMessage(hdr, body);
        return;
    }
    sendViaSpace(hdrOrig, body);
}

void HostedObject::handleLocationSubstream(const SpaceObjectReference& spaceobj, int err, boost::shared_ptr< Stream<UUID> > s) {
    s->registerReadCallback( std::tr1::bind(&HostedObject::handleLocationSubstreamRead, this, spaceobj, s, new std::stringstream(), _1, _2) );
}

void HostedObject::handleProximitySubstream(const SpaceObjectReference& spaceobj, int err, boost::shared_ptr< Stream<UUID> > s) {
    std::stringstream** prevdataptr = new std::stringstream*;
    *prevdataptr = new std::stringstream();
    s->registerReadCallback( std::tr1::bind(&HostedObject::handleProximitySubstreamRead, this, spaceobj, s, prevdataptr, _1, _2) );
}

void HostedObject::handleLocationSubstreamRead(const SpaceObjectReference& spaceobj, boost::shared_ptr< Stream<UUID> > s, std::stringstream* prevdata, uint8* buffer, int length) {
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

void HostedObject::handleProximitySubstreamRead(const SpaceObjectReference& spaceobj, boost::shared_ptr< Stream<UUID> > s, std::stringstream** prevdataptr, uint8* buffer, int length) {
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
        ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(), ObjectReference(update.object())));
        if (!proxy_obj) continue;


        if (update.has_location()) {

            Sirikata::Protocol::TimedMotionVector update_loc = update.location();
            TimedMotionVector3f loc(localTime(space, update_loc.t()), MotionVector3f(update_loc.position(), update_loc.velocity()));

            proxy_obj->setLocation(loc);

            CONTEXT_OHTRACE(objectLoc,
                getUUID(),
                update.object(),
                loc
            );
        }

        if (update.has_orientation()) {
            Sirikata::Protocol::TimedMotionQuaternion update_orient = update.orientation();
            TimedMotionQuaternion orient(localTime(space, update_orient.t()), MotionQuaternion(update_orient.position(), update_orient.velocity()));
            proxy_obj->setOrientation(orient);
        }

        if (update.has_mesh()) {
          std::string mesh = update.mesh();
          ProxyMeshObject *meshObj = dynamic_cast<ProxyMeshObject*>(proxy_obj.get());

          if (meshObj && mesh != "") {
            //std::cout << "MESH UPDATE: " << mesh  << "!!!\n";
            meshObj->setMesh(Transfer::URI(mesh));
          }
          else if (mesh != ""){
            //std::cout << "MESH UPDATE but no proxy object!\n";
          }
        }
    }

    return true;
}

bool HostedObject::handleProximityMessage(const SpaceObjectReference& spaceobj, const std::string& payload) {
    
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
                getUUID(),
                addition.object(),
                true,
                loc
            );


            ProxyManagerPtr pmp = getProxyManager(spaceobj.space(),spaceobj.object());
            if (! pmp)
            {
                return true;
            }


            
            if (!getProxyManager(spaceobj.space(),spaceobj.object())->getProxyObject(proximateID)) {
                TimedMotionQuaternion orient(localTime(space, addition.orientation().t()), MotionQuaternion(addition.orientation().position(), addition.orientation().velocity()));

                URI meshuri;
                if (addition.has_mesh()) meshuri = URI(addition.mesh());

                // FIXME use weak_ptr instead of raw
                BoundingSphere3f bnds = addition.bounds();
                ProxyObjectPtr proxy_obj = createProxy(proximateID, spaceobj, meshuri, false, loc, orient, bnds);
            }
            else {
                ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(), spaceobj.object());
                ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(),
                        ObjectReference(addition.object())));

                if (!proxy_obj) continue;

              ProxyMeshObject *mesh = dynamic_cast<ProxyMeshObject*>(proxy_obj.get());
              if (mesh) {
                mesh->setMesh( Transfer::URI(addition.mesh()));
              }
            }
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);

            ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(), spaceobj.object());
            ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(),
                                                                     ObjectReference(removal.object())));
            if (!proxy_obj) continue;

            ProxyMeshObject *mesh = dynamic_cast<ProxyMeshObject*>(proxy_obj.get());
            if (mesh) {
              mesh->setMesh( Transfer::URI(""));
            }
            else continue;

            //std::cout << "Proximity removal: " << removal.object().toString()  <<  "\n";

            CONTEXT_OHTRACE(prox,
                getUUID(),
                removal.object(),
                false,
                TimedMotionVector3f()
            );
        }
    }

    return true;
}


ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const URI& meshuri, bool is_camera, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmq, const BoundingSphere3f& bs)
{
    ProxyObjectPtr returner = buildProxy(objref,owner_objref,meshuri,is_camera);
    returner->setLocation(tmv);
    returner->setOrientation(tmq);
    returner->setBounds(bs);

    
    if (!is_camera && meshuri)
    {
        ProxyMeshObject *mesh = dynamic_cast<ProxyMeshObject*>(returner.get());
        if (mesh)
        {
            mesh->setMesh(meshuri);
        }
    }

    return returner;
}

//should only be called from within createProxy functions.  Otherwise, will not
//initilize position and quaternion correctly
ProxyObjectPtr HostedObject::buildProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const URI& meshuri, bool is_camera)
{
    //ProxyManagerPtr proxy_manager = getProxyManager(objref.space(),
    //objref.object());
    ProxyManagerPtr proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());

    if (!proxy_manager)
    {
        //mSpaceData->insert(SpaceDataMap::value_type( objref, PerPresenceData(this, objref.space(),objref.object()) ));
        //proxy_manager = getProxyManager(objref.space(), objref.object());
        mSpaceData->insert(SpaceDataMap::value_type( owner_objref, PerPresenceData(this, owner_objref.space(),owner_objref.object()) ));
        proxy_manager = getProxyManager(owner_objref.space(), owner_objref.object());
    }

    
    ProxyObjectPtr proxy_obj;

    if (is_camera) proxy_obj = ProxyObject::construct<ProxyCameraObject>
                       (proxy_manager.get(), objref, getSharedPtr(), owner_objref);
    else proxy_obj = ProxyObject::construct<ProxyMeshObject>(proxy_manager.get(), objref, getSharedPtr(), owner_objref);

// The call to createObject must occur before trying to do any other
// operations so that any listeners will be set up.
    proxy_manager->createObject(proxy_obj, getTracker(objref.space(),objref.object()));
    return proxy_obj;
}

ProxyManagerPtr HostedObject::getDefaultProxyManager(const SpaceID& space)
{
    std::cout<<"\n\nINCORRECT in getDefaultProxyManager: should try to match object!!!\n\n";
    ObjectReference oref = mSpaceData->begin()->first.object();
    return  getProxyManager(space, oref);
}

ProxyObjectPtr HostedObject::getDefaultProxyObject(const SpaceID& space)
{
    std::cout<<"\n\nINCORRECT in getDefaultProxyObject: should try to match object!!!\n\n";
    ObjectReference oref = mSpaceData->begin()->first.object();
    return  getProxy(space, oref);
}




// Identification
// SpaceObjectReference HostedObject::id(const SpaceID& space) const
// {
//     SpaceDataMap::const_iterator it = mSpaceData->find(space);
//     if (it == mSpaceData->end()) return SpaceObjectReference::null();
//     return it->second.id();
// }



// ODP::Service Interface
ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref, ODP::PortID port) {
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

void HostedObject::registerDefaultODPHandler(const ODP::MessageHandler& cb) {
    mDelegateODPService->registerDefaultODPHandler(cb);
}

void HostedObject::registerDefaultODPHandler(const ODP::OldMessageHandler& cb) {
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
    return mObjectHost->send(getSharedPtr(), source_ep.space(), source_ep.port(), dest_ep.object().getAsUUID(), dest_ep.port(), payload);
}



void HostedObject::requestLocationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f& loc)
{
    sendLocUpdateRequest(space, oref,&loc, NULL, NULL, NULL);
}

//only update the position of the object, leave the velocity and orientation unaffected
void HostedObject::requestPositionUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& pos)
{
    Vector3f curVel = requestCurrentVelocity(space,oref);
    TimedMotionVector3f tmv (currentSpaceTime(space),MotionVector3f(pos,curVel));
    requestLocationUpdate(space,oref,tmv);
}

//only update the velocity of the object, leave the position and the orientation
//unaffected
void HostedObject::requestVelocityUpdate(const SpaceID& space,  const ObjectReference& oref, const Vector3f& vel)
{
    Vector3f curPos = Vector3f(requestCurrentPosition(space,oref));
    TimedMotionVector3f tmv (currentSpaceTime(space),MotionVector3f(curPos,vel));
    requestLocationUpdate(space,oref,tmv);
}

//send a request to update the orientation of this object
void HostedObject::requestOrientationDirectionUpdate(const SpaceID& space, const ObjectReference& oref,const Quaternion& quat)
{
    Quaternion curQuatVel = requestCurrentQuatVel(space,oref);
    TimedMotionQuaternion tmq (Time::local(),MotionQuaternion(quat,curQuatVel));
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
    Location curLoc = proxy_obj->extrapolateLocation(Time::local());
    return curLoc.getOrientation();
}

Quaternion HostedObject::requestCurrentOrientationVel(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return proxy_obj->getOrientationSpeed();
}

void HostedObject::requestOrientationVelocityUpdate(const SpaceID& space, const ObjectReference& oref, const Quaternion& quat)
{
    Quaternion curOrientQuat = requestCurrentOrientation(space,oref);
    TimedMotionQuaternion tmq (Time::local(),MotionQuaternion(curOrientQuat,quat));
    requestOrientationUpdate(space, oref,tmq);
}



//goes into proxymanager and gets out the current location of the presence
//associated with
Vector3d HostedObject::requestCurrentPosition (const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj  = getProxy(space,oref);

    //BFTM_FIXME: need to decide whether want the extrapolated position or last
    //known position.  (Right now, we're going with last known position.)


    Location curLoc = proxy_obj->extrapolateLocation(Time::local());
    Vector3d currentPosition = curLoc.getPosition();
    return currentPosition;
}


bool HostedObject::requestMeshUri(const SpaceID& space, const ObjectReference& oref, Transfer::URI& tUri)
{
    
    ProxyManagerPtr proxy_manager = getProxyManager(space,oref);
    ProxyObjectPtr  proxy_obj     = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));


    //this cast does not work.
    ProxyMeshObjectPtr proxy_mesh_obj = std::tr1::dynamic_pointer_cast<ProxyMeshObject,ProxyObject> (proxy_obj);

    
    if (proxy_mesh_obj )
        return false;

    tUri =  proxy_mesh_obj->getMesh();
    return true;
}


Vector3f HostedObject::requestCurrentVelocity(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return (Vector3f)proxy_obj->getVelocity();
}

void HostedObject::requestOrientationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionQuaternion& orient) {
    sendLocUpdateRequest(space, oref, NULL, &orient, NULL, NULL);
}

void HostedObject::requestBoundsUpdate(const SpaceID& space, const ObjectReference& oref, const BoundingSphere3f& bounds) {
    sendLocUpdateRequest(space, oref,NULL, NULL, &bounds, NULL);
}

void HostedObject::requestScaleUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& toScaleTo)
{
    std::cout<<"\n\nThe requestScaleUpdate function does not work\n\n";
    assert(false);
}

bool HostedObject::requestCurrentScale(const SpaceID& space, const ObjectReference& oref, Vector3f& scaler)
{
    std::cout<<"\n\nThe requestCurrentScale function does not work\n\n";
    assert(false);
    return false;
}


void HostedObject::requestMeshUpdate(const SpaceID& space, const ObjectReference& oref, const String& mesh)
{
    sendLocUpdateRequest(space, oref, NULL, NULL, NULL, &mesh);
}

void HostedObject::sendLocUpdateRequest(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh) {
    // Generate and send an update to Loc
    Protocol::Loc::Container container;
    Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
    if (loc != NULL) {
        Protocol::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t( spaceTime(space, loc->updateTime()) );
        requested_loc.set_position(loc->position());
        requested_loc.set_velocity(loc->velocity());
    }
    if (orient != NULL) {
        Protocol::ITimedMotionQuaternion requested_orient = loc_request.mutable_orientation();
        requested_orient.set_t( spaceTime(space, orient->updateTime()) );
        requested_orient.set_position(orient->position());
        requested_orient.set_velocity(orient->velocity());
    }
    if (bounds != NULL)
        loc_request.set_bounds(*bounds);
    if (mesh != NULL)
    {
        loc_request.set_mesh(*mesh);
    }

    std::string payload = serializePBJMessage(container);

    std::cout<<"\n\nBFTM: fix.  Calling getUUID, may make changes to the wrong oref in sendLocUpdateRequest\n\n";
    boost::shared_ptr<Stream<UUID> > spaceStream = mObjectHost->getSpaceStream(space, getUUID());
    if (spaceStream != boost::shared_ptr<Stream<UUID> >()) {
        boost::shared_ptr<Connection<UUID> > conn = spaceStream->connection().lock();
        assert(conn);

        conn->datagram( (void*)payload.data(), payload.size(), OBJECT_PORT_LOCATION,
            OBJECT_PORT_LOCATION, NULL);
    }
}


Location HostedObject::getLocation(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy = getProxy(space, oref);
    assert(proxy);
    Location currentLoc = proxy->globalLocation(currentSpaceTime(space));
    return currentLoc;
}

//BFTM_FIXME: need to actually write this function (called by ObjectHost's updateAddressable).
void HostedObject::updateAddressable()
{
    std::cout<<"\n\n\n";
    std::cout<<"BFTM: need to actually write this function";
    std::cout<<"\n\n\n";
    assert(false);
}


}
