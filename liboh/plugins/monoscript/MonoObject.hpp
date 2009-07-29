/*  Sirikata - Mono Embedding
 *  MonoObject.hpp
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
#ifndef _MONO_OBJECT_HPP_
#define _MONO_OBJECT_HPP_


#include "MonoDefs.hpp"
#include "util/UUID.hpp"
#include "util/SpaceObjectReference.hpp"
#include "util/BoundingInfo.hpp"
#include <mono/metadata/object.h>
#include <vector>

namespace Mono {

class MonoGCObject;

/** A Mono Object.  This class provides all the basic functionality you would
 *  get from an object in Mono - access to methods, properties, fields - as well
 *  as some convenience methods for checking interfaces, performing unboxing, etc.
 */
class Object {
public:
    /** Construct a null object. */
    explicit Object();
    /** Construct an object from the Mono internal representation of an
     *  object.
     */
    explicit Object(MonoObject* obj);
    /** Construct an objec which is a copy of the given object.  This is
     *  just a copy of the reference but they share the same backing and
     *  GC handle.
     */
    Object(const Object& cpy);

    virtual ~Object();

    /** Get the Mono internal object representation for this object. */
    MonoObject* object() const;

/*
    void listParentClasses() const;
*/
    void listMethods() const;

    /** Returns true if this object is a null reference. */
    bool null() const;

    /** Returns true if this object is a value type. */
    bool valuetype() const;

    /** Returns the type of this object as a Mono::Class. */
    Class type() const;
    /** Returns true if this object extends or implements the given class.
     *  Note that this works for both classes and interfaces.
     *  \param klass the class or interface to check for inheritance/implementation
     */
    bool instanceOf(Class klass) const;

    /** Returns the assembly that holds the class for this object. */
    Assembly assembly() const;

    /** Returns true if the Mono::Object is the same object as this Mono::Object.
     *  \param rhs the object to compare to
     */
    bool referenceEquals(const Object& rhs);


    /** Calculate the memory usage of this object, i.e. the memory usage of all objects
     *  that can be reached from this object reference.  Note that this is not thread safe,
     *  unlike the GC.  You will likely get incorrect results if you are manipulating the
     *  object in one thread and computing memory usage in another.
     *  \returns the total size of all objects reachable from this one, in bytes
     */
    guint32 memoryUsage() const;

    /** Call the method message on the object and return the return value from the method. */
    Object send(const Sirikata::String& message) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(const Sirikata::String& message, const Object& p1) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(const Sirikata::String& message, const Object& p1, const Object& p2) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3, const Object& p4) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3, const Object& p4, const Object& p5) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(const Sirikata::String& message, const std::vector<Object>& args) const;

    /** Call the method message on the object and return the return value from the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3, const Object& p4) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3, const Object& p4, const Object& p5) const;
    /** Call the method message on the object and return the return value from the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message, const std::vector<Object>& args) const;

    /** Get the value of the given property.
     *  \param propname the name of the property
     *  \returns the value of the property
     */
    Object getProperty(const Sirikata::String& propname) const;
    /** Get the value of the given property with the specified parameter.
     *  \param propname the name of the property
     *  \param p1 the parameter to the property
     *  \returns the value of the property
     */
    Object getProperty(const Sirikata::String& propname, const Object& p1) const;

    /** Set the value of the given property to the specified value.
     *  \param propname the name of the property
     *  \param val the new value of the property
     */
    void setProperty(const Sirikata::String& propname, const Object& val) const;
    /** Set the value of the given property with the specified parameter to the specified value.
     *  \param propname the name of the property
     *  \param p1 the parameter value
     *  \param val the new value of the property
     */
    void setProperty(const Sirikata::String& propname, const Object& p1, const Object& val) const;

    /** Get the field with the given name for this object.
     *  \param fieldname the name of the field to access
     *  \returns the current value of the named field
     */
    Object getField(const Sirikata::String& fieldname) const;

    /** Set the field with the given name for this object to the specified value.
     *  \param fieldname the name of the field to set
     *  \param value the new value for the field
     */
    void setField(const Sirikata::String& fieldname, const Object& value) const;

    /** Returns the unboxed value of this boolean object. The object *must* represent
     *  a Boolean.
     */
    bool unboxBoolean() const;

    /** Returns the unboxed value of this Byte object. The object *must* represent
     *  a Byte.
     */
    guint8 unboxByte() const;
    /** Returns the unboxed value of this UInt16 object. The object *must* represent
     *  a UInt16.
     */
    guint16 unboxUInt16() const;
    /** Returns the unboxed value of this UInt32 object. The object *must* represent
     *  a UInt32.
     */
    guint32 unboxUInt32() const;
    /** Returns the unboxed value of this UInt64 object. The object *must* represent
     *  a UInt64.
     */
    guint64 unboxUInt64() const;

    /** Returns the unboxed value of this SByte object. The object *must* represent
     *  a SByte.
     */
    gint8 unboxSByte() const;
    /** Returns the unboxed value of this Int16 object. The object *must* represent
     *  a Int16.
     */
    gint16 unboxInt16() const;
    /** Returns the unboxed value of this Int32 object. The object *must* represent
     *  a Int32.
     */
    gint32 unboxInt32() const;
    /** Returns the unboxed value of this Int64 object. The object *must* represent
     *  a Int64.
     */
    gint64 unboxInt64() const;

    /** Returns the unboxed value of this Single object. The object *must* represent
     *  a Single.
     */
    float unboxSingle() const;
    /** Returns the unboxed value of this Double object. The object *must* represent
     *  a Double.
     */
    double unboxDouble() const;

    /** Returns the unboxed value of this enum object.  The object *must* represent
     *  an enum, but may use any underlying type.
     */
    gint32 unboxEnum() const;

    /** Returns the unboxed value of this IntPtr. */
    void* unboxIntPtr() const;

    /** Returns the unboxed value of this HandleRef object. The object *must* represent
     *  a HandleRef. Note that this *will not work* for an IntPtr and IntPtrs should
     *  never be unwrapped directly.
     */
    void* unboxHandleRef() const;

    /** Returns the unboxed value of this String object.  The object *must* represent
     *  a String.
     */
    Sirikata::String unboxString() const;

    /** Returns a buffer containing the contents of the byte array. The object *must*
     *  represent a byte array.
     */
    Sirikata::MemoryBuffer unboxByteArray() const;

    /** "Unbox" a Mono Vector3f into a C++ Vector3f */
    Sirikata::Vector3f unboxVector3f() const;

    /** "Unbox" a Mono Vector3d into a C++ Vector3d */
    Sirikata::Vector3d unboxVector3d() const;

    /** "Unbox" a Mono Quaternion into a C++ Quaternion */
    Sirikata::Quaternion unboxQuaternion() const;

    /** "Unbox" a Mono Location into a C++ Location */
    Sirikata::Location unboxLocation() const;

    /** "Unbox" a Mono Color into a C++ Color */
    Sirikata::Vector4f unboxColor() const;

    Sirikata::BoundingInfo unboxBoundingInfo() const;

    /** "Unbox" a Mono UUID into a C++ UUID */
    Sirikata::UUID unboxUUID() const;

    /** "Unbox" a Mono ResourceID into a C++ ResourceID */
    //Sirikata::ResourceID unboxResourceID()const;

    /** "Unbox" a Mono SpaceObjectReference into a C++ SpaceObjectReference */
    Sirikata::SpaceObjectReference unboxSpaceObjectReference() const;

    /** "Unbox" a Mono DateTime into a C++ Time */
    Sirikata::Time unboxTime() const;

    /** Marhsal this object to a buffer.  Note that this will marshal the entire object
     *  tree starting from this object, i.e. external references are only used if you
     *  specify them as proxy objects.
     *  \returns the object serialized to a buffer
     */
    Sirikata::MemoryBuffer marshal() const;
    /** Unmarshal an object from the given string.
     *  \returns the object generated from the serialization string
     */
    static Object unmarshal(const Sirikata::MemoryBuffer& marshalled);

    /** Unwrap a wrapped native object.  Note that this is entirely type unsafe; no guarantee
     *  can be made that the returned pointer is valid for the requested type.  Further,
     *  since the lifetime of the object cannot be guaranteed, be certain to maintain the
     *  object reference until you are done with the pointer.
     *  \returns a pointer to the unwrapped object
     */
    template<typename T>
    T* unwrap() const {
        return (T*)unsafeUnwrap();
    }

    /** Unwrap a copy of a wrapped native object.  Note that this is entirely type unsafe;
     *  no guarantee can be made that the unwrapped pointer used to make the copy is valid
     *  for the requested type.  Note that this does not handle inheritance - i.e. the copy
     *  will be precisely the type requested.
     *  \returns a copy of the unwrapped object
     */
    template<typename T>
    T unwrapCopy() {
        return T(*(T*)unsafeUnwrap());
    }

private:
    friend class Class;
    void* unsafeUnwrap() const;

    std::tr1::shared_ptr<MonoGCObject> mGCObject;
};


} // namespace Mono

#endif //_MONO_OBJECT_HPP_
