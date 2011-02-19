#ifndef __SIRIKATA_JS_CONTEXT_STRUCT_HPP__
#define __SIRIKATA_JS_CONTEXT_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>
#include "JSFakerootStruct.hpp"
#include "../JSSystemNames.hpp"
#include "../JSObjects/JSFields.hpp"
#include "JSSuspendable.hpp"


namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;
class JSPresenceStruct;
class JSTimerStruct;

struct JSContextStruct : public JSSuspendable
{
    JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence, SpaceObjectReference* home, bool sendEveryone, bool recvEveryone, bool proxQueries, v8::Handle<v8::ObjectTemplate> contGlobTempl);
    ~JSContextStruct();


    static JSContextStruct* getJSContextStruct();
    static JSContextStruct* decodeContextStruct(v8::Handle<v8::Value> toDecode, String& errorMsg);

    
    v8::Handle<v8::Value>  struct_executeScript(v8::Handle<v8::Function> funcToCall,const v8::Arguments& args);
    v8::Handle<v8::Value>  struct_getAssociatedPresPosition();
    v8::Handle<v8::Value>  struct_sendHome(String& toSend);

    virtual v8::Handle<v8::Value> suspend();
    virtual v8::Handle<v8::Value> resume();
    virtual v8::Handle<v8::Value> clear();
    
    v8::Handle<v8::Value>  struct_suspendContext();
    v8::Handle<v8::Value>  struct_resumeContext();
    
    v8::Handle<v8::Object> struct_getFakeroot();


    

    void struct_registerSuspendable   (JSSuspendable* toRegister);
    void struct_deregisterSuspendable (JSSuspendable* toDeregister);
    
    
    void jsscript_print(const String& msg);
    void presenceDied();



    
    
    //********data
    
    JSObjectScript* jsObjScript;

    //a pointer to the local presence that is associated with this context.  for
    //instance, when you call getPosition on the fakeroot object, you actually
    //end up returning the position of this presence.  you send messages from
    //this presence, etc.
    JSPresenceStruct* associatedPresence; 

    //homeObject is the spaceobjectreference of an object that you can always
    //send messages to regardless of permissions
    SpaceObjectReference* mHomeObject;  

    //a pointer to the fakeroot struct that will be used as a system-like object
    //inside of the context.  
    JSFakerootStruct* mFakeroot;

    //struct associated with the Emerson util object that is associated with this
    //context.  
    JSUtilObjStruct* mUtil;
    
    //this is the context that any and all objects will be run in.
    v8::Persistent<v8::Context> mContext;

    bool isSuspended;
    
    //all associated objects that will need to be suspended/resumed if context
    //is suspended/resumed
    SuspendableMap associatedSuspendables;
};

typedef std::vector<JSContextStruct*> ContextVector;

}//end namespace js
}//end namespace sirikata

#endif
