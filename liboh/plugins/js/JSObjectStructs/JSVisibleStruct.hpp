

#ifndef __SIRIKATA_JS_VISIBLE_STRUCT_HPP__
#define __SIRIKATA_JS_VISIBLE_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>
#include "JSPositionListener.hpp"

namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class EmersonScript;
class JSVisibleManager;
class JSProxyData;


struct JSVisibleStruct : public JSPositionListener
{
public:
    virtual ~JSVisibleStruct();

    //for decoding
    static JSVisibleStruct* decodeVisible(v8::Handle<v8::Value> senderVal,std::string& errorMessage);

    //methods mapped to javascript's visible object
    v8::Handle<v8::Value> toString();
    v8::Handle<v8::Value> printData();

    /**
       Should be auto-called by v8 when the last emerson object with a reference
       to this struct has been garbage collected.
       Frees memory associated with visStruct field in
       containsVisStruct.  otherArg should be null.
     */
    static void visibleWeakReferenceCleanup(v8::Persistent<v8::Value> containsVisStruct, void* otherArg);

    
private:
    JSVisibleStruct( std::tr1::shared_ptr<JSProxyData>  jspd);
    friend class JSVisibleManager;

};

typedef std::vector<JSVisibleStruct*> JSVisibleVec;
typedef JSVisibleVec::iterator JSVisibleVecIter;



}//end namespace js
}//end namespace sirikata

#endif
