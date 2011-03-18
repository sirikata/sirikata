#ifndef __SIRIKATA_JS_CONTEXT_STRUCT_HPP__
#define __SIRIKATA_JS_CONTEXT_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>
#include "JSFakerootStruct.hpp"
#include "../JSSystemNames.hpp"
#include "../JSObjects/JSFields.hpp"
#include "JSSuspendable.hpp"
#include "JSEventHandlerStruct.hpp"
#include "../JSPattern.hpp"

namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;
class JSPresenceStruct;
class JSTimerStruct;
class JSUtilObjStruct;

struct JSContextStruct : public JSSuspendable
{
    JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence, SpaceObjectReference* home, bool sendEveryone, bool recvEveryone, bool proxQueries, bool canImport, v8::Handle<v8::ObjectTemplate> contGlobTempl);
    ~JSContextStruct();

    //looks in current context and returns the current context as pointer to
    //user.  if unsuccessful, return null.
    static JSContextStruct* getJSContextStruct();
    static JSContextStruct* decodeContextStruct(v8::Handle<v8::Value> toDecode, String& errorMsg);

    

    //contexts can be suspended (suspends all suspendables within the context)
    //resumed (takes all suspendables in context and calls resume on them)
    //or cleared (marks all objects native to this context and its chilren as
    //ready for garbage collection)  (also clears all suspendables in the context).
    virtual v8::Handle<v8::Value> suspend();
    virtual v8::Handle<v8::Value> resume();
    virtual v8::Handle<v8::Value> clear();
    
    v8::Handle<v8::Value>  struct_suspendContext();
    v8::Handle<v8::Value>  struct_resumeContext();

    //returns an object that contains the fakeroot/system object associated with
    //this context
    v8::Handle<v8::Object> struct_getFakeroot();


    //creates a new jseventhandlerstruct and wraps it in a js object
    //registers the jseventhandlerstruct both with this context and
    //jsobjectscript
    v8::Handle<v8::Value>  struct_makeEventHandlerObject(const PatternList& native_patterns,v8::Persistent<v8::Object> target_persist, v8::Persistent<v8::Function> cb_persist, v8::Persistent<v8::Object> sender_persist);
    

    //when add a handler, timer, when inside of context, want to register them.
    //That way, when call suspend on context and resume on context, can
    //suspend/resume them.
    void struct_registerSuspendable   (JSSuspendable* toRegister);
    void struct_deregisterSuspendable (JSSuspendable* toDeregister);
    
    
    void jsscript_print(const String& msg);
    void presenceDied();

    v8::Handle<v8::Value>  struct_executeScript(v8::Handle<v8::Function> funcToCall,const v8::Arguments& args);
    v8::Handle<v8::Value>  struct_getAssociatedPresPosition();
    v8::Handle<v8::Value>  struct_sendHome(const String& toSend);
    //string argument is the filename that we're trying to open and execute
    //contents of.
    v8::Handle<v8::Value>  struct_import(const String& toImportFrom);
    //requests jsobjscript to create an event handler in the context associated
    //wth jscontextstruct.  registers this handler as well through struct_registerSuspendable
    v8::Handle<v8::Value>  struct_makeEventHandlerObject(JSEventHandlerStruct* jsehs);


    //presStruct: who the messages that this context's fakeroot sends will
    //be from canMessage: who you can always send messages to.  sendEveryone creates
    //fakeroot that can send messages to everyone besides just who created you.
    //recvEveryone means that you can receive messages from everyone besides just
    //who created you.  proxQueries means that you can issue proximity queries
    //yourself, and latch on callbacks for them.  canImport means that you can
    //import files/libraries into your code.
    //creates a new context, and hangs the child into suspendables map.
    v8::Handle<v8::Value> struct_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool canImport,JSPresenceStruct* presStruct);


    //returns a v8 object that wraps the c++ presence
    v8::Local<v8::Object>  struct_getPresence();
    JSPresenceStruct* struct_getPresenceCPP();
    
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
typedef ContextVector::iterator ContextVecIter;


}//end namespace js
}//end namespace sirikata

#endif
