/*  Sirikata - Mono Embedding
 *  MonoIDictionary.cpp
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
#include "MonoIDictionary.hpp"
#include "MonoDomain.hpp"
#include "MonoClass.hpp"
#include "MonoAssembly.hpp"

namespace Mono {

//#####################################################################
// Function IDictionary
//#####################################################################
IDictionary::IDictionary(MonoObject* obj)
    : Object(obj)
{
    assert( instanceOf( Domain::root().getAssembly("mscorlib").getClass("System.Collections", "IDictionary") ) );
}

//#####################################################################
// Function IDictionary
//#####################################################################
IDictionary::IDictionary(Object obj)
    : Object(obj)
{
    assert( instanceOf( Domain::root().getAssembly("mscorlib").getClass("System.Collections", "IDictionary") ) );
}

//#####################################################################
// Function ~IDictionary
//#####################################################################
IDictionary::~IDictionary() {
}

//#####################################################################
// Function operator[]
//#####################################################################
Object IDictionary::operator[](const Object& obj) const {
    return getProperty("Item", obj);
}

//#####################################################################
// Function operator[]
//#####################################################################
Object IDictionary::operator[](const Sirikata::String& str) const {
    return getProperty("Item", Domain::root().String(str));
}

//#####################################################################
// Function operator[]
//#####################################################################
Object IDictionary::operator[](const Sirikata::int32 idx) const {
    return getProperty("Item", Domain::root().Int32(idx));
}

//#####################################################################
// Function count
//#####################################################################
int IDictionary::count() const {
    return getProperty("Count").unboxInt32();
}

} // namespace Mono
