/*  Sirikata - Mono Embedding
 *  MonoUtil.hpp
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
#ifndef _MONO_UTIL_HPP_
#define _MONO_UTIL_HPP_

#include "MonoDefs.hpp"
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/class.h>

namespace Mono {

// Mono Internals, currently required to get at the MonoType* from the MonoReflectionType*
struct MonoReflectionType {
    MonoObject object;
    MonoType  *type;
};

/** Free the resources associated with a MonoAssemblyName structure.
 *  \param aname the name to free
 *  Frees the provided assembly name object.
 *  (it does not frees the object itself, only the name members).
 */
void mono_assembly_name_free(MonoAssemblyName* aname);
/** Parse a raw assembly name into a MonoAssemblyName structure.
 *  \param name the human readable assembly name string
 *  \param aname the MonoAssemblyName struct to store the parsed name into
 *  \param save_public_key if true, the public key is saved
 *  \param is_version_defined if not NULL, will be set to true if the version is defined in the name
 */
gboolean mono_assembly_name_parse_full(const char* name, MonoAssemblyName* aname, gboolean save_public_key, gboolean* is_version_defined);
/** Parse a raw assembly name into a MonoAssemblyName structure.
 *  \param name the human readable assembly name string
 *  \param aname the MonoAssemblyName struct to store the parsed name into
 */
gboolean mono_assembly_name_parse(const char* name, MonoAssemblyName* aname);

/** Check if the given pointer is a null reference and throw a NullReferenceException
 *  if it is.
 *  \param obj the object reference to check
 */
void checkNullReference(MonoObject* obj);

/** Look up the method for a property get operation.
 *  \param obj the object to search for the method in
 *  \param name the name of the property to look up
 */
MonoMethod* mono_object_get_property_get(MonoObject* obj, const char* name);
/** Look up the method for a property set operation.
 *  \param obj the object to search for the method in
 *  \param name the name of the property to look up
 */
MonoMethod* mono_object_get_property_set(MonoObject* obj, const char* name);

/** Convert an object into a form acceptable to the Mono API.  This means unwrapping value
 *  types to their raw form and simply returning the MonoObject* for object types.
 *  \param receive_klass the type that will
 *  \param obj the object to convert to its raw form
 */
void* object_to_raw(MonoClass* receive_klass, MonoObject* obj);

/** Lowest level method for invoking Mono methods.  This is a very thin wrapper around
 *  mono_runtime_invoke which deals only with unboxing args and throwing resulting exceptions.
 *  \param this_ptr pointer to the object to invoke the method on, or NULL if the method is static
 *  \param method the name of the method to invoke
 *  \param args the argument array
 *  \param nargs the number of arguments to pass
 *  \returns the value returned by the method invocation
 *  \throws a Mono::Exception if the method call throws an exception
 */
Object send_method(MonoObject* this_ptr, MonoMethod* method, MonoObject* args[], int nargs);

/** Method call wrapper for instance methods.
 *  \param this_ptr pointer to the object to invoke the method on
 *  \param message the name of the method to invoke
 *  \param args the argument array
 *  \param nargs the number of arguments to the method
 *  \param cache a cache for the method lookup, if NULL ignored
 *  \returns the value returned by the method invocation
 *  \throws a Mono::Exception if the method call throws an exception
 */
Object send_base(MonoObject* this_ptr, const char* message, MonoObject* args[], int nargs, MethodLookupCache* cache = NULL);
/** Method call wrapper for static methods.
 *  \param klass the class to look up the method in
 *  \param message the name of the method to invoke
 *  \param args the argument array
 *  \param nargs the number of arguments to the method
 *  \param cache a cache for the method lookup, if NULL ignored
 *  \returns the value returned by the method invocation
 *  \throws a Mono::Exception if the method call throws an exception
 */
Object send_base(MonoClass* klass, const char* message, MonoObject* args[], int nargs, MethodLookupCache* cache = NULL);


/** Measures the memory usage from an object root.  This essentially does the
 *  "mark" phase of a mark and sweep GC, summing the size of each object as it
 *  goes.
 */
int memory_usage_from_object_root(MonoObject* obj);

} // namespace Mono


#endif //_MONO_UTIL_HPP_
