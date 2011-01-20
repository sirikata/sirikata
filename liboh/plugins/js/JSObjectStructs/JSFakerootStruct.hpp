#ifndef __SIRIKATA_JS_FAKEROOT_STRUCT_HPP__
#define __SIRIKATA_JS_FAKEROOT_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>



namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;
class JSContextStruct;

struct JSFakerootStruct
{
    JSFakerootStruct(JSContextStruct* jscont, bool send, bool receive, bool prox);
    ~JSFakerootStruct();

    static JSFakerootStruct* decodeRootStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);

    //regular members
    v8::Handle<v8::Value> script_canSendMessage();
    v8::Handle<v8::Value> script_canRecvMessage();
    v8::Handle<v8::Value> script_canProx();
    v8::Handle<v8::Value> script_getPosition();
    v8::Handle<v8::Value> script_print(const String& msg);    
    v8::Handle<v8::Value> script_sendHome(String& toSend);
    
    //associated data 
    JSContextStruct* associatedContext;
    bool canSend, canRecv, canProx;
};


}//end namespace js
}//end namespace sirikata

#endif
