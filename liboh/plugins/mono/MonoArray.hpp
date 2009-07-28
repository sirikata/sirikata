/*  Sirikata - Mono Embedding
 *  MonoArray.hpp
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

#ifndef _MONO_ARRAY_HPP_
#define _MONO_ARRAY_HPP_

#include "MonoObject.hpp"

namespace Mono {

/** A wrapper class for objects which implement the Array interface,
 *  e.g. Arrays.  This class provides similar methods and
 *  funcionality to the interface in Mono and is purely for convenience.
 */
class Array : public Object {
public:

  /** Construct an Array directly from the Mono internal representation
   *  of an object.
   *  \param obj the Mono internal object, which *must* implement the Array interface
   */
  explicit Array(MonoObject* obj);

  /** Construct an Array from a Mono object.
   *  \param obj the Mono object, which *must* implement the Array interface
   */
  explicit Array(Object obj);

  ~Array();

  /** Lookup the value associated with the given index.
   *  \param idx the index to lookup
   *  \returns the Mono Object assocated with the index in the list
   */
  Object operator[](const Sirikata::int32 idx) const;

  /** Set the value at the given index to the specified value.
   *  \param idx the index to assign to
   *  \param obj the object to assign to the specified index
   */
  void set(const Sirikata::int32 idx, const Object& obj);

  /** Returns the length of the list. */
  int length() const;
};

} // namespace Mono

#endif //_MONO_ARRAY_HPP_
