/*  Sirikata - Mono Embedding
 *  MonoException.hpp
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
#ifndef _MONO_EXCEPTION_HPP_
#define _MONO_EXCEPTION_HPP_

#include "MonoObject.hpp"
#include <ostream>
#include <string>

namespace Mono {

/** A wrapper class for Mono exceptions which provides similar functionality
 *  to the Exception class within Mono, e.g. get the backtrace and message.
 */
class Exception {
public:
    /** Construct an exception from a Mono object which *must* be of type Exception.
     *  \param orig the original Mono exception object
     */
    explicit Exception(const Object& orig);

    ~Exception();

    /** Get this Exception's message. */
    std::string message() const;
    /** Get this Exception's backtrace. */
    std::string backtrace() const;
    /** Get the original Mono Exception object. */
    Object original() const;

    /** Print this exception's information to the specified stream.
     *  \param os the stream to output to
     */
    void print(std::ostream& os) const;


    /** Create a new NullReferenceException. */
    static Exception NullReference();

    /** Create a new MissingMethodException.
     *  \param klass the class where the lookup was performed
     *  \param method_name the name of the method attempted to call
     *  \returns new MissingMethodException
     */
    static Exception MissingMethod(Class klass, const std::string& method_name);

    /** Create a new MissingPropertyException.
     *  \param klass the class where the lookup was performed
     *  \param prop_name the name of the property attempted to lookup
     *  \returns new MissingPropertyException
     */
    static Exception MissingProperty(Class klass, const std::string& prop_name);

    /** Create a new MissingFieldException.
     *  \param klass the class where the lookup was performed
     *  \param field_name the name of the field attempted to lookup
     *  \returns new MissingFieldException.
     */
    static Exception MissingField(Class klass, const std::string& field_name);

    /** Create a new ClassNotFoundException.
     *  \param klass_name the name of the class that couldn't be found
     *  \param assembly_name the name of the assembly in which the search was performed
     *  \returns new ClassNotFoundException
     */
    static Exception ClassNotFound(const std::string& klass_name, const std::string& assembly_name);

    /** Create a new ClassNotFoundException.
     *  \param name_space the namespace that was searched
     *  \param klass_name the name of the class that couldn't be found
     *  \param assembly_name the name of the assembly in which the search was performed
     *  \returns new ClassNotFoundException
     */
    static Exception ClassNotFound(const std::string& name_space, const std::string& klass_name, const std::string& assembly_name);
private:
    Exception();

    Object mOriginal;
};

std::ostream& operator<<(std::ostream& os, Exception& exc);

} // namespace Mono

#endif //_MONO_EXCEPTION_HPP_
