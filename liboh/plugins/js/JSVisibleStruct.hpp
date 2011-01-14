


#ifndef _SIRIKATA_JS_VISIBLE_STRUCT_HPP_
#define _SIRIKATA_JS_VISIBLE_STRUCT_HPP_

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>

namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;

struct JSVisibleStruct
{
    JSVisibleStruct(JSObjectScript* parent, const SpaceObjectReference& whatsVisible, const SpaceObjectReference& toWhom, bool visibleCurrently, const Vector3d& currentPosition)
     : jsObjScript(parent),
       whatIsVisible(new SpaceObjectReference(whatsVisible)),
       visibleToWhom(new SpaceObjectReference( toWhom)),
       stillVisible(new bool(visibleCurrently)),
       mPosition(new Vector3d(currentPosition))
    {
    }
    
    
    ~JSVisibleStruct()
    {
        //do not delete jsObjScript: someone else is responsible for that.
        delete whatIsVisible;
        delete stillVisible;
        delete mPosition;
    }

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
