
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





JSPositionListener::JSPositionListener(JSProxyPtr _jpp)
 : jpp(_jpp)
{
}


JSPositionListener::~JSPositionListener()
{
}

void JSPositionListener::setSharedProxyDataPtr( JSProxyPtr _jpp)
{
    jpp = _jpp;
}

v8::Handle<v8::Value> JSPositionListener::struct_getAllData()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getAllData);
    
    v8::HandleScope handle_scope;
    v8::Handle<v8::Object> returner = v8::Object::New();
    returner->Set(v8::String::New("sporef"), struct_getSporef());
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

v8::Handle<v8::Value> JSPositionListener::struct_getSporef()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getSporef);
    return v8::String::New(getSporef().toString().c_str());
}

SpaceObjectReference JSPositionListener::getSporef()
{
    return jpp->sporefToListenTo;
}

String JSPositionListener::getMesh()
{

    return jpp->mMesh;
}

v8::Handle<v8::Value> JSPositionListener::struct_getMesh()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getMesh);
    return v8::String::New(jpp->mMesh.c_str(),jpp->mMesh.size());
}

bool JSPositionListener::getStillVisible()
{
    return jpp->emerScript->isVisible(jpp->sporefToListenTo);
}

v8::Handle<v8::Value> JSPositionListener::struct_getStillVisible()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getStillVisible);
    return v8::Boolean::New(getStillVisible());
}

String JSPositionListener::getPhysics()
{
    return jpp->mPhysics;
}

v8::Handle<v8::Value> JSPositionListener::struct_getPhysics()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getVisible);
    return v8::String::New(jpp->mPhysics.c_str(),jpp->mPhysics.size());
}


Vector3f JSPositionListener::getPosition()
{
    return jpp->mLocation.position(jpp->emerScript->getHostedTime());
}
Vector3f JSPositionListener::getVelocity()
{
    return jpp->mLocation.velocity();
}

Quaternion JSPositionListener::getOrientationVelocity()
{
    return jpp->mOrientation.velocity();
}

Quaternion JSPositionListener::getOrientation()
{
    return jpp->mOrientation.position(jpp->emerScript->getHostedTime());
}


BoundingSphere3f JSPositionListener::getBounds()
{
    return jpp->mBounds;
}

v8::Handle<v8::Value> JSPositionListener::struct_getPosition()
{
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(getPosition,curContext);
    return CreateJSResult(curContext,getPosition());
}

v8::Handle<v8::Value> JSPositionListener::struct_getTransTime()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getTranstime);
    uint64 transTime = jpp->mLocation.updateTime().raw();
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
    CHECK_JPP_INIT_THROW_V8_ERROR(getOrientTime);
    
    uint64 orientTime = jpp->mOrientation.updateTime().raw();
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



v8::Handle<v8::Value> JSPositionListener::wrapSporef(SpaceObjectReference sporef)
{
    return v8::String::New(sporef.toString().c_str());
}


v8::Handle<v8::Value> JSPositionListener::struct_checkEqual(JSPositionListener* jpl)
{
    CHECK_JPP_INIT_THROW_V8_ERROR(checkEqual);
    bool returner = (getSporef() == jpl->getSporef());
    return v8::Boolean::New(returner);
}


v8::Handle<v8::Value>JSPositionListener::struct_getVelocity()
{
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(getVelocity,curContext);
    return CreateJSResult(curContext,getVelocity());
}

v8::Handle<v8::Value> JSPositionListener::struct_getOrientationVel()
{
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(getOrientationVel,curContext);
    return CreateJSResult(curContext,getOrientationVelocity());
}

v8::Handle<v8::Value> JSPositionListener::struct_getOrientation()
{
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(getOrientationVel,curContext);
    return CreateJSResult(curContext,getOrientation());
}

v8::Handle<v8::Value> JSPositionListener::struct_getScale()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getScale);
    return v8::Number::New(getBounds().radius());
}

v8::Handle<v8::Value> JSPositionListener::struct_getDistance(const Vector3d& distTo)
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getDistance);
    Vector3d curPos = Vector3d(getPosition());
    double distVal = (distTo - curPos).length();

    return v8::Number::New(distVal);
}


} //namespace JS
} //namespace Sirikata
