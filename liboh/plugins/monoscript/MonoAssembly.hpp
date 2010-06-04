/*  Sirikata - Mono Embedding
 *  MonoAssembly.hpp
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
#ifndef _MONO_ASSEMBLY_HPP_
#define _MONO_ASSEMBLY_HPP_

#include <sirikata/oh/Platform.hpp>
#include "MonoDefs.hpp"
#include <mono/metadata/assembly.h>

namespace Mono {

/** A Mono Assembly is a dynamic library or executable containing a set
 *  of classes.
 */
class Assembly {
public:
    /** Construct an Assembly from Mono's internal representation of an assembly. */
    explicit Assembly(MonoAssembly* ass);
    /** Copy an assembly. */
    Assembly(const Assembly& cpy);

    ~Assembly();

    /** Get the Class with the name klass_name from the assembly.
     *  This method looks in the global namespace.
     *  \param klass_name the name of the class to lookup
     *  \returns the class
     *  \throws Exception ClassNotFound
     */
    Class getClass(const Sirikata::String& klass_name) const;
    /** Get the Class with the name klass_name from the assembly.
     *  \param name_space the namespace to look in
     *  \param klass_name the name of the class to lookup
     *  \returns the class
     *  \throws Exception ClassNotFound
     */
    Class getClass(const Sirikata::String& name_space, const Sirikata::String& klass_name) const;

    /** Get the name of this assembly. */
    Sirikata::String name() const;

    /** Get the full name of this assembly, e.g. MyAssembly, Version=0.0.0.0, Culture=neutral. */
    Sirikata::String fullname() const;
private:
    Assembly();

    MonoAssembly* mAssembly;
};

} // namespace Mono

#endif //_MONO_ASSEMBLY_HPP_
