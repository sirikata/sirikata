/*  Sirikata - Mono Embedding
 *  MonoDomain.hpp
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
#ifndef _MONO_DOMAIN_HPP_
#define _MONO_DOMAIN_HPP_

#include "oh/Platform.hpp"
#include "MonoDefs.hpp"
#include <boost/shared_ptr.hpp>
#include <mono/metadata/appdomain.h>
#include "MonoClass.hpp"

namespace Mono {

/** A Mono AppDomain.  AppDomains provide isolation of code and data.
    The AppDomain will be destroyed when you no longer have a reference
    to it, so you must keep one around to avoid destroying the entire domain
    (and all objects in it).  Copies of the Domain can safely use the same
    underlying AppDomain.
 */
class Domain {
public:
    /** Construct a Domain from Mono's internal representation of a Domain. */
    explicit Domain(MonoDomain* domain);
    /** Construct a copy of a Domain.  Note that this is a shallow copy, the internal
     *  Mono Domain is not copied.
     */
    Domain(const Domain& cpy);

    ~Domain();

    /** Check if this Domain is null. */
    bool null() const;

    /** Set this domain as the active domain. */
    void set() const;
    /** Return true if this is the active domain. */
    bool active() const;


    /** Fires the ProcessExit event for this domain. */
    void fireProcessExit() const;

    /** Get the root domain for the current VM. */
    static Domain root();

    /** Get the assembly with the given name in this domain.  The returned assembly may
     *  be the same as from another Domain, but using this method will also increase the
     *  reference count on that assembly.
     *  \param name the name of the assembly to lookup
     *  \returns the assembly with that name, or a null assembly if it doesn't exist
     */
    Assembly getAssembly(const Sirikata::String& name) const;


    /** Copy the given null terminated string into a Mono string belonging to this Domain.
     *  \param str the null terminated string to copy
     */
    Object String(const char* str);
    /** Copy the given string of the given length into a Mono string belonging to this Domain.
     *  \param str pointer to the string to copy
     *  \param nbytes the length of the string in bytes
     */
    Object String(const char* str, int nbytes);
    /** Copy the given string into a Mono string belonging to this Domain.
     *  \param str the string to copy
     */
    Object String(const Sirikata::String& str);

    /** Create a Mono Boolean with the given value that belongs to this Domain.
     *  \param b the value of the boolean
     */
    Object Boolean(bool b);

    /** Create a Mono Byte with the given value that belongs to this Domain.
     *  \param i the value of the byte
     */
    Object Byte(guint8 i);

    /** Create a Mono UInt16 with the given value that belongs to this Domain.
     *  \param i the value of the UInt16
     */
    Object UInt16(guint16 i);

    /** Create a Mono UInt32 with the given value that belongs to this Domain.
     *  \param i the value of the UInt32
     */
    Object UInt32(guint32 i);

    /** Create a Mono UInt64 with the given value that belongs to this Domain.
     *  \param i the value of the UInt64
     */
    Object UInt64(guint64 i);

    /** Create a Mono SByte with the given value that belongs to this Domain.
     *  \param i the value of the SByte
     */
    Object SByte(gint8 i);

    /** Create a Mono Int16 with the given value that belongs to this Domain.
     *  \param i the value of the Int16
     */
    Object Int16(gint16 i);

    /** Create a Mono Int32 with the given value that belongs to this Domain.
     *  \param i the value of the Int32
     */
    Object Int32(gint32 i);

    /** Create a Mono Int64 with the given value that belongs to this Domain.
     *  \param i the value of the Int64
     */
    Object Int64(gint64 i);

    /** Create a Mono Single (single precision floating point value) with the given value that belongs to this Domain.
     *  \param f the value of the Single
     */
    Object Single(float f);

    /** Create a Mono Double (double precision floating point value) with the given value that belongs to this Domain.
     *  \param d the value of the Double
     */
    Object Double(double d);

    /** Create a Mono IntPtr, an unmanaged pointer, with the given value that belongs to this Domain.
     *  \param ptr the value of the IntPtr
     */
    Object IntPtr(void* ptr);

    /** Create a Mono Array holding objects of the given type that belongs to this Domain.
     *  \param klass the type of object the array should hold, i.e. the array returned will be of type klass[]
     *  \param sz the number of items the array should hold
     */
    ::Mono::Array Array(const Class& klass, int sz);

    /** Create a Mono Byte[] that contains the provided data. */
    ::Mono::Array ByteArray(const char* data, unsigned int length);

    /** Create a Mono Byte[] that contains the provided data. */
    ::Mono::Array ByteArray(const Sirikata::MemoryBuffer& data);

    /** Create a Mono Time that belongs to this Domain.
     *  \param time the Time to copy
     */
    Object Time(const Sirikata::Time& time);

    /** Create a Mono Vector3 that belongs to this Domain.
     *  \param vec the Vector3f to copy
     */
    Object Vector3(const Sirikata::Vector3f& vec);

    /** Create a Mono Vector3 that belongs to this Domain.
     *  \param vec the Vector3d to copy
     */
    Object Vector3(const Sirikata::Vector3d& vec);

    /** Create a Mono Color that belongs to this Domain.
     *  \param vec the Color to copy
     */
    Object Color(const Sirikata::Vector4f& color);

    /** Create a Mono Quaternion that belongs to this Domain.
     *  \param vec the Quaternion to copy
     */
    Object Quaternion(const Sirikata::Quaternion& vec);

    /** Create a Mono Location that belongs to this Domain.
     *  \param loc the Location to copy
     */
    Object Location(const Sirikata::Location& loc);

    /** Create a Mono Guid that belongs to this Domain.
     *  \param uuid the UUID to copy
     */
    Object UUID(const Sirikata::UUID& uuid);

    /** Create a Mono SpaceObjectReference that belongs to this Domain.
     *  \param loc the SpaceObjectReference to copy
     */
    Object SpaceObjectReference(const Sirikata::SpaceObjectReference& loc);


    /** Get the Sirikata.Runtime.Assemblies.AssemblyReader class for this domain. */
    Class AssemblyReaderClass();

    /** Create a Sirikata.Runtime.Assemblies.AssemblyRewriter object in this domain. */
    Object AssemblyRewriter();

    /** Create a Mono Sirikata.Runtime.TimerID. */
    //Object TimerID(Sirikata::Scripting::TimerID id);

    /** Create a Mono Sirikata.Runtime.ResourceID. */
    //Object ResourceID(const Sirikata::ResourceID& id);

private:
    Domain();

    /** Get the internal Mono Domain associated with this Domain. */
    MonoDomain* domain() const;

    /** Wrap the given pointer in an instance of the class in the given namespace.
     *  \param assembly the assembly the class is in
     *  \param name_space the namespace the class is in
     *  \param klass_name the name of the class to wrap the pointer in
     *  \param ptr the raw pointer to the C++ data
     *  \param own if true, Mono will own and garbage collect the raw data
     */
    Object wrapObject(const Sirikata::String& assembly, const Sirikata::String& name_space, const Sirikata::String& klass_name, void* ptr, bool own);

    /** Box an object using the specified class.
     *  \param assembly the assembly the class is in
     *  \param name_space the namespace the class is in
     *  \param struct_name the name of the struct to wrap the data in
     *  \param ptr the raw pointer to the data
     */
    Object boxObject(const Sirikata::String& assembly, const Sirikata::String& name_space, const Sirikata::String& struct_name, void* ptr);

    friend class MonoSystem;
    friend class Class;
    friend class Thread;

    /** Set the given domain as the root domain.
     *  \param rootd the Domain to be used as the new root Domain.
     */
    static void setRoot(const Domain& rootd);

    static Domain sRootDomain;

    class RefCountedMonoDomain;
    boost::shared_ptr<RefCountedMonoDomain> mDomain;
};


} // namespace Mono


#endif //_MONO_DOMAIN_HPP_
