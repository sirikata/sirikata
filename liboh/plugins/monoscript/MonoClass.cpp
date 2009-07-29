/*  Sirikata - Mono Embedding
 *  MonoClass.cpp
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

#include "MonoClass.hpp"
#include "MonoObject.hpp"
#include "MonoUtil.hpp"
#include "MonoDomain.hpp"
#include "MonoException.hpp"
#include "MonoAssembly.hpp"
#include "MonoMethodLookupCache.hpp"
#include "MonoPropertyLookupCache.hpp"

#include <mono/metadata/appdomain.h>

namespace Mono {

/** Should only be called internally for class caches. */
Class::Class()
 : mClass(NULL)
{
}

//#####################################################################
// Function Class
//#####################################################################
Class::Class(MonoClass* klass)
    : mClass(klass)
{
    assert(klass != NULL);
}

//#####################################################################
// Function Class
//#####################################################################
Class::Class(const Class& cpy)
    : mClass(cpy.mClass)
{
}

//#####################################################################
// Function ~Class
//#####################################################################
Class::~Class() {
}

//#####################################################################
// Function klass
//#####################################################################
MonoClass* Class::klass() const {
    return mClass;
}

//#####################################################################
// Function valuetype
//#####################################################################
bool Class::valuetype() const {
    return mono_class_is_valuetype(mClass);
}

//#####################################################################
// Function assembly
//#####################################################################
Assembly Class::assembly() const {
    MonoImage* image = mono_class_get_image(mClass);
    return Assembly(mono_image_get_assembly(image));
}

//#####################################################################
// Function instance
//#####################################################################
Object Class::instance() const {
    MonoDomain* domain = mono_domain_get();
    Object retval(mono_object_new(domain, mClass));
    retval.send(".ctor");
    return retval;
}

//#####################################################################
// Function instance
//#####################################################################
Object Class::instance(const Object& p1) const {
    MonoDomain* domain = mono_domain_get();
    Object retval(mono_object_new(domain, mClass));
    retval.send(".ctor", p1);
    return retval;
}

//#####################################################################
// Function instance
//#####################################################################
Object Class::instance(const Object& p1, const Object& p2) const {
    MonoDomain* domain = mono_domain_get();
    Object retval(mono_object_new(domain, mClass));
    retval.send(".ctor", p1, p2);
    return retval;
}

//#####################################################################
// Function instance
//#####################################################################
Object Class::instance(const Object& p1, const Object& p2, const Object& p3) const {
    MonoDomain* domain = mono_domain_get();
    Object retval(mono_object_new(domain, mClass));
    retval.send(".ctor", p1, p2, p3);
    return retval;
}

//#####################################################################
// Function instance
//#####################################################################
Object Class::instance(const Object& p1, const Object& p2, const Object& p3, const Object& p4) const {
    MonoDomain* domain = mono_domain_get();
    Object retval(mono_object_new(domain, mClass));
    retval.send(".ctor", p1, p2, p3, p4);
    return retval;
}

//#####################################################################
// Function instance
//#####################################################################
Object Class::instance(const Object& p1, const Object& p2, const Object& p3, const Object& p4, const Object& p5) const {
    MonoDomain* domain = mono_domain_get();
    Object retval(mono_object_new(domain, mClass));
    retval.send(".ctor", p1, p2, p3, p4, p5);
    return retval;
}

//#####################################################################
// Function getName
//#####################################################################
std::string Class::getName() const {
    const char* name = mono_class_get_name(mClass);
    return std::string(name);
}

//#####################################################################
// Function getNamespace
//#####################################################################
std::string Class::getNamespace() const {
    const char* name_space = mono_class_get_namespace(mClass);
    return std::string(name_space);
}



Object Class::send(const Sirikata::String& message) const {
    return send(NULL, message);
}

Object Class::send(const Sirikata::String& message, const Object& p1) const {
    return send(NULL, message, p1);
}

Object Class::send(const Sirikata::String& message, const Object& p1, const Object& p2) const {
    return send(NULL, message, p1, p2);
}

Object Class::send(const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3) const {
    return send(NULL, message, p1, p2, p3);
}

//#####################################################################
// Function send
//#####################################################################
Object Class::send(MethodLookupCache* cache, const Sirikata::String& message) const {
    MonoObject* args[] = { NULL };
    return send_base(mClass, message.c_str(), args, 0, cache);
}

//#####################################################################
// Function send
//#####################################################################
Object Class::send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1) const {
    MonoObject* args[] = { p1.object(), NULL };
    return send_base(mClass, message.c_str(), args, 1, cache);
}

//#####################################################################
// Function send
//#####################################################################
Object Class::send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2) const {
    MonoObject* args[] = { p1.object(), p2.object(), NULL };
    return send_base(mClass, message.c_str(), args, 2, cache);
}

//#####################################################################
// Function send
//#####################################################################
Object Class::send(MethodLookupCache* cache, const Sirikata::String& message, const Object& p1, const Object& p2, const Object& p3) const {
    MonoObject* args[] = { p1.object(), p2.object(), p3.object(), NULL };
    return send_base(mClass, message.c_str(), args, 3, cache);
}

//#####################################################################
// Function getProperty
//#####################################################################
Object Class::getProperty(const Sirikata::String& propname) const {
    assert(mClass != NULL);

    MonoProperty* prop = mono_class_get_property_from_name( mClass, propname.c_str() );
    if (prop == NULL)
        throw Exception::MissingProperty(Class(mClass), propname);

    MonoMethod* prop_get_method = mono_property_get_get_method(prop);
    if (prop_get_method == NULL)
        throw Exception::MissingMethod(*this, propname);

    MonoObject* args[] = { NULL };
    return send_method(NULL, prop_get_method, args, 0);
}

//#####################################################################
// Function getProperty
//#####################################################################
Object Class::getProperty(PropertyLookupCache* cache, const Sirikata::String& propname) const {
    assert(mClass != NULL);

    MonoMethod* prop_get_method = NULL;
    if (cache != NULL)
        prop_get_method = cache->lookupGet(mClass, propname.c_str(), NULL, 0);

    // if cache fails, do full lookup
    if (prop_get_method == NULL) {
        MonoProperty* prop = mono_class_get_property_from_name( mClass, propname.c_str() );
        if (prop == NULL)
            throw Exception::MissingProperty(Class(mClass), propname);

        prop_get_method = mono_property_get_get_method(prop);
        if (prop_get_method == NULL)
            throw Exception::MissingMethod(*this, propname);

        if (cache != NULL)
            cache->updateGet(mClass, propname.c_str(), NULL, 0, prop_get_method);
    }

    MonoObject* args[] = { NULL };
    return send_method(NULL, prop_get_method, args, 0);
}

//#####################################################################
// Function getField
//#####################################################################
Object Class::getField(const Sirikata::String& fieldname) const {
    assert(mClass != NULL);

    MonoClassField* field = mono_class_get_field_from_name( mClass, fieldname.c_str() );
    if (field == NULL)
        throw Exception::MissingField(Class(mClass), fieldname);

    // FIXME might need to customize domain
    return Object(mono_field_get_value_object(Domain::root().domain(), field, NULL));
}

//#####################################################################
// Function setField
//#####################################################################
void Class::setField(const Sirikata::String& fieldname, const Object& value) const {
    assert(mClass != NULL);

    MonoClassField* field = mono_class_get_field_from_name( mClass, fieldname.c_str() );
    if (field == NULL)
        throw Exception::MissingField(Class(mClass), fieldname);

    MonoClass* receive_klass = mono_class_from_mono_type( mono_field_get_type(field) );
    // FIXME might want to customize domain
    MonoVTable* vtable = mono_class_vtable (Domain::root().domain(), mono_field_get_parent(field));
    mono_field_static_set_value(vtable, field, object_to_raw(receive_klass, value.object()));
}

} // namespace Mono
