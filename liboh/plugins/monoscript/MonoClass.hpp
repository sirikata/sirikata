/*  Sirikata - Mono Embedding
 *  MonoClass.hpp
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
#ifndef _MONO_CLASS_HPP_
#define _MONO_CLASS_HPP_

#include "MeruDefs.hpp"
#include "MonoDefs.hpp"
#include <mono/metadata/metadata.h>

namespace Mono {

/** A Mono Class.
 */
class Class {
public:
    /** Construct a Class from Mono's internal representation of a Class. */
    explicit Class(MonoClass* klass);
    /** Copy a Class. */
    Class(const Class& cpy);

    ~Class();

    /** Returns true if this class is a value type. */
    bool valuetype() const;

    /** Returns the assembly that contains this class. */
    Assembly assembly() const;

    /** Create a new instance of this class, passing the given parameters to the constructor. */
    Object instance() const;
    /** Create a new instance of this class, passing the given parameters to the constructor. */
    Object instance(const Object& p1) const;
    /** Create a new instance of this class, passing the given parameters to the constructor. */
    Object instance(const Object& p1, const Object& p2) const;
    /** Create a new instance of this class, passing the given parameters to the constructor. */
    Object instance(const Object& p1, const Object& p2, const Object& p3) const;
    /** Create a new instance of this class, passing the given parameters to the constructor. */
    Object instance(const Object& p1, const Object& p2, const Object& p3, const Object& p4) const;
    /** Create a new instance of this class, passing the given parameters to the constructor. */
    Object instance(const Object& p1, const Object& p2, const Object& p3, const Object& p4, const Object& p5) const;

    /** Get the name of this class.  This returns only the short name, i.e. not fully qualified. */
    std::string getName() const;
    /** Get the namespace this class resides in. */
    std::string getNamespace() const;

    /** Call a static method on this class, passing the additional parameters to the method. */
    Object send(const Sirikata::String& message) const;
    /** Call a static method on this class, passing the additional parameters to the method. */
    Object send(const Sirikata::String& message, const Object& p1) const;
    /** Call a static method on this class, passing the additional parameters to the method. */
    Object send(const Sirikata::String& message, const Object& p1, const Object& p2) const;
    /** Call a static method on this class, passing the additional parameters to the method. */
    Object send(const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3) const;

    /** Call a static method on this class, passing the additional parameters to the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message) const;
    /** Call a static method on this class, passing the additional parameters to the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1) const;
    /** Call a static method on this class, passing the additional parameters to the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2) const;
    /** Call a static method on this class, passing the additional parameters to the method. */
    Object send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3) const;


    /** Get the value of the given static property.
     *  \param propname the name of the property
     *  \returns the value of the property
     */
    Object getProperty(const Sirikata::String& propname) const;
    /** Get the value of the given static property.
     *  \param cache a method lookup cache
     *  \param propname the name of the property
     *  \returns the value of the property
     */
    Object getProperty(PropertyLookupCache* cache, const Sirikata::String& propname) const;

    /** Get the static field with the given name for this class.
     *  \param fieldname the name of the field to access
     *  \returns the current value of the named field
     */
    Object getField(const Sirikata::String& fieldname) const;

    /** Set the static field with the given name for this object to the specified value.
     *  \param fieldname the name of the field to set
     *  \param value the new value for the field
     */
    void setField(const Sirikata::String& fieldname, const Object& value) const;

private:
    friend class Object;
    friend class Domain;

    /** Return the internal Mono representation of this Class. */
    MonoClass* klass() const;

    Class();

    MonoClass* mClass;
};

} // namespace Mono

#endif //_MONO_CLASS_HPP_
