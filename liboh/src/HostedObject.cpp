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
#include <sirikata/proxyobject/LightInfo.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/oh/ObjectHost.hpp>

#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/util/KnownMessages.hpp>
#include <sirikata/core/util/KnownScriptTypes.hpp>

#include <sirikata/core/util/ThreadId.hpp>
#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/core/odp/Exceptions.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>
#include <list>
#include <vector>
#include <sirikata/proxyobject/SimulationFactory.hpp>
#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Frame.pbj.hpp"
#include "Protocol_JSMessage.pbj.hpp"

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

    
    mDelegateODPService = new ODP::DelegateService(
        std::tr1::bind(
            &HostedObject::createDelegateODPPort, this,
            _1, _2, _3
        )
    );

    mHasScript = false;
}

void HostedObject::runGraphics(const SpaceObjectReference& sporef, const String& simName)
{
    TimeSteppedSimulation* sim = NULL;
    
    SpaceDataMap::iterator psd_it = mSpaceData->find(sporef);
    if (psd_it == mSpaceData->end())
    {
        std::cout<<"\n\nERROR: should have an entry for this space object reference in spacedatamap.  Aborting.\n\n";
        assert(false);
    }
    
    PerPresenceData& pd =  psd_it->second;
    pd.mProxyObject->setCamera(true);
    addSimListeners(pd,simName,sim);

    if (sim != NULL)
    {
        HO_LOG(info, "Adding simulation to context");
        mContext->add(sim);
    }

    // Special case for camera
    //if (self_proxy->isCamera())
    pd.mProxyObject->attach(String(), 0, 0);

    
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


ProxyManagerPtr HostedObject::getProxyManager(const SpaceID& space, const ObjectReference& oref)
{
    SpaceDataMap::const_iterator it = mSpaceData->find(SpaceObjectReference(space,oref));
    if (it == mSpaceData->end())
    {
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

static ProxyManagerPtr nullManPtr;
ProxyObjectPtr HostedObject::getProxy(const SpaceID& space, const ObjectReference& oref)
{
    ProxyManagerPtr proxy_manager = getProxyManager(space,oref);
    if (proxy_manager == nullManPtr)
        return nullPtr;
    
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

void HostedObject::initializeScript(const String& script, const ObjectScriptManager::Arguments &args, const std::string& fileScriptToAttach)
{
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
				mObjectScript->scriptTypeIs(script);
				mObjectScript->scriptFileIs(fileScriptToAttach);
        if (fileScriptToAttach != "")
        {
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
    connect(spaceID, startingLocation, meshBounds, mesh, SolidAngle::Max, object_uuid_evidence,ppd,scriptFile,scriptType);
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

        std::tr1::bind(&HostedObject::handleConnected, this, _1, _2, _3, _4, _5, _6, _7, scriptFile,scriptType,ppd),

        std::tr1::bind(&HostedObject::handleMigrated, this, _1, _2, _3),
        std::tr1::bind(&HostedObject::handleStreamCreated, this, _1) 
    );

}



void HostedObject::addSimListeners(PerPresenceData& pd, const String& simName,TimeSteppedSimulation*& sim)
{
    SpaceID space = mObjectHost->getDefaultSpace();

    SILOG(cppoh,error,simName);

    ObjectHostProxyManagerPtr proxyManPtr = pd.getProxyManager();

    HO_LOG(info,String("[OH] Initializing ") + simName);
    sim =SimulationFactory::getSingleton().getConstructor ( simName ) ( mContext, proxyManPtr.get(), "" );
    if (!sim)
    {
        HO_LOG(error,String("Unable to load ") + simName + String(" plugin. The PATH environment variable is ignored, so make sure you have copied the DLLs from dependencies/ogre/bin/ into the current directory. Sorry about this!"));
        std::cerr << "Press enter to continue" << std::endl;
        fgetc(stdin);
        exit(0);
    }
    else
    {
        mObjectHost->addListener(sim);
        HO_LOG(info,String("Successfully initialized ") + simName);
    }
}




void HostedObject::handleConnected(const SpaceID& space, const ObjectReference& obj, ServerID server, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& mesh, const String& scriptFile, const String& scriptType, PerPresenceData* ppd)
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
        std::tr1::bind(&HostedObject::handleConnectedIndirect, this, space, obj, server, loc, orient, bnds,mesh,scriptFile,scriptType, ppd)
    );
}


void HostedObject::handleConnectedIndirect(const SpaceID& space, const ObjectReference& obj, ServerID server, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& mesh, const String& scriptFile, const String& scriptType, PerPresenceData* ppd)
{
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
    ProxyObjectPtr self_proxy = createProxy(self_objref, self_objref, Transfer::URI(mesh), mIsCamera, local_loc, local_orient, bnds);

    // Use to initialize PerSpaceData
    SpaceDataMap::iterator psd_it = mSpaceData->find(self_objref);
    PerPresenceData& psd = psd_it->second;
    initializePerSpaceData(psd, self_proxy);


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
    SSTStreamPtr sstStream = mObjectHost->getSpaceStream(spaceobj.space(), getUUID());

    if (sstStream != SSTStreamPtr() ) {
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
        mSpaceData->erase(where);
    } else {
        SILOG(cppoh,error,"Attempting to disconnect from space "<<spaceID<<" when not connected to it...");
    }
}


// //returns true if this is a script init message.  returns false otherwise
// bool HostedObject::handleScriptInitMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData)
// {
//     if (dst.port() != Services::LISTEN_FOR_SCRIPT_BEGIN)
//         return false;

//     //I don't really know what this segment of code does.  I copied it from processRPC
//     RoutableMessageBody msg;
//     RoutableMessageBody outer_msg;
//     outer_msg.ParseFromArray(bodyData.data(), bodyData.length());
//     if (outer_msg.has_payload())
//     {
//         assert( outer_msg.message_size() == 0 );
//         msg.ParseFromString(outer_msg.payload());
//     }
//     else
//         msg = outer_msg;


//     int numNames = msg.message_size();
//     if (numNames <= 0)
//     {
//         // Invalid message!
//         //was a poorly formatted message to the listen_for_script_begin port.
//         //send back a protocol error.
//         RoutableMessageHeader replyHeader;
//         replyHeader.set_return_status(RoutableMessageHeader::PROTOCOL_ERROR);
//         sendViaSpace(replyHeader, MemoryReference::null());
//         return true;
//     }

//     //if any of the names match, then we're going to go ahead an create a script
//     //for it.
//     for (int i = 0; i < numNames; ++i)
//     {
//         std::string name = msg.message_names(i);
//         MemoryReference body(msg.message_arguments(i));
        
//         //means that we are supposed to create a new scripted object
//         if (name == KnownMessages::INIT_SCRIPT)
//             processInitScriptMessage(body);
//     }

//     //it was on the script init port, so it was a scripting init message
//     return true;
// }



//returns true if this is a script init message.  returns false otherwise
bool HostedObject::handleScriptInitMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData)
{
    if (dst.port() != Services::LISTEN_FOR_SCRIPT_BEGIN)
        return false;

    //I don't really know what this segment of code does.  I copied it from
    //processRPC
    Sirikata::JS::Protocol::ScriptingInit sMessage;


    bool parsed = sMessage.ParseFromArray(bodyData.data(),bodyData.size());
    
    if (! parsed)
        return false;

    if (!((sMessage.has_script()) && (sMessage.has_messager())))
    {
        return false;
    }

    String scriptType = sMessage.script();
    String messager   = sMessage.messager();

    if (messager != KnownMessages::INIT_SCRIPT)
        return false;
    
    if (scriptType == ScriptTypes::JS_SCRIPT_TYPE)
    {
        mHasScript = true;
        mScriptType = scriptType;

        ObjectScriptManager::Arguments scriptargs;
        initializeScript(scriptType,scriptargs,"");
    }

    return true;
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

        if (update.has_mesh())
        {
          std::string mesh = update.mesh();
          if (mesh != "")
              proxy_obj->setMesh(Transfer::URI(mesh));
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

                Transfer::URI meshuri;
                if (addition.has_mesh()) meshuri = Transfer::URI(addition.mesh());

                // FIXME use weak_ptr instead of raw
                BoundingSphere3f bnds = addition.bounds();
                ProxyObjectPtr proxy_obj = createProxy(proximateID, spaceobj, meshuri, false, loc, orient, bnds);
            }
            else
            {

                ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(), spaceobj.object());
                ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(),
                        ObjectReference(addition.object())));

                if (!proxy_obj)
                    continue;
                
                proxy_obj->setMesh(Transfer::URI(addition.mesh()));
            }
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);


            ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space(), spaceobj.object());
            ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(),
                                                                     ObjectReference(removal.object())));
            if (!proxy_obj) continue;

            proxy_obj->setMesh(Transfer::URI(""));


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


ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri, bool is_camera, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmq, const BoundingSphere3f& bs)
{
    ProxyObjectPtr returner = buildProxy(objref,owner_objref,meshuri,is_camera);
    returner->setLocation(tmv);
    returner->setOrientation(tmq);
    returner->setBounds(bs);

    
    if (!is_camera)
    {
        if(meshuri)
        {
            returner->setMesh(meshuri);
        }
    }
    return returner;
}

//should only be called from within createProxy functions.  Otherwise, will not
//initilize position and quaternion correctly
ProxyObjectPtr HostedObject::buildProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri, bool is_camera)
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

    
    ProxyObjectPtr proxy_obj(new ProxyObject (proxy_manager.get(),objref,getSharedPtr(),owner_objref));// = ProxyObject::construct(proxy_manager.get(),objref,getSharedPtr(),owner_objref);
    proxy_obj->setCamera(is_camera);

    
    // if (is_camera) proxy_obj = ProxyObject::construct<ProxyCameraObject>
    //                    (proxy_manager.get(), objref, getSharedPtr(), owner_objref);
    
    // else proxy_obj = ProxyObject::construct<ProxyMeshObject>(proxy_manager.get(), objref, getSharedPtr(), owner_objref);

// The call to createObject must occur before trying to do any other
// operations so that any listeners will be set up.
    proxy_manager->createObject(proxy_obj);
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


//apparently, will always return true now.  even if camera.
bool HostedObject::requestMeshUri(const SpaceID& space, const ObjectReference& oref, Transfer::URI& tUri)
{
    
    ProxyManagerPtr proxy_manager = getProxyManager(space,oref);
    ProxyObjectPtr  proxy_obj     = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));

    tUri = proxy_obj->getMesh();
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


    SSTStreamPtr spaceStream = mObjectHost->getSpaceStream(space, getUUID());
    if (spaceStream != SSTStreamPtr()) {
        SSTConnectionPtr conn = spaceStream->connection().lock();
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
    if(mObjectScript)
    {
        mObjectScript->updateAddressable();
    }
}

void HostedObject::persistToFile(std::ofstream& fp)
{
  SpaceObjRefSet ss;

  getSpaceObjRefs(ss);

  SpaceObjRefSet::iterator it = ss.begin();

  for(; it != ss.end(); it++)
  {
      HostedObject::EntityState* es = getEntityState((*it).space(), (*it).object()); 
      es->persistToFile(fp);
  }
}


void HostedObject::EntityState::persistToFile(std::ofstream& fp)
{
   /* We need to persist the things that are not part of the state. Like the meta state definitions. so,we are writing this file over and over again */

   fp << "\"" << objType << "\"" << "," << "\"" << subType << "\"" << "," << "\"" << name << "\"" << ",";
   /* Persist each field separated by a comma */
   /* persist the position  */

	 fp << pos.x << "," << pos.y << "," << pos.z << ",";


	 /* persist the quaternion.. How do it do it? I don't have x,y,z,w */
	 fp << orient.x << "," << orient.y << "," << orient.z << "," << orient.w << "," ;

	 /* persist the velocity */
	 
	 fp << vel.x << "," << vel.y << "," << vel.z << "," ;

	 /* persist the rotation */

	 fp << rot.x << "," << rot.y << "," << rot.z << ",";

	 /* persist the angular speed*/

	 fp << angular_speed << ",";

	 /*  persist the mesh url*/

	 fp << "\"" << mesh << "\""  << "," ;
   
	 
	 /* persist the scale */

	 fp << scale << ",";


	 /* persist the object id */

	 fp << objectID << ",";
	 
	 
	 fp << script_type << ",";


   fp << script_file << std::endl;
}


HostedObject::EntityState* HostedObject::getEntityState(const SpaceID& space, const ObjectReference& oref)
{

    HostedObject::EntityState* es = new HostedObject::EntityState();
    ProxyObjectPtr poptr = getProxy(space, oref);


    
    Location loc = getLocation(space, oref);
    es->objType = "mesh";
    es->subType = "graphiconly";
  
    // FIXME : HostedObject does not take the name of the entity into account right now after reading from the scene file.
    es->name = "unknown";
    es->pos = loc.getPosition();
    es->orient = loc.getOrientation();
    es->vel = loc.getVelocity();
    es->rot = loc.getAxisOfRotation();	
    es->angular_speed = loc.getAngularSpeed(); 


    if (poptr == nullPtr)
        assert (false);

    std::cout<<"\n\n";
    std::cout<<poptr->getMesh();
    std::cout<<"\n\n";
    std::cout.flush();
    
    es->mesh = poptr->getMesh().toString();



    /* Get Scale from the Bounding Sphere. Scale is the radius of this sphere */
    es->scale = poptr->getBounds().radius(); 
    es->objectID = oref.toString(); 
  	
    if(mObjectScript)
    {
        es->script_type = mObjectScript->scriptType();
        es->script_file = mObjectScript->scriptFile();
    }
    return es;

}





}
