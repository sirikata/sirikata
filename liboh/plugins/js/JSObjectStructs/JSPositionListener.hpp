#ifndef __SIRIKATA_JS_POSITION_LISTENER_STRUCT_HPP__
#define __SIRIKATA_JS_POSITION_LISTENER_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>


namespace Sirikata {
namespace JS {

class JSProxyData;


//note: only position and isConnected will actually set the flag of the watchable
struct JSPositionListener 
{
    friend class JSSerializer;
    friend class JSVisibleStruct;
    friend class JSPresenceStruct;

public:
    virtual ~JSPositionListener();

    virtual Vector3f     getPosition();
    virtual Vector3f     getVelocity();
    virtual Quaternion   getOrientation();
    virtual Quaternion   getOrientationVelocity();
    virtual BoundingSphere3f   getBounds();
    virtual String getMesh();
    virtual String getPhysics();
    virtual bool getStillVisible();

    
    virtual v8::Handle<v8::Value> struct_getPosition();
    virtual v8::Handle<v8::Value> struct_getVelocity();
    virtual v8::Handle<v8::Value> struct_getOrientation();
    virtual v8::Handle<v8::Value> struct_getOrientationVel();
    virtual v8::Handle<v8::Value> struct_getScale();
    virtual v8::Handle<v8::Value> struct_getMesh();
    virtual v8::Handle<v8::Value> struct_getPhysics();
    virtual v8::Handle<v8::Value> struct_getTransTime();
    virtual v8::Handle<v8::Value> struct_getOrientTime();
    virtual v8::Handle<v8::Value> struct_getSporef();
    virtual v8::Handle<v8::Value> struct_getStillVisible();

    virtual v8::Handle<v8::Value> struct_getAllData();
    virtual v8::Handle<v8::Value> struct_checkEqual(JSPositionListener* jpl);
    
    virtual v8::Handle<v8::Value> struct_getDistance(const Vector3d& distTo);

    //simple accessors for sporef fields
    SpaceObjectReference getSporef();


    /**
       This call mostly exists for presences, which don't know their sporefs
       (and hence their proxy data ptrs) until after they've connected to the
       world.  JSPresenceStruct can set it here.
     */
    void setSharedProxyDataPtr(    std::tr1::shared_ptr<JSProxyData>_jpp);
    

protected:
    v8::Handle<v8::Value> wrapSporef(SpaceObjectReference sporef);
    std::tr1::shared_ptr<JSProxyData> jpp;

    
private:

    //private constructor.  Can only be made through serializer,
    //JSVisibleStruct, or JSPresenceStruct.
    JSPositionListener(    std::tr1::shared_ptr<JSProxyData> _jpp);
};



//Throws an error if jpp has not yet been initialized.
#define CHECK_JPP_INIT_THROW_V8_ERROR(funcIn)\
{\
    if (!jpp)\
    {\
        JSLOG(error,"Error in jspositionlistener.  Position proxy was not set."); \
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling " #funcIn ".  Proxy ptr was not set.")));\
    }\
}
    

//Throws an error if not in context.
//funcIn specifies which function is asking passErrorChecks, and gets printed in
//an error message if call fails.
//If in context, returns current context in con.
#define JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(funcIn,con)\
    CHECK_JPP_INIT_THROW_V8_ERROR(funcIn);\
    if (!v8::Context::InContext())                  \
    {\
        JSLOG(error,"Error in jspositionlistener.  Was not in a context."); \
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling " #funcIn ".  Not currently within a context.")));\
    }\
    v8::Handle<v8::Context>con = v8::Context::GetCurrent();



}//end namespace js
}//end namespace sirikata

#endif
