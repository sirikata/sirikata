/*  Sirikata - Mono Embedding
 *  MonoAssembly.cpp
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

#include "MonoAssembly.hpp"
#include "MonoClass.hpp"
#include "MonoException.hpp"
#include <cassert>

#include <mono/metadata/class.h>

namespace Mono {

//#####################################################################
// Function Assembly
//#####################################################################
Assembly::Assembly(MonoAssembly* ass)
    : mAssembly(ass)
{
    assert(ass != NULL);
}

//#####################################################################
// Function Assembly
//#####################################################################
Assembly::Assembly(const Assembly& cpy)
    : mAssembly(cpy.mAssembly)
{
}

//#####################################################################
// Function ~Assembly
//#####################################################################
Assembly::~Assembly() {
}

//#####################################################################
// Function getClass
//#####################################################################
Class Assembly::getClass(const Sirikata::String& klass_name) const {
    MonoImage* image = mono_assembly_get_image(mAssembly);
    assert(image != NULL);
    MonoClass* klass = mono_class_from_name(image, "", klass_name.c_str());
    if (klass == NULL)
        throw Exception::ClassNotFound(klass_name, fullname());
    return Class(klass);
}

//#####################################################################
// Function getClass
//#####################################################################
Class Assembly::getClass(const Sirikata::String& name_space, const Sirikata::String& klass_name) const {
    MonoImage* image = mono_assembly_get_image(mAssembly);
    assert(image != NULL);
    MonoClass* klass = mono_class_from_name(image, name_space.c_str(), klass_name.c_str());
    if (klass == NULL)
        throw Exception::ClassNotFound(name_space, klass_name);
    return Class(klass);
}

//#####################################################################
// Function fullname
//#####################################################################
Sirikata::String Assembly::fullname() const {
    MonoAssemblyName aname;

    MonoImage* image = mono_assembly_get_image(mAssembly);
    assert(image != NULL);
    bool filled_name = mono_assembly_fill_assembly_name(image, &aname);
    if (!filled_name) {
        printf("!!! Loaded assembly, mono_assembly_fill_assembly_name failed\n");
        return "(Unknown)";
    }

    char* name = mono_stringify_assembly_name(&aname);
    std::string res_name = name;
    g_free(name);

    return res_name;
}

//#####################################################################
// Function name
//#####################################################################
Sirikata::String Assembly::name() const {
    MonoAssemblyName aname;

    MonoImage* image = mono_assembly_get_image(mAssembly);
    assert(image != NULL);
    bool filled_name = mono_assembly_fill_assembly_name(image, &aname);
    if (!filled_name) {
        printf("!!! Loaded assembly, mono_assembly_fill_assembly_name failed\n");
        return "(Unknown)";
    }

    std::string res_name = aname.name;
    return res_name;
}

} // namespace Mono
