/*  Sirikata - Mono Embedding
 *  MonoException.cpp
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
#include "MonoException.hpp"
#include "MonoClass.hpp"
#include "MonoDomain.hpp"
#include <mono/metadata/appdomain.h>
#include <mono/metadata/exception.h>

namespace Mono {


//#####################################################################
// Function Exception
//#####################################################################
Exception::Exception(const Object& orig)
    : mOriginal(orig)
{
    assert(!mOriginal.null());
    assert(mOriginal.instanceOf( Class(mono_get_exception_class()) ));
}

//#####################################################################
// Function ~Exception
//#####################################################################
Exception::~Exception() {
}


//#####################################################################
// Function message
//#####################################################################
std::string Exception::message() const {
    Object message_obj = mOriginal.getProperty("Message");
    assert(!message_obj.null());
    return message_obj.unboxString();
}

//#####################################################################
// Function backtrace
//#####################################################################
std::string Exception::backtrace() const {
    Object backtrace_obj = mOriginal.getProperty("StackTrace");
    // FIXME backtraces won't be filled in for exceptions generated
    // outside the VM proper because they aren't thrown using Mono's
    // mechanism (which requires JIT ).  There doesn't seem to be a way
    // to fill it in without exposing the Mono internals....
    //assert(!backtrace_obj.null());
    if (backtrace_obj.null())
        return std::string("(StackTrace unavailable.)");
    return backtrace_obj.unboxString();
}

//#####################################################################
// Function original
//#####################################################################
Object Exception::original() const {
    return mOriginal;
}

//#####################################################################
// Function print
//#####################################################################
void Exception::print(std::ostream& os) const {
    os << original().send("ToString").unboxString() << std::endl;
    //os << message() << std::endl << backtrace() << std::endl;
}


//#####################################################################
// Function NullReference
//#####################################################################
Exception Exception::NullReference() {
    MonoException* exc = mono_get_exception_null_reference();
    assert(exc != NULL);
    Object exc_obj = Object( (MonoObject*)exc );
    return Exception( exc_obj );
}

//#####################################################################
// Function MissingProperty
//#####################################################################
Exception Exception::MissingProperty(Class klass, const std::string& prop_name) {
    // FIXME using missing_method because there doesn't seem to be missing_property
    MonoException* exc = mono_get_exception_missing_method(klass.getName().c_str(), prop_name.c_str());
    assert(exc != NULL);
    Object exc_obj = Object( (MonoObject*)exc );
    return Exception( exc_obj );
}

//#####################################################################
// Function MissingMethod
//#####################################################################
Exception Exception::MissingMethod(Class klass, const std::string& method_name) {
    MonoException* exc = mono_get_exception_missing_method(klass.getName().c_str(), method_name.c_str());
    assert(exc != NULL);
    Object exc_obj = Object( (MonoObject*)exc );
    return Exception( exc_obj );
}

//#####################################################################
// Function MissingField
//#####################################################################
Exception Exception::MissingField(Class klass, const std::string& field_name) {
    MonoException* exc = mono_get_exception_missing_field(klass.getName().c_str(), field_name.c_str());
    assert(exc != NULL);
    Object exc_obj = Object( (MonoObject*)exc );
    return Exception( exc_obj );
}

//#####################################################################
// Function ClassNotFound
//#####################################################################
Exception Exception::ClassNotFound(const std::string& klass_name, const std::string& assembly_name) {
    MonoString* klass_str = mono_string_new( mono_domain_get(), klass_name.c_str() );
    assert(klass_str != NULL);
    MonoException* exc = mono_get_exception_type_load(klass_str, (char*)assembly_name.c_str());
    assert(exc != NULL);
    Object exc_obj = Object( (MonoObject*)exc );
    return Exception(exc_obj);
}

//#####################################################################
// Function ClassNotFound
//#####################################################################
Exception Exception::ClassNotFound(const std::string& name_space, const std::string& klass_name, const std::string& assembly_name) {
    std::string full_klass_name = name_space + "." + klass_name;
    MonoString* namespace_klass_str = mono_string_new( mono_domain_get(), full_klass_name.c_str() );
    assert(namespace_klass_str != NULL);
    MonoException* exc = mono_get_exception_type_load(namespace_klass_str, (char*)assembly_name.c_str());
    assert(exc != NULL);
    Object exc_obj = Object( (MonoObject*)exc );
    return Exception(exc_obj);
}

//#####################################################################
// Function operator<<
//#####################################################################
std::ostream& operator<<(std::ostream& os, Exception& exc) {
    exc.print(os);
    return os;
}

} // namespace Mono
