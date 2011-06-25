

#ifndef __SIRIKATA_JS_VISIBLE_STRUCT_HPP__
#define __SIRIKATA_JS_VISIBLE_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>
#include "../JSVisibleStructMonitor.hpp"
#include "JSPositionListener.hpp"

namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class EmersonScript;

struct JSVisibleStruct : public JSPositionListener
{
    friend class JSVisibleStructMonitor;

public:
    ~JSVisibleStruct();

    //for decoding
    static JSVisibleStruct* decodeVisible(v8::Handle<v8::Value> senderVal,std::string& errorMessage);

    //methods mapped to javascript's visible object
    bool getStillVisibleCPP();
    v8::Handle<v8::Value> toString();
    v8::Handle<v8::Value> printData();
    v8::Handle<v8::Value> getStillVisible();
    v8::Handle<v8::Value> checkEqual(JSVisibleStruct* jsvis);

private:

    JSVisibleStruct(EmersonScript* parent, const SpaceObjectReference& whatsVisible, const SpaceObjectReference& toWhom,VisAddParams* addParams);


    //these notifiers should only be called by the friend class JSVisibleStructMonitor
    void notifyVisible();
    void notifyNotVisible();

    //data
    bool* stillVisible;
};

typedef std::vector<JSVisibleStruct*> JSVisibleVec;
typedef JSVisibleVec::iterator JSVisibleVecIter;



}//end namespace js
}//end namespace sirikata

#endif
