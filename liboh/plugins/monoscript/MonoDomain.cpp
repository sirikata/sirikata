/*  Sirikata - Mono Embedding
 *  MonoDomain.cpp
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

#include <sirikata/oh/Platform.hpp>

#include "MonoDomain.hpp"
#include "MonoObject.hpp"
#include "MonoAssembly.hpp"
#include "MonoUtil.hpp"
#include "MonoSystem.hpp"
#include "MonoClass.hpp"
#include "MonoConvert.hpp"
#include <sirikata/core/util/Time.hpp>
#include "MonoArray.hpp"
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/util/RoutableMessageHeader.hpp>
//#include "ResourceID.hpp"
namespace Mono {

//#####################################################################
// Class Domain::RefCountedMonoDomain
//#####################################################################
class Domain::RefCountedMonoDomain {
public:
    RefCountedMonoDomain(MonoDomain* domain)
        : mDomain(domain)
    {
    }

    MonoDomain* domain() const {
        return mDomain;
    }

    ~RefCountedMonoDomain() {
        // FIXME
        //if (mDomain != NULL)
        //    mono_domain_unload(mDomain);
    }
private:
    MonoDomain* mDomain;
};



Domain Domain::sRootDomain = Domain(NULL);

//#####################################################################
// Function Domain
//#####################################################################
Domain::Domain(MonoDomain* domain)
    : mDomain(new RefCountedMonoDomain(domain))
{
}

//#####################################################################
// Function Domain
//#####################################################################
Domain::Domain(const Domain& cpy)
    : mDomain(cpy.mDomain)
{
}

//#####################################################################
// Function ~Domain
//#####################################################################
Domain::~Domain() {
}

//#####################################################################
// Function null
//#####################################################################
bool Domain::null() const {
    return (mDomain == NULL);
}

//#####################################################################
// Function set
//#####################################################################
void Domain::set() const {
    bool was_set = (mono_domain_set(mDomain->domain(), FALSE)!=0);
    assert(was_set);
}

//#####################################################################
// Function active
//#####################################################################
bool Domain::active() const {
    return false; // FIXME
}


void Domain::fireProcessExit() const {
    // This does effectively the same thing as fire_process_exit_event, but without requiring access to mono internals
    Object current_domain = getAssembly("mscorlib").getClass("System", "AppDomain").getProperty("CurrentDomain");
    Object process_exit_delegate = current_domain.getField("ProcessExit");

    if (process_exit_delegate.null())
        return;

    gpointer pa[2];
    pa [0] = domain();
    pa [1] = NULL;

    MonoObject* exc = NULL;
    mono_runtime_delegate_invoke (process_exit_delegate.object(), pa, &exc);

    assert(exc == NULL);
}


//#####################################################################
// Function root
//#####################################################################
Domain Domain::root() {
    return sRootDomain;
}

//#####################################################################
// Function setRoot
//#####################################################################
void Domain::setRoot(const Domain& rootd) {
    sRootDomain = rootd;
}

//#####################################################################
// Function domain
//#####################################################################
MonoDomain* Domain::domain() const {
    return mDomain->domain();
}

//#####################################################################
// Function getAssembly
//#####################################################################
Assembly Domain::getAssembly(const Sirikata::String& name) const {
    // The commented version requires an actual filename, but is the only thing
    // that actually takes a domain.  FIXME how do we get the domain to increment
    // the reference count of the assembly?
    //return Assembly( mono_domain_assembly_open(mDomain->domain(), name) );

    // For the time being, just use the global version which checks for a loaded
    // assembly with the given name.
    MonoAssemblyName aname;
    bool parsed = (mono_assembly_name_parse(name.c_str(), &aname)!=0);
    if (!parsed) return Assembly(NULL);

    MonoAssembly* assembly = mono_assembly_loaded(&aname);

    mono_assembly_name_free(&aname);

    return Assembly(assembly);
}

//#####################################################################
// Function String
//#####################################################################
Object Domain::String(const char* str) {
    return Object( (MonoObject*)mono_string_new(mDomain->domain(), str) );
}

//#####################################################################
// Function String
//#####################################################################
Object Domain::String(const char* str, int nbytes) {
    return Object( (MonoObject*)mono_string_new_len(mDomain->domain(), str, nbytes) );
}

//#####################################################################
// Function String
//#####################################################################
Object Domain::String(const Sirikata::String& str) {
    return Object( (MonoObject*)mono_string_new_len(mDomain->domain(), str.c_str(), str.size()) );
}

//#####################################################################
// Function Boolean
//#####################################################################
Object Domain::Boolean(bool b) {
    return Object( mono_value_box(mDomain->domain(), mono_get_boolean_class(), &b) );
}

//#####################################################################
// Function Byte
//#####################################################################
Object Domain::Byte(Sirikata::uint8 i) {
    return Object( mono_value_box(mDomain->domain(), mono_get_byte_class(), &i) );
}

//#####################################################################
// Function UInt16
//#####################################################################
Object Domain::UInt16(Sirikata::uint16 i) {
    return Object( mono_value_box(mDomain->domain(), mono_get_uint16_class(), &i) );
}

//#####################################################################
// Function UInt32
//#####################################################################
Object Domain::UInt32(Sirikata::uint32 i) {
    return Object( mono_value_box(mDomain->domain(), mono_get_uint32_class(), &i) );
}

//#####################################################################
// Function UInt64
//#####################################################################
Object Domain::UInt64(Sirikata::uint64 i) {
    return Object( mono_value_box(mDomain->domain(), mono_get_uint64_class(), &i) );
}

//#####################################################################
// Function SByte
//#####################################################################
Object Domain::SByte(Sirikata::int8 i) {
    return Object( mono_value_box(mDomain->domain(), mono_get_sbyte_class(), &i) );
}

//#####################################################################
// Function Int16
//#####################################################################
Object Domain::Int16(Sirikata::int16 i) {
    return Object( mono_value_box(mDomain->domain(), mono_get_int16_class(), &i) );
}

//#####################################################################
// Function Int32
//#####################################################################
Object Domain::Int32(Sirikata::int32 i) {
    return Object( mono_value_box(mDomain->domain(), mono_get_int32_class(), &i) );
}

//#####################################################################
// Function Int64
//#####################################################################
Object Domain::Int64(Sirikata::int64 i) {
    return Object( mono_value_box(mDomain->domain(), mono_get_int64_class(), &i) );
}

//#####################################################################
// Function Single
//#####################################################################
Object Domain::Single(float f) {
    return Object( mono_value_box(mDomain->domain(), mono_get_single_class(), &f) );
}

//#####################################################################
// Function Double
//#####################################################################
Object Domain::Double(double d) {
    return Object( mono_value_box(mDomain->domain(), mono_get_double_class(), &d) );
}

//#####################################################################
// Function IntPtr
//#####################################################################
Object Domain::IntPtr(void* ptr) {
    return Object( mono_value_box(mDomain->domain(), mono_get_intptr_class(), &ptr) );
}

//#####################################################################
// Function Array
//#####################################################################
Array Domain::Array(const Class& klass, int sz) {
    return Array::Array( (MonoObject*)mono_array_new(mDomain->domain(), klass.klass(), sz) );
}

Array Domain::ByteArray(const void* data, unsigned int length) {
    Class ByteClass(mono_get_byte_class());
    ::Mono::Array byte_array = Domain::root().Array(ByteClass, length);
    MonoArray* byte_mono_array = (MonoArray*)byte_array.object();
    memcpy( mono_array_addr(byte_mono_array,char,0), data, length );
    return byte_array;
}

Array Domain::ByteArray(const Sirikata::MemoryBuffer& data) {
    int length = data.size();
    Class ByteClass(mono_get_byte_class());
    ::Mono::Array byte_array = Domain::root().Array(ByteClass, length);
    MonoArray* byte_mono_array = (MonoArray*)byte_array.object();

    if (!data.empty())
        memcpy( mono_array_addr(byte_mono_array,char,0), &data[0], length );

    return byte_array;
}


Array Domain::StringArray(unsigned int length) {
    Class StringClass(mono_get_string_class());
    ::Mono::Array string_array = Domain::root().Array(StringClass, length);
    return string_array;
}

Array Domain::StringArray(const std::vector<Sirikata::String>& data) {
    int length = data.size();
    Class StringClass(mono_get_string_class());
    ::Mono::Array string_array = Domain::root().Array(StringClass, length);

    for(unsigned int idx = 0; idx < data.size(); idx++)
        string_array.set(idx, String(data[idx]));

    return string_array;
}

//#####################################################################
// Function wrapObject
//#####################################################################
Object Domain::wrapObject(const Sirikata::String& assembly, const Sirikata::String& name_space, const Sirikata::String& klass_name, void* ptr, bool own) {
    Class klass = getAssembly(assembly).getClass(name_space, klass_name);
    return klass.instance( IntPtr(ptr) , Boolean(own) );
}


Object Domain::boxObject(const Sirikata::String& assembly, const Sirikata::String& name_space, const Sirikata::String& struct_name, void* ptr) {
    Class klass = getAssembly(assembly).getClass(name_space, struct_name);
    return Object( mono_value_box(mDomain->domain(), klass.mClass, ptr) );
}


Object Domain::UUID(const Sirikata::UUID& uuid) {
    CSharpUUID csuuid;
    ConvertUUID(uuid, &csuuid);

    return boxObject("mscorlib", "System", "Guid", &csuuid);
}

Object Domain::SpaceObjectReference(const Sirikata::SpaceObjectReference& sor) {
    CSharpSpaceObjectReference cssor;
    ConvertSpaceObjectReference(sor, &cssor);

    return boxObject("Sirikata.Runtime", "Sirikata.Runtime", "SpaceObjectReference", &cssor);
}




//#####################################################################
// Function Time
//#####################################################################
Object Domain::Time(const Sirikata::Time& time) {
    CSharpTime ticks = ConvertTime(time);
    return boxObject("Sirikata.Runtime","Sirikata.Runtime","Time",&ticks);
}

//#####################################################################
// Function Vector3
//#####################################################################
Object Domain::Vector3(const Sirikata::Vector3f& v3) {
    CSharpVector3 csvec3;
    ConvertVector3(v3, &csvec3);

    return boxObject("Sirikata.Runtime", "Sirikata.Runtime", "Vector3", &csvec3);
}

Object Domain::Vector3(const Sirikata::Vector3d& v3) {
    CSharpVector3 csvec3;
    ConvertVector3(v3, &csvec3);

    return boxObject("Sirikata.Runtime", "Sirikata.Runtime", "Vector3", &csvec3);
}

Object Domain::Color(const Sirikata::Vector4f& color) {
    CSharpColor cscolor;
    ConvertColor(color, &cscolor);

    return boxObject("Sirikata.Runtime", "Sirikata.Runtime", "Color", &cscolor);
}

//#####################################################################
// Function Quaternion
//#####################################################################
Object Domain::Quaternion(const Sirikata::Quaternion& quat) {
    CSharpQuaternion csquat;
    ConvertQuaternion(quat, &csquat);

    return boxObject("Sirikata.Runtime", "Sirikata.Runtime", "Quaternion", &csquat);
}

//#####################################################################
// Function Location
//#####################################################################
Object Domain::Location(const Sirikata::Location& loc) {
    CSharpLocation csloc;
    ConvertLocation(loc, &csloc);

    return boxObject("Sirikata.Runtime", "Sirikata.Runtime", "Location", &csloc);
}

Class Domain::AssemblyReaderClass() {
    return getAssembly("Sirikata.Runtime").getClass("Sirikata.Runtime.Assemblies", "AssemblyReader");
}

Object Domain::AssemblyRewriter() {
    return getAssembly("Sirikata.Runtime").getClass("Sirikata.Runtime.Assemblies", "AssemblyRewriter").instance();
}

} // namespace Mono
