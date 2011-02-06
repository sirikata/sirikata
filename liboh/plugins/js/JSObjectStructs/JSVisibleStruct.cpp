
#include "JSVisibleStruct.hpp"
#include "../JSObjectScript.hpp"
#include "../JSObjects/JSFields.hpp"
#include <v8.h>
#include "JSWatchable.hpp"

namespace Sirikata {
namespace JS {


v8::Handle<v8::Value> JSVisibleStruct::dist(Vector3d* distTo)
{
    v8::HandleScope handle_scope;  //for garbage collection.
    setFlag();
    
    if (distTo == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in call to dist of jsvisible object in jsvisiblestruct.cpp.  Must provide a vector to dist function.")) );


    float dist = (*mPosition - *distTo).length();

    return v8::Number::New(dist);
}




JSVisibleStruct::JSVisibleStruct(JSObjectScript* parent, const SpaceObjectReference& whatsVisible, const SpaceObjectReference& toWhom, bool visibleCurrently, const Vector3d& currentPosition)
 : JSWatchable(),
   jsObjScript(parent),
   whatIsVisible(new SpaceObjectReference(whatsVisible)),
   visibleToWhom(new SpaceObjectReference( toWhom)),
   stillVisible(new bool(visibleCurrently)),
   mPosition(new Vector3d(currentPosition))
{
}



JSVisibleStruct::~JSVisibleStruct()
{
    //do not delete jsObjScript: someone else is responsible for that.
    delete whatIsVisible;
    delete stillVisible;
    delete mPosition;
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
    std::cout << "internal field count is = " << senderObject->InternalFieldCount() << "\n\n";

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


v8::Handle<v8::Value> JSVisibleStruct::returnProxyPosition()
{
    setFlag();
    return jsObjScript->returnProxyPosition(whatIsVisible,visibleToWhom,mPosition);
}


v8::Handle<v8::Value> JSVisibleStruct::toString()
{
    v8::HandleScope handle_scope;  //for garbage collection.

    std::string s = whatIsVisible->toString();
    v8::Local<v8::String> ret = v8::String::New(s.c_str(), s.length());
    v8::Persistent<v8::String> pret = v8::Persistent<v8::String>::New(ret); 
    return pret;
}


//just prints out associated space object reference, position, and whether still visible
v8::Handle<v8::Value> JSVisibleStruct::printData()
{
    std::cout << "Printing Object Reference :" << whatIsVisible->toString() << "\n";
    std::cout << "Still visible : "<<*stillVisible<<"\n";
    if (*stillVisible)
        jsObjScript->updatePosition(whatIsVisible,visibleToWhom,mPosition);

    std::cout<<*mPosition<<"\n\n";
    
    return v8::Undefined();
}


v8::Handle<v8::Value> JSVisibleStruct::visibleSendMessage (std::string& msgToSend)
{
    //actually send the message to the entity
    jsObjScript->sendMessageToEntity(whatIsVisible,visibleToWhom,msgToSend);
    return v8::Undefined();
}


v8::Handle<v8::Value> JSVisibleStruct::getStillVisible()
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(*stillVisible);
}


v8::Handle<v8::Value> JSVisibleStruct::checkEqual(JSVisibleStruct* jsvis)
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(  *whatIsVisible == *(jsvis->whatIsVisible));
}



}//js namespace
}//sirikata namespace
