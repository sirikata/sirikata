#ifndef _SIRIKATA_JS_CONTEXT_STRUCT_HPP_
#define _SIRIKATA_JS_CONTEXT_STRUCT_HPP_

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>
#include "JSFakerootStruct.hpp"
#include "../JSSystemNames.hpp"
#include "../JSObjects/JSFields.hpp"

namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;
class JSPresenceStruct;


struct JSContextStruct
{
    JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence, SpaceObjectReference* home, bool sendEveryone, bool recvEveryone, bool proxQueries);
    ~JSContextStruct();

    

    v8::Handle<v8::Value> struct_executeScript(v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv);
    v8::Handle<v8::Value> struct_getAssociatedPresPosition();
    v8::Handle<v8::Value> struct_sendHome(String& toSend);

    void jsscript_print(const String& msg);
    void presenceDied();


    static JSContextStruct* decodeContextStruct(v8::Handle<v8::Value> toDecode, String& errorMsg);
    
    
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

    //this is the context that any and all objects will be run in.
    v8::Persistent<v8::Context> mContext; 
    
};


}//end namespace js
}//end namespace sirikata

#endif
