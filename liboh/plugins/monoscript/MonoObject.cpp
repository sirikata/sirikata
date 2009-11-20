/*  Sirikata - Mono Embedding
 *  MonoObject.cpp
 *
 *  Copyright (c) 2009, Stanford University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "oh/Platform.hpp"
#include "util/Time.hpp"
#include "MonoObject.hpp"
#include "MonoClass.hpp"
#include "MonoException.hpp"
#include "MonoUtil.hpp"
#include "MonoAssembly.hpp"
#include "MonoDomain.hpp"
#include "MonoSystem.hpp"
#include "MonoArray.hpp"
#include "MonoConvert.hpp"
#include "MonoMethodLookupCache.hpp"
#include "util/BoundingInfo.hpp"
#include "util/UUID.hpp"
#include "util/SpaceObjectReference.hpp"
//#include "ResourceID.hpp"
#include <mono/metadata/appdomain.h>

//#define MONO_TRACK_PINNED

namespace Mono {

static std::set<guint32> monoPinnedHandles;
void reportPinnedObjects(){
#if 0
    Sirikata::Statistics::ID id = Sirikata::Statistics::getSingleton().registerStat("MonoPinnedHandle", Sirikata::Statistics::RecordAll);
    for(std::set<guint32>::iterator it = monoPinnedHandles.begin(); it != monoPinnedHandles.end(); it++) {
        MonoObject* obj = mono_gchandle_get_target(*it);
        Sirikata::String val = boost::lexical_cast<Sirikata::String>(*it) + ":" + boost::lexical_cast<Sirikata::String>(memory_usage_from_object_root(obj));
        Sirikata::Statistics::getSingleton().reportStat(id, val);
    }
#endif
}

// A GC controlled Mono Object, wrapped into a class
// so we can cleanly handle pinning only once per
// object.
class MonoGCObject {
public:
    MonoGCObject()
        : mObject(NULL),
         mGCHandle(0)
    {
    }

    MonoGCObject(MonoObject* obj, bool gcpin = true)
        : mObject(obj),
         mGCHandle(0)
    {
        if (gcpin)
            pin();
    }

    ~MonoGCObject() {
        if (mGCHandle) {
            mono_gchandle_free(mGCHandle);
#ifdef MONO_TRACK_PINNED
            monoPinnedHandles.erase(mGCHandle);
#endif
        }
        mObject = NULL;
    }

    MonoObject* object() const {
        return mObject;
    }

    void pin() {
        if (mObject != NULL) {
            mGCHandle = mono_gchandle_new(mObject, TRUE);
#ifdef MONO_TRACK_PINNED
            monoPinnedHandles.insert(mGCHandle);
#endif
        }
    }

private:
    MonoObject* mObject;
    guint32 mGCHandle;
};



//#####################################################################
// Function Object
//#####################################################################
Object::Object()
    : mGCObject(new MonoGCObject(NULL, false))
{
}

//#####################################################################
// Function Object
//#####################################################################
Object::Object(MonoObject* obj)
    : mGCObject(new MonoGCObject(obj, (obj != NULL)))
{
}

//#####################################################################
// Function Object
//#####################################################################
Object::Object(const Object& cpy)
    : mGCObject(cpy.mGCObject)
{
}

//#####################################################################
// Function ~Object
//#####################################################################
Object::~Object() {
}

//#####################################################################
// Function object
//#####################################################################
MonoObject* Object::object() const {
    return mGCObject->object();
}

//#####################################################################
// Function unsafeUnwrap
//#####################################################################
void* Object::unsafeUnwrap() const {
    Class wrapper_utils = Domain::root().getAssembly("Sirikata.Client").getClass("Sirikata.Client", "WrapperUtility");
    Object ptr_as_uint64_obj = wrapper_utils.send("NativePtrToUInt64", this->getField("swigCPtr"));
    Sirikata::uint64 ptr_as_uint64 = ptr_as_uint64_obj.unboxUInt64();
    return (void*)ptr_as_uint64;
}

//#####################################################################
// Function null
//#####################################################################
bool Object::null() const {
    return (mGCObject->object() == NULL);
}

//#####################################################################
// Function valuetype
//#####################################################################
bool Object::valuetype() const {
    return type().valuetype();
}

/*
void Object::listParentClasses() const {
    MonoClass* klass = mono_object_get_class(mGCObject->object());
    for(MonoClass* k = klass; k; k = mono_class_get_parent(k))
        printf("%s\n", mono_class_get_name(k));
}
*/

//#####################################################################
// Function printMethodSignature
//#####################################################################
static
void printMethodSignature(MonoMethod* method) {
    const char* method_name = mono_method_get_name(method);

    MonoMethodSignature* signature = mono_method_signature(method);

    MonoType* return_type = mono_signature_get_return_type(signature);
    char* return_type_name = mono_type_get_name(return_type);

    printf("%s %s(", return_type_name, method_name);

    gpointer param_iter = NULL;
    bool comma = false;
    while( MonoType* param_type = mono_signature_get_params(signature, &param_iter) ) {
        if (comma)
            printf(", ");
        comma = true;

        char* param_type_name = mono_type_get_name(param_type);
        printf("%s", param_type_name);
    }
    printf(")");
}

//#####################################################################
// Function listMethods
//#####################################################################
void Object::listMethods() const {
    MonoClass* klass = mono_object_get_class(mGCObject->object());
    gpointer method_iter = NULL;
    while( MonoMethod* method = mono_class_get_methods(klass, &method_iter) ) {
        printMethodSignature(method);
        printf("\n");
    }
}

//#####################################################################
// Function type
//#####################################################################
Class Object::type() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);
    return Class( mono_object_get_class(obj) );
}

//#####################################################################
// Function instanceOf
//#####################################################################
bool Object::instanceOf(Class klass) const {
    MonoObject* obj = mGCObject->object();
    if (obj == NULL)
        return false;

    return mono_object_isinst(obj, klass.mClass)!=NULL;
}

//#####################################################################
// Function assembly
//#####################################################################
Assembly Object::assembly() const {
    return type().assembly();
}

//#####################################################################
// Function referenceEquals
//#####################################################################
bool Object::referenceEquals(const Object& rhs) {
    return (mGCObject->object() == rhs.mGCObject->object());
}

guint32 Object::memoryUsage() const {
    if (null()) return 0;

    return memory_usage_from_object_root(mGCObject->object());
}

Object Object::send(const Sirikata::String& message) const {
    return send(NULL, message);
}

Object Object::send(const Sirikata::String& message, const Object& p1) const {
    return send(NULL, message, p1);
}

Object Object::send(const Sirikata::String& message, const Object& p1, const Object& p2) const {
    return send(NULL, message, p1, p2);
}

Object Object::send(const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3) const {
    return send(NULL, message, p1, p2, p3);
}

Object Object::send(const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3, const Object& p4) const {
    return send(NULL, message, p1, p2, p3, p4);
}

Object Object::send(const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3, const Object& p4, const Object& p5) const {
    return send(NULL, message, p1, p2, p3, p4, p5);
}

Object Object::send(const Sirikata::String& message, const std::vector<Object>& args) const {
    return send(NULL, message, args);
}

//#####################################################################
// Function send
//#####################################################################
Object Object::send(MethodLookupCache* cache, const Sirikata::String& message) const {
    MonoObject* args[] = { NULL };
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);
    return send_base(obj, message.c_str(), args, 0, cache);
}

//#####################################################################
// Function send
//#####################################################################
Object Object::send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1) const {
    MonoObject* args[] = { p1.mGCObject->object(), NULL };
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);
    return send_base(obj, message.c_str(), args, 1, cache);
}

//#####################################################################
// Function send
//#####################################################################
Object Object::send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2) const {
    MonoObject* args[] = { p1.mGCObject->object(), p2.mGCObject->object(), NULL };
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);
    return send_base(obj, message.c_str(), args, 2, cache);
}

//#####################################################################
// Function send
//#####################################################################
Object Object::send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3) const {
    MonoObject* args[] = { p1.mGCObject->object(), p2.mGCObject->object(), p3.mGCObject->object(), NULL };
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);
    return send_base(obj, message.c_str(), args, 3, cache);
}

//#####################################################################
// Function send
//#####################################################################
Object Object::send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3, const Object& p4) const {
    MonoObject* args[] = { p1.mGCObject->object(), p2.mGCObject->object(), p3.mGCObject->object(), p4.mGCObject->object(), NULL };
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);
    return send_base(obj, message.c_str(), args, 4, cache);
}

//#####################################################################
// Function send
//#####################################################################
Object Object::send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3, const Object& p4, const Object& p5) const {
    MonoObject* args[] = { p1.mGCObject->object(), p2.mGCObject->object(), p3.mGCObject->object(), p4.mGCObject->object(), p5.mGCObject->object(), NULL };
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);
    return send_base(obj, message.c_str(), args, 5, cache);
}

//#####################################################################
// Function send
//#####################################################################
Object Object::send(MethodLookupCache* cache, const Sirikata::String& message, const std::vector<Object>& args) const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    int nargs = args.size();
    MonoObject** margs = new MonoObject*[nargs];
    for(int i = 0; i < nargs; i++)
        margs[i] = args[i].mGCObject->object();

    Object rv = send_base(obj, message.c_str(), margs, nargs, cache);
    delete[] margs;
    return rv;
}

//#####################################################################
// Function getProperty
//#####################################################################
Object Object::getProperty(const Sirikata::String& propname) const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    MonoMethod* prop_get_method = mono_object_get_property_get(obj, propname.c_str());
    // FIXME asssert num arguments

    MonoObject* args[] = { NULL };
    return send_method(obj, prop_get_method, args, 0);
}

//#####################################################################
// Function getProperty
//#####################################################################
Object Object::getProperty(const Sirikata::String& propname, const Object& p1) const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    MonoMethod* prop_get_method = mono_object_get_property_get(obj, propname.c_str());
    // FIXME asssert num arguments

    MonoObject* args[] = { p1.mGCObject->object(), NULL };
    return send_method(obj, prop_get_method, args, 1);
}

//#####################################################################
// Function setProperty
//#####################################################################
void Object::setProperty(const Sirikata::String& propname, const Object& val) const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    MonoMethod* prop_set_method = mono_object_get_property_set(obj, propname.c_str());
    // FIXME assert num arguments

    MonoObject* args[] = { val.mGCObject->object(), NULL };
    send_method(obj, prop_set_method, args, 1);
}

//#####################################################################
// Function setProperty
//#####################################################################
void Object::setProperty(const Sirikata::String& propname, const Object& p1, const Object& val) const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    MonoMethod* prop_set_method = mono_object_get_property_set(obj, propname.c_str());
    // FIXME assert num arguments

    MonoObject* args[] = { p1.mGCObject->object(), val.mGCObject->object(), NULL };
    send_method(obj, prop_set_method, args, 2);
}

//#####################################################################
// Function getField
//#####################################################################
Object Object::getField(const Sirikata::String& fieldname) const {
    MonoObject* obj = object();
    checkNullReference(obj);

    MonoClass* klass = mono_object_get_class(obj);
    assert(klass != NULL);

    MonoClassField* field = mono_class_get_field_from_name( klass, fieldname.c_str() );
    if (field == NULL)
        throw Exception::MissingField(Class(klass), fieldname);

    return Object(mono_field_get_value_object(mono_object_get_domain(obj), field, obj));
}

//#####################################################################
// Function setField
//#####################################################################
void Object::setField(const Sirikata::String& fieldname, const Object& value) const {
    MonoObject* obj = object();
    checkNullReference(obj);

    MonoClass* klass = mono_object_get_class(obj);
    assert(klass != NULL);

    MonoClassField* field = mono_class_get_field_from_name( klass, fieldname.c_str() );
    if (field == NULL)
        throw Exception::MissingField(Class(klass), fieldname);

    MonoClass* receive_klass = mono_class_from_mono_type( mono_field_get_type(field) );
    mono_field_set_value(obj, field, object_to_raw(receive_klass, value.object()));
}

//#####################################################################
// Function unboxBoolean
//#####################################################################
bool Object::unboxBoolean() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_boolean_class());
    return *(bool*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxByte
//#####################################################################
guint8 Object::unboxByte() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_byte_class());
    return *(guint8*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxUInt16
//#####################################################################
guint16 Object::unboxUInt16() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_uint16_class());
    return *(guint16*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxUInt32
//#####################################################################
guint32 Object::unboxUInt32() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_uint32_class());
    return *(guint32*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxUInt64
//#####################################################################
Sirikata::uint64 Object::unboxUInt64() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_uint64_class());
    return *(Sirikata::uint64*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxSByte
//#####################################################################
Sirikata::int8 Object::unboxSByte() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_sbyte_class());
    return *(Sirikata::int8*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxInt16
//#####################################################################
Sirikata::int16 Object::unboxInt16() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_int16_class());
    return *(Sirikata::int16*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxInt32
//#####################################################################
Sirikata::int32 Object::unboxInt32() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_int32_class());
    return *(Sirikata::int32*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxInt64
//#####################################################################
Sirikata::int64 Object::unboxInt64() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_int64_class());
    return *(Sirikata::int64*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxSingle
//#####################################################################
float Object::unboxSingle() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_single_class());
    return *(float*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxDouble
//#####################################################################
double Object::unboxDouble() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_double_class());
    return *(double*)mono_object_unbox(mGCObject->object());
}

//#####################################################################
// Function unboxEnum
//#####################################################################
Sirikata::int32 Object::unboxEnum() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    MonoClass* klass = mono_object_get_class(obj);
    assert(mono_class_is_enum(klass));

    MonoClass* enumclass = mono_class_from_mono_type(mono_class_enum_basetype(klass));

    if (enumclass == mono_get_byte_class())
        return (Sirikata::int32)(*(Sirikata::uint8*)mono_object_unbox(obj));
    else if (enumclass == mono_get_uint16_class())
        return (Sirikata::int32)(*(Sirikata::uint16*)mono_object_unbox(obj));
    else if (enumclass == mono_get_uint32_class())
        return (Sirikata::int32)(*(Sirikata::uint32*)mono_object_unbox(obj));
    else if (enumclass == mono_get_uint64_class())
        return (Sirikata::int32)(*(Sirikata::uint64*)mono_object_unbox(obj));
    else if (enumclass == mono_get_sbyte_class())
        return (Sirikata::int32)(*(Sirikata::int8*)mono_object_unbox(obj));
    else if (enumclass == mono_get_int16_class())
        return (Sirikata::int32)(*(Sirikata::int16*)mono_object_unbox(obj));
    else if (enumclass == mono_get_int32_class())
        return (Sirikata::int32)(*(Sirikata::int32*)mono_object_unbox(obj));
    else if (enumclass == mono_get_int64_class())
        return (Sirikata::int32)(*(Sirikata::int64*)mono_object_unbox(obj));
    else
        assert(false);

    return 0;
}

//#####################################################################
// Function unboxIntPtr
//#####################################################################
void* Object::unboxIntPtr() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    Class wrapperutils = Domain::root().getAssembly("Sirikata.Client").getClass("Sirikata.Client", "WrapperUtility");
    Sirikata::uint64 ptr_as_uint64 = wrapperutils.send("NativePtrToUInt64", *this).unboxUInt64();
    return (void*)ptr_as_uint64;
}

//#####################################################################
// Function unboxHandleRef
//#####################################################################
void* Object::unboxHandleRef() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    Class wrapperutils = Domain::root().getAssembly("Sirikata.Client").getClass("Sirikata.Client", "WrapperUtility");
    Sirikata::uint64 ptr_as_uint64 = wrapperutils.send("NativePtrToUInt64", *this).unboxUInt64();
    return (void*)ptr_as_uint64;
}

//#####################################################################
// Function unboxString
//#####################################################################
Sirikata::String Object::unboxString() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    assert(mono_object_get_class(obj) == mono_get_string_class());

    char* utf8_str = mono_string_to_utf8( (MonoString*)obj );

    std::string result = utf8_str;

    g_free(utf8_str);

    return result;
}

void Object::unboxInPlaceByteArray(Sirikata::MemoryBuffer&result) const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    // FIXME assert klass = byte[]

    MonoArray* ary = (MonoArray*)obj;
    int result_length = mono_array_length(ary);

    result.resize((size_t)result_length);
    if (result_length) {
        memcpy( &result[0], mono_array_addr(ary,char,0), (size_t)result_length );
    }
}




Sirikata::MemoryBuffer Object::unboxByteArray() const {
    Sirikata::MemoryBuffer retval;
    unboxInPlaceByteArray(retval);
    return retval;
}

Sirikata::Vector3f Object::unboxVector3f() const {
    double x = getField("x").unboxDouble();
    double y = getField("y").unboxDouble();
    double z = getField("z").unboxDouble();
    return Sirikata::Vector3f(x,y,z);
}

Sirikata::Vector3d Object::unboxVector3d() const {
    double x = getField("x").unboxDouble();
    double y = getField("y").unboxDouble();
    double z = getField("z").unboxDouble();
    return Sirikata::Vector3d(x,y,z);
}

Sirikata::Quaternion Object::unboxQuaternion() const {
    double w = getField("w").unboxDouble();
    double x = getField("x").unboxDouble();
    double y = getField("y").unboxDouble();
    double z = getField("z").unboxDouble();
    return Sirikata::Quaternion(w,x,y,z,Sirikata::Quaternion::WXYZ());
}

Sirikata::Location Object::unboxLocation() const {
    return Sirikata::Location(
        getField("mPosition").unboxVector3d(),
        getField("mOrientation").unboxQuaternion(),
        getField("mVelocity").unboxVector3f(),
        getField("mAxisOfRotation").unboxVector3f(),
        getField("mAngularSpeed").unboxSingle()
    );
}

Sirikata::Vector4f Object::unboxColor() const {
    float r = getField("r").unboxSingle();
    float g = getField("g").unboxSingle();
    float b = getField("b").unboxSingle();
    float a = getField("a").unboxSingle();
    return Sirikata::Vector4f(r,g,b,a);
}

Sirikata::BoundingInfo Object::unboxBoundingInfo() const {
  MonoObject* obj = mGCObject->object();
  checkNullReference(obj);
    float x = getField("mMinx").unboxSingle();
    float y = getField("mMiny").unboxSingle();
    float z = getField("mMinz").unboxSingle();
    float r = getField("mRadius").unboxSingle();
    float xmx = getField("mMaxx").unboxSingle();
    float ymx = getField("mMaxy").unboxSingle();
    float zmx = getField("mMaxz").unboxSingle();

  // FIXME assert class = BoundingInfo
    return Sirikata::BoundingInfo(Sirikata::Vector3f(x,y,z),
                                  Sirikata::Vector3f(xmx,ymx,zmx),
                                  r);
}

Sirikata::UUID Object::unboxUUID() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    // FIXME assert class = UUID
    return UUID((CSharpUUID*)mono_object_unbox(mGCObject->object()));
}
static const Sirikata::String mIDstring("mID");
/*
Sirikata::ResourceID Object::unboxResourceID() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    // FIXME assert class = ResourceID
    return Sirikata::ResourceID(getField(mIDstring).unboxString());
}
*/

Sirikata::Duration Object::unboxDuration() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);
    // FIXME assert class = Time
    Sirikata::uint32 lower=getField("mLowerWord").unboxUInt32();
    Sirikata::uint32 upper=getField("mUpperWord").unboxUInt32();
    Sirikata::uint64 ticks=upper;
    ticks*=65536;
    ticks*=65536;
    ticks+=lower;
    return Sirikata::Duration::microseconds(ticks);
}
Sirikata::Time Object::unboxTime() const {
    return Sirikata::Time::epoch()+unboxDuration();
}


Sirikata::SpaceObjectReference Object::unboxSpaceObjectReference() const {
    MonoObject* obj = mGCObject->object();
    checkNullReference(obj);

    // FIXME assert class = SpaceObjectReference
    return SpaceObjectReference((CSharpSpaceObjectReference*)mono_object_unbox(mGCObject->object()));
}



//#####################################################################
// Function marshal
//#####################################################################
Sirikata::MemoryBuffer Object::marshal() const {
    if (object() == NULL)
        return Sirikata::MemoryBuffer();

    Class UtilityClass = Domain::root().getAssembly("sirikatacore").getClass("Sirikata", "Utility");

    static ThreadSafeSingleMethodLookupCache marshal_cache;
    Object byte_array = UtilityClass.send(&marshal_cache, "Marshal", *this);
    return byte_array.unboxByteArray();
}

//#####################################################################
// Function unmarshal
//#####################################################################
Object Object::unmarshal(const Sirikata::MemoryBuffer& marshalled) {
    if (marshalled.size() == 0)
        return Object(NULL);

    Class UtilityClass = Domain::root().getAssembly("sirikatacore").getClass("Sirikata", "Utility");

    Object byte_array = Domain::root().ByteArray( marshalled );

    static ThreadSafeSingleMethodLookupCache unmarshal_cache;
    return UtilityClass.send(&unmarshal_cache, "Unmarshal", byte_array);
}

} // namespace Mono
