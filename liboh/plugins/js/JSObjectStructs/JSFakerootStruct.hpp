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
    JSFakerootStruct(JSContextStruct* jscont, bool send, bool receive, bool prox,bool import);
    ~JSFakerootStruct();

    static JSFakerootStruct* decodeRootStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);

    //regular members
    v8::Handle<v8::Value> struct_canSendMessage();
    v8::Handle<v8::Value> struct_canRecvMessage();
    v8::Handle<v8::Value> struct_canProx();
    v8::Handle<v8::Value> struct_canImport();
    
    v8::Handle<v8::Value> struct_getPosition();
    v8::Handle<v8::Value> struct_print(const String& msg);    
    v8::Handle<v8::Value> struct_sendHome(const String& toSend);
    v8::Handle<v8::Value> struct_import(const String& toImportFrom);
    
    //associated data 
    JSContextStruct* associatedContext;
    bool canSend, canRecv, canProx,canImport;
};


}//end namespace js
}//end namespace sirikata

#endif
