
#include <v8.h>
#include "JSPresenceStruct.hpp"
#include "../EmersonScript.hpp"
#include "../JSSerializer.hpp"
#include "../JSLogging.hpp"
#include "../JSObjects/JSVec3.hpp"
#include "../JSObjects/JSQuaternion.hpp"
#include "JSPresenceStruct.hpp"
#include <boost/lexical_cast.hpp>

namespace Sirikata {
namespace JS {


//this constructor is called when the presence associated
JSPositionListener::JSPositionListener(EmersonScript* script, VisAddParams* addParams)
 : emerScript(script),
   sporefToListenTo(NULL),
   sporefToListenFrom(NULL),
   hasRegisteredListener(false)
{
    if (addParams != NULL)
    {
        if (addParams->mLocation != NULL)
            mLocation    = *addParams->mLocation;
        if (addParams->mOrientation != NULL)
            mOrientation = *addParams->mOrientation;
        if (addParams->mBounds != NULL)
            mBounds      = *addParams->mBounds;
        if (addParams->mMesh != NULL)
            mMesh        = *addParams->mMesh;
        if (addParams->mPhysics != NULL)
            mPhysics     = *addParams->mPhysics;
    }
}


JSPositionListener::~JSPositionListener()
{
    if (hasRegisteredListener)
        deregisterAsPosAndMeshListener();


    if (sporefToListenTo != NULL)
        delete sporefToListenTo;
    if (sporefToListenFrom != NULL)
        delete sporefToListenFrom;

}

v8::Handle<v8::Object> JSPositionListener::struct_getAllData()
{
    v8::HandleScope handle_scope;
    v8::Handle<v8::Object> returner = v8::Object::New();
    returner->Set(v8::String::New("sporef"), struct_getSporefListeningTo());
    returner->Set(v8::String::New("sporefFrom"), struct_getSporefListeningFrom());
    returner->Set(v8::String::New("pos"), struct_getPosition());
    returner->Set(v8::String::New("vel"), struct_getVelocity());
    returner->Set(v8::String::New("orient"), struct_getOrientation());
    returner->Set(v8::String::New("orientVel"), struct_getOrientationVel());
    returner->Set(v8::String::New("scale"), struct_getScale());
    returner->Set(v8::String::New("mesh"), struct_getMesh());
    returner->Set(v8::String::New("physics"), struct_getPhysics());

    returner->Set(v8::String::New("posTime"),struct_getTransTime());
    returner->Set(v8::String::New("orientTime"), struct_getOrientTime());

    return handle_scope.Close(returner);
}




void JSPositionListener::destroyed()
{
    if (hasRegisteredListener)
    {
        hasRegisteredListener = false;
        deregisterAsPosAndMeshListener();
    }
}

void JSPositionListener::setListenTo(const SpaceObjectReference* objToListenTo,const SpaceObjectReference* objToListenFrom)
{
    //setting listenTo
    if (sporefToListenTo != NULL)
        *sporefToListenTo = *objToListenTo;
    else
    {
        if (objToListenTo != NULL)
            sporefToListenTo  =  new SpaceObjectReference(objToListenTo->space(),objToListenTo->object());
    }

    //setting listenFrom
    if (sporefToListenFrom != NULL)
        *sporefToListenFrom = *objToListenFrom;
    else
    {
        //may frequently get this parameter to be null from inheriting
        //JSPresenceStruct class.
        if (objToListenFrom != NULL)
            sporefToListenFrom =  new SpaceObjectReference(objToListenFrom->space(),objToListenFrom->object());
    }
}


bool JSPositionListener::registerAsPosAndMeshListener()
{
    if (hasRegisteredListener)
        return true;

    //initializes mLocation and mOrientation to correct starting values.
    if (sporefToListenTo == NULL)
    {
        JSLOG(error,"error in JSPositionListener.  Requesting to register as pos listener to null sporef.  Doing nothing");
        return false;
    }

    hasRegisteredListener = emerScript->registerPosAndMeshListener(sporefToListenTo,sporefToListenFrom,this,this,&mLocation,&mOrientation,&mBounds,&mMesh,&mPhysics);

    return hasRegisteredListener;
}



void JSPositionListener::deregisterAsPosAndMeshListener()
{
    if (!hasRegisteredListener)
        return;

    hasRegisteredListener =false;

    if (sporefToListenTo != NULL)
        emerScript->deRegisterPosAndMeshListener(sporefToListenTo,sporefToListenFrom,this,this);

}


SpaceObjectReference* JSPositionListener::getToListenTo()
{
    return sporefToListenTo;
}

SpaceObjectReference* JSPositionListener::getToListenFrom()
{
    return sporefToListenFrom;
}


//calls updateLocation on jspos, filling in mLocation, mOrientation, and mBounds
//for the newLocation,newOrientation, and newBounds of updateLocation field.
void JSPositionListener::updateOtherJSPosListener(JSPositionListener* jspos)
{
    jspos->updateLocation(mLocation,mOrientation,mBounds,*sporefToListenTo);
    jspos->onSetMesh(ProxyObjectPtr(),Transfer::URI(mMesh),*sporefToListenTo);
}

//from being a position listener, must define what to do when receive an updated location.
void JSPositionListener::updateLocation (const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds, const SpaceObjectReference& ider)
{
    mLocation    = newLocation;
    mOrientation = newOrient;
    mBounds      = newBounds;


    //if I received an updated location and I am associated with an object that
    //I am listeningFrom, then I should propagate this update to visible structs
    //that may not know about updates to this presence.
    if (sporefToListenFrom != NULL)
    {
        if (*sporefToListenFrom != SpaceObjectReference::null())
            emerScript->checkForwardUpdate(*sporefToListenFrom,newLocation,newOrient,newBounds);
    }

}

String JSPositionListener::getMesh()
{
    return mMesh;
}

v8::Handle<v8::Value> JSPositionListener::struct_getMesh()
{
    return v8::String::New(mMesh.c_str(),mMesh.size());
}


String JSPositionListener::getPhysics()
{
    return mPhysics;
}

v8::Handle<v8::Value> JSPositionListener::struct_getPhysics()
{
    return v8::String::New(mPhysics.c_str(),mPhysics.size());
}


void JSPositionListener::onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh, const SpaceObjectReference& ider)
{
    mMesh = newMesh.toString();

    if (sporefToListenFrom != NULL)
    {
        if (*sporefToListenFrom != SpaceObjectReference::null())
            emerScript->checkForwardUpdateMesh(*sporefToListenFrom,proxy,newMesh);
    }
}

void JSPositionListener::onSetScale (ProxyObjectPtr proxy, float32 newScale, const SpaceObjectReference& ider )
{
    mBounds = BoundingSphere3f(mBounds.center(),newScale);
    if (sporefToListenFrom != NULL)
    {
        if (*sporefToListenFrom != SpaceObjectReference::null())
            emerScript->checkForwardUpdate(*sporefToListenFrom,mLocation,mOrientation,mBounds);
    }
}

void JSPositionListener::onSetPhysics (ProxyObjectPtr proxy, const String& newphy, const SpaceObjectReference& ider )
{
    mPhysics = newphy;
}


Vector3f JSPositionListener::getPosition()
{
    return mLocation.position(emerScript->getHostedTime());
}
Vector3f JSPositionListener::getVelocity()
{
    return mLocation.velocity();
}

Quaternion JSPositionListener::getOrientationVelocity()
{
    return mOrientation.velocity();
}

Quaternion JSPositionListener::getOrientation()
{
    return mOrientation.position(emerScript->getHostedTime());
}


BoundingSphere3f JSPositionListener::getBounds()
{
    return mBounds;
}

v8::Handle<v8::Value> JSPositionListener::struct_getPosition()
{
    String errorMsg;
    if (! passErrorChecks(errorMsg,"getPosition"))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg.c_str(), errorMsg.size())));


    v8::Handle<v8::Context>curContext = v8::Context::GetCurrent();
    return CreateJSResult(curContext,getPosition());
}

v8::Handle<v8::Value> JSPositionListener::struct_getTransTime()
{
    String errorMsg;
    if (! passErrorChecks(errorMsg,"getTransTime"))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg.c_str(),errorMsg.size())));


    uint64 transTime = mLocation.updateTime().raw();
    String convertedString;

    try
    {
        convertedString = boost::lexical_cast<String>(transTime);
    }
    catch(boost::bad_lexical_cast & blc)
    {
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in getTransTime.  Cound not convert uint64 to string.")));
    }

    return v8::String::New(convertedString.c_str(), convertedString.size());
}


v8::Handle<v8::Value> JSPositionListener::struct_getOrientTime()
{
    String errorMsg;
    if (! passErrorChecks(errorMsg,"getOrientTime"))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg.c_str())));


    uint64 orientTime = mOrientation.updateTime().raw();
    String convertedString;

    try
    {
        convertedString = boost::lexical_cast<String>(orientTime);
    }
    catch(boost::bad_lexical_cast & blc)
    {
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in getOrientTime.  Cound not convert uint64 to string.")));
    }

    return v8::String::New(convertedString.c_str(),convertedString.size());
}


v8::Handle<v8::Value> JSPositionListener::struct_getSporefListeningTo()
{
    return wrapSporef(sporefToListenTo);
}
v8::Handle<v8::Value> JSPositionListener::struct_getSporefListeningFrom()
{
    return wrapSporef(sporefToListenFrom);
}

v8::Handle<v8::Value> JSPositionListener::wrapSporef(SpaceObjectReference* sporef)
{
    if (sporef == NULL)
    {
        SpaceObjectReference sp = SpaceObjectReference::null();
        return v8::String::New(sp.toString().c_str());
    }

    return v8::String::New(sporef->toString().c_str());
}



v8::Handle<v8::Value>JSPositionListener::struct_getVelocity()
{
    String errorMsg;
    if (! passErrorChecks(errorMsg,"getVelocity"))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg.c_str())));


    v8::Handle<v8::Context> curContext = v8::Context::GetCurrent();
    return CreateJSResult(curContext,getVelocity());
}

v8::Handle<v8::Value> JSPositionListener::struct_getOrientationVel()
{
    String errorMsg;
    if (! passErrorChecks(errorMsg,"getOrientationVel"))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg.c_str())));

    v8::Handle<v8::Context> curContext = v8::Context::GetCurrent();
    return CreateJSResult(curContext,getOrientationVelocity());
}

v8::Handle<v8::Value> JSPositionListener::struct_getOrientation()
{
    String errorMsg;
    if (! passErrorChecks(errorMsg,"getOrientation"))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg.c_str())));

    v8::Handle<v8::Context>curContext = v8::Context::GetCurrent();
    return CreateJSResult(curContext,getOrientation());
}

v8::Handle<v8::Value> JSPositionListener::struct_getScale()
{
    String errorMsg;
    if (! passErrorChecks(errorMsg,"getScale"))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg.c_str())));

    v8::Handle<v8::Context>curContext = v8::Context::GetCurrent();
    return v8::Number::New(getBounds().radius());
}

v8::Handle<v8::Value> JSPositionListener::struct_getDistance(const Vector3d& distTo)
{
    String errorMsg;
    if (! passErrorChecks(errorMsg,"getOrientation"))
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMsg.c_str())));

    Vector3d curPos = Vector3d(getPosition());
    double distVal = (distTo - curPos).length();

    v8::Handle<v8::Context>curContext = v8::Context::GetCurrent();
    return v8::Number::New(distVal);
}


bool JSPositionListener::passErrorChecks(String& errorMsg, const String& funcIn )
{
    if (sporefToListenTo == NULL)
    {
        errorMsg =  "Error when calling " + funcIn + ".  Did not specify who to listen to.";
        return false;
    }

    if (!v8::Context::InContext())
    {
        errorMsg = "Error when calling " + funcIn + ".  Not currently within a context.";
        return false;
    }

    return true;
}



} //namespace JS
} //namespace Sirikata
