/*  Sirikata - Mono Embedding
 *  MonoArray.cpp
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
#include <oh/Platform.hpp>
#include "MonoArray.hpp"
#include "MonoDomain.hpp"
#include "MonoClass.hpp"
#include "MonoAssembly.hpp"
#include "MonoException.hpp"

namespace Mono {

//#####################################################################
// Function Array
//#####################################################################
Array::Array(MonoObject* obj)
  : Object(obj)
{
    assert( instanceOf( Domain::root().getAssembly("mscorlib").getClass("System", "Array") ) );
}

//#####################################################################
// Function Array
//#####################################################################
Array::Array(Object obj)
  : Object(obj)
{
    assert( instanceOf( Domain::root().getAssembly("mscorlib").getClass("System", "Array") ) );
}

//#####################################################################
// Function ~Array
//#####################################################################
Array::~Array() {
}

//#####################################################################
// Function operator[]
//#####################################################################
Object Array::operator[](const Meru::int32 idx) const {
    return send("GetValue", Domain::root().Int32(idx));
}

//#####################################################################
// Function set
//#####################################################################
void Array::set(const Meru::int32 idx, const Object& obj) {
    send("SetValue", obj, Domain::root().Int32(idx));
}

//#####################################################################
// Function length
//#####################################################################
int Array::length() const {
    return getProperty("Length").unboxInt32();
}

} // namespace Mono
