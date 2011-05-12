
#include "JSVisibleStruct.hpp"
#include "../JSObjectScript.hpp"
#include "../JSObjects/JSFields.hpp"
#include "../JSLogging.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {


JSVisibleStruct::JSVisibleStruct(JSObjectScript* parent, const SpaceObjectReference& whatsVisible, const SpaceObjectReference& toWhom,VisAddParams* addParams)
 : JSPositionListener(parent,addParams),
   stillVisible(new bool(false))
{
    if ((addParams != NULL) && (addParams->mIsVisible != NULL))
        *stillVisible = *addParams->mIsVisible;

    JSPositionListener::setListenTo(&whatsVisible,&toWhom);

    //only register as pos listener if still visible is true
    *stillVisible = JSPositionListener::registerAsPosAndMeshListener();
}

v8::Handle<v8::Value> JSVisibleStruct::struct_getAllData()
{
    v8::HandleScope handle_scope;
    v8::Handle<v8::Object> returner = v8::Object::New();
    returner->Set(v8::String::New("sporef"), struct_getSporefListeningTo());
    returner->Set(v8::String::New("sporefFrom"), struct_getSporefListeningFrom());
    returner->Set(v8::String::New("pos"), struct_getPosition());
    returner->Set(v8::String::New("vel"), struct_getVelocity());
    returner->Set(v8::String::New("orient"), struct_getOrientation());
    returner->Set(v8::String::New("orientVel"), struct_getOrientationVel());
    returner->Set(v8::String::New("scale"), struct_getScale());
    returner->Set(v8::String::New("mesh"), struct_getMesh());
    
    returner->Set(v8::String::New("posTime"),struct_getTransTime());
    returner->Set(v8::String::New("orientTime"), struct_getOrientTime());

    return handle_scope.Close(returner);
}



JSVisibleStruct::~JSVisibleStruct()
{
    //do not delete jsObjScript: someone else is responsible for that.
    jsObjScript->deRegisterVisibleStruct(this);
    delete stillVisible;
}


//if can decode senderVal as a visible struct, returns the pointer to that
//visible struct.  Otherwise, returns null.  the error message field allows
//decodeVisible to pass back any error messages to be thrown from the methods
//associated with JSVisible in JSObjects.
JSVisibleStruct* JSVisibleStruct::decodeVisible(v8::Handle<v8::Value> senderVal, std::string& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.

    if (! senderVal->IsObject())
    {
        errorMessage += "Error in decode of visible object.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> senderObject = senderVal->ToObject();

    //now check internal field count

    if (senderObject->InternalFieldCount() != VISIBLE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of visible object.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsVisibleStruct field
    v8::Local<v8::External> wrapJSVisibleObj;
    wrapJSVisibleObj = v8::Local<v8::External>::Cast(senderObject->GetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD));
    void* ptr = wrapJSVisibleObj->Value();

    JSVisibleStruct* returner;
    returner = static_cast<JSVisibleStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of visible object.  Internal field of object given cannot be casted to a JSVisibleStruct.";

    return returner;
}


void JSVisibleStruct::notifyNotVisible()
{
    JSLOG(insane,"Visible struct for object " << *sporefToListenTo <<" is no longer visible to "<<*sporefToListenFrom);

    *stillVisible = false;
    JSPositionListener::deregisterAsPosAndMeshListener();
}

void JSVisibleStruct::notifyVisible()
{
    //if you were already visible, do nothing.  you should be registered okay.
    if (*stillVisible == false)
    {
        JSLOG(insane,"Visible struct for object " << *sporefToListenTo <<" is now visible to "<<*sporefToListenFrom);
        *stillVisible = true;
        JSPositionListener::registerAsPosAndMeshListener();
    }
}

v8::Handle<v8::Value> JSVisibleStruct::toString()
{
    v8::HandleScope handle_scope;  //for garbage collection.

    std::string s = sporefToListenTo->toString();
    v8::Local<v8::String> ret = v8::String::New(s.c_str(), s.length());
    v8::Persistent<v8::String> pret = v8::Persistent<v8::String>::New(ret);
    return pret;
}


//just prints out associated space object reference, position, and whether still visible
v8::Handle<v8::Value> JSVisibleStruct::printData()
{
    std::cout << "Printing Object Reference :" << sporefToListenTo->toString() << "\n";
    std::cout << "Still visible : "<<*stillVisible<<"\n";
    std::cout<<JSPositionListener::getPosition()<<"\n\n";

    return v8::Undefined();
}



bool JSVisibleStruct::getStillVisibleCPP()
{
    return *stillVisible;
}


v8::Handle<v8::Value> JSVisibleStruct::getStillVisible()
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(*stillVisible);
}


v8::Handle<v8::Value> JSVisibleStruct::checkEqual(JSVisibleStruct* jsvis)
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(  *sporefToListenTo == *(jsvis->getToListenTo()));
}






}//js namespace
}//sirikata namespace
