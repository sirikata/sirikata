/*  Sirikata Object Host Scripting Plugin
 *  MonoSystem.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#ifndef _SIRIKATA_MONO_SYSTEM_HPP_
#define _SIRIKATA_MONO_SYSTEM_HPP_
#include "MonoDefs.hpp"
#include <mono/metadata/mono-gc.h>
namespace Mono {

class MonoSystem {
public:
    MonoSystem();
    ~MonoSystem();

    /** Create a new AppDomain in this virtual machine. */
    Domain createDomain();

    /** List all loaded assemblies for this virtual machine to stdout. For debugging purposes only. */
    void listAssemblies();

    /** Load the assembly with the given name. This can be a full AssemblyName, e.g.
     *  mscorlib, Version=1.0.5000.0, Culture=neutral, PublicKeyToken=b77a5c561934e089.
     *  Returns true if successful, false otherwise.  This doesn't result in any references
     *  to the Assembly, you must use the Assembly with a Domain to create a
     *  reference.
     *  \param name the name of the assembly
     */
    bool loadAssembly(const Sirikata::String& name) const;

    /** Load the assembly with the given name. This can be a full AssemblyName, e.g.
     *  mscorlib, Version=1.0.5000.0, Culture=neutral, PublicKeyToken=b77a5c561934e089.
     *  Returns true if successful, false otherwise.  This doesn't result in any references
     *  to the Assembly, you must use the Assembly with a Domain to create a
     *  reference.
     *  \param name the name of the assembly
     *  \param dir an additional directory to search for the assembly in
     */
    bool loadAssembly(const Sirikata::String& name, const Sirikata::String& dir) const;

    /** Load an assembly from memory.
     *  \param membuf pointer to the raw buffer holding the assembly image
     *  \param len the length of the buffer in bytes
     */
    bool loadMemoryAssembly(char* membuf, unsigned int len) const;

    /** Run the garbage collector.*/
    void GC() const;
private:
    /** Search for the given class in the given namespace in any of the loaded assemblies.
     *  If you know the assembly its in, access it directly as this is inefficient.
     *  \param name_space the namespace to search for the class in
     *  \param klass the name of the class to look for
     */
    Class getClass(const Sirikata::String& name_space, const Sirikata::String& klass);

    Domain mDomain;
    std::vector<Assembly> mAssemblies;
    Sirikata::String mWorkDir;
};

}


#endif
