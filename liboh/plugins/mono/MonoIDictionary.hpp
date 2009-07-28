/*  Sirikata - Mono Embedding
 *  MonoIDictionary.hpp
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
#ifndef _MONO_IDICTIONARY_HPP_
#define _MONO_IDICTIONARY_HPP_


#include "MonoObject.hpp"

namespace Mono {

/** A wrapper class for objects which implement the IDictionary interface,
 *  e.g. Hashtables.  This class provides similar methods and functionality
 *  to the interface in Mono and is purely for convenience.
 */
class IDictionary : public Object {
public:
    /** Construct an IDictionary directly from the Mono internal representation
     *  of an object.
     *  \param obj the Mono internal object, which *must* implement the IDictionary interface
     */
    explicit IDictionary(MonoObject* obj);
    /** Construct an IDictionary from a Mono object.
     *  \param obj the Mono object, which *must* implement the IDictionary interface
     */
    explicit IDictionary(Object obj);

    ~IDictionary();

    /** Lookup the value associated with the given key.
     *  \param key the key to use in the dictionary lookup
     *  \returns the value associated with the key parameter
     */
    Object operator[](const Object& key) const;
    /** Lookup the value associated with the given string key.  Just a convenience method
     *  which handles wrapping the string parameter into a Mono string.
     *  \param key the string key to use in the dictionary lookup
     *  \returns the value associated with the key parameter
     */
    Object operator[](const Sirikata::String& key) const;
    /** Lookup the value associated with the given integer key. Just a convenience method
     *  which handles wrapping the integer parameter into a Mono integer.
     *  \param key the integer key to use in the dictionary lookup
     *  \returns the value associated with the key parameter
     */
    Object operator[](const Sirikata::int32 key) const;

    /** Returns the number of key/value pairs stored in this IDictionary. */
    int count() const;
};

} // namespace Mono

#endif //_MONO_IDICTIONARY_HPP_
