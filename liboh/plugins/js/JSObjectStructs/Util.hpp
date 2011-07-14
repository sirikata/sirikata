
#ifndef __OBJECT_STRUCT_UTIL_HPP__
#define __OBJECT_STRUCT_UTIL_HPP__

namespace Sirikata{
namespace JS{


/**
   @param {v8::Handle<v8::Object>} obj The object whose internal type fields we're checking
   @param {String} struct The struct we're checking the number of internal
   fields for.
   @param {int} Number of internal fields that should be in object
 */
#define CHECK_INTERNAL_FIELD_COUNT(obj,struct,shouldBeFieldCount,optionalReturn) \
{\
    if(obj->InternalFieldCount() != shouldBeFieldCount)\
    {\
        JSLOG(error, "Error when cleaning up " #struct ".  Has incorrect number of internal fields.");\
        return optionalReturn;                                          \
    }\
}


/**
   @param {v8::Handle<v8::Object>} obj The object whose internal type fields we're checking
   @param {String} struct The type of struct we're trying to delete the type id
   for
   @param {const char*} Character string that we want to match type to
 */
#define DEL_TYPEID_AND_CHECK(obj,struct,shouldBeTypeID,optionalReturn)  \
{                                                                   \
    v8::Local<v8::Value> typeidVal = obj->GetInternalField(TYPEID_FIELD);   \
    if (typeidVal.IsEmpty())                                            \
    {                                                                   \
        JSLOG(error, "Error when cleaning up " #struct ".  No value in type id."); \
        return optionalReturn;                                          \
    }                                                                   \
    if(typeidVal->IsNull() || typeidVal->IsUndefined())                 \
    {                                                                   \
        JSLOG(error, "Error when cleaning up " #struct ".  Null or undefined type id."); \
        return optionalReturn;                                          \
    }                                                                   \
                                                                        \
    v8::Local<v8::External> wrapped  = v8::Local<v8::External>::Cast(typeidVal);  \
    void* ptr = wrapped->Value();                                       \
    std::string* typeId = static_cast<std::string*>(ptr);               \
    if(typeId == NULL)                                                  \
    {                                                                   \
        JSLOG(detailed, "Potential error when cleaning up " #struct ".  Casting typeid to string produces null.");  \
        return optionalReturn;                                          \
    }                                                                   \
    if (*typeId != shouldBeTypeID)                                      \
    {                                                                   \
        JSLOG(error, "Error when cleaning up "#struct ".  Incorrect type id received: " + (*typeId)); \
        return optionalReturn;                                          \
    }                                                                   \
    delete typeId;                                                      \
    obj->SetInternalField(TYPEID_FIELD,v8::External::New(NULL));        \
}


}//close namespace js
}//close namespace sirikata

#endif
