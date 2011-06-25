
#include "JSVisibleStruct.hpp"
#include "../EmersonScript.hpp"
#include "JSPresenceStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include "../JSLogging.hpp"
#include <v8.h>
#include "Util.hpp"

namespace Sirikata {
namespace JS {

JSVisibleStruct::JSVisibleStruct(JSProxyPtr addParams)
 : JSPositionListener(addParams)
{
}


JSVisibleStruct::~JSVisibleStruct()
{
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


v8::Handle<v8::Value> JSVisibleStruct::toString()
{
    v8::HandleScope handle_scope;  //for garbage collection.

    std::string s = getSporef().toString();
    v8::Local<v8::String> ret = v8::String::New(s.c_str(), s.length());
    v8::Persistent<v8::String> pret = v8::Persistent<v8::String>::New(ret);
    return pret;
}


//just prints out associated space object reference, position, and whether still visible
v8::Handle<v8::Value> JSVisibleStruct::printData()
{
    std::cout << "Printing Object Reference :" << getSporef().toString() << "\n";
    std::cout << "Still visible : "<<getStillVisible() <<"\n";
    std::cout<<JSPositionListener::getPosition()<<"\n\n";

    return v8::Undefined();
}



void JSVisibleStruct::visibleWeakReferenceCleanup(v8::Persistent<v8::Value> containsVisStruct, void* otherArg)
{
    if (!containsVisStruct->IsObject())
    {
        JSLOG(error, "Error when cleaning up jsvisible.  Received a visible to clean up that wasn't an object.");
        return;
    }

    v8::Handle<v8::Object> vis = containsVisStruct->ToObject();

    //check to make sure object has adequate number of fields.
    CHECK_INTERNAL_FIELD_COUNT(vis,JSVisible,VISIBLE_FIELD_COUNT);

    //delete typeId, and return if have incorrect params for type id
    DEL_TYPEID_AND_CHECK(vis,jsvis,VISIBLE_TYPEID_STRING);

    String err = "Potential error when cleaning up jsvisible.  Could not decode visible struct.";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(vis,err);

    //jsvis now contains a pointer to a jsvisible struct, which we can now
    //delete.
    delete jsvis;
    containsVisStruct.Dispose();

    JSLOG(insane,"Freeing memory for jsvisible.");
}



}//js namespace
}//sirikata namespace
