

#ifndef __SIRIKATA_JS_VISIBLE_STRUCT_HPP__
#define __SIRIKATA_JS_VISIBLE_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>

namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;

struct JSVisibleStruct
{
    JSVisibleStruct(JSObjectScript* parent, const SpaceObjectReference& whatsVisible, const SpaceObjectReference& toWhom, bool visibleCurrently, const Vector3d& currentPosition);
    ~JSVisibleStruct();

    //for decoding
    static JSVisibleStruct* decodeVisible(v8::Handle<v8::Value> senderVal,std::string& errorMessage);

    
    //methods mapped to javascript's visible object 
    v8::Handle<v8::Value> returnProxyPosition();
    v8::Handle<v8::Value> toString();
    v8::Handle<v8::Value> printData();
    v8::Handle<v8::Value> getStillVisible();
    v8::Handle<v8::Value> visibleSendMessage (std::string& msgToSend);
    v8::Handle<v8::Value> checkEqual(JSVisibleStruct* jsvis);
    
    
    //data
    JSObjectScript* jsObjScript;
    SpaceObjectReference* whatIsVisible;
    SpaceObjectReference* visibleToWhom;
    bool* stillVisible;
    Vector3d* mPosition;
};


}//end namespace js
}//end namespace sirikata

#endif
