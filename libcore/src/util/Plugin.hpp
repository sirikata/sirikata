/*  Sirikata Utilities -- Plugin
 *  Plugin.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_PLUGIN_HPP_
#define _SIRIKATA_PLUGIN_HPP_

#include "util/DynamicLibrary.hpp"
#include <list>

namespace Sirikata {

/** Plugin provides loading of plugins to allow for extension of the system.
 *  Plugins are dynamic libraries which register services with the core system.
 *  Generally these will take the form of implementations of services which
 *  are registered with the appropriate factory.  Plugins must provide a simple
 *  interface implemented as a small set of C functions:
 *    void init(); // initialize this plugin, generally just register services
 *    void destroy(); // destroy this plugin, generally just unregister services
 *    const char* name(); // get the name of this plugin
 *    int refcount(); // get the reference count of this plugin
 *  These should be mostly self explanatory.  The reference count is maintained
 *  by the plugin but used by the main system itself.  While a plugin's code is
 *  potentially in use it should ensure that its refcount is greater than 0.
 */
class Plugin {
public:
    Plugin(const String& path);
    ~Plugin();

    /** Loads the plugin, returning true if it satisfies all the plugin requirements,
     *  i.e. if it is successfully loaded from disk and has the appropriate interface.
     */
    bool load();

    /** Forcefully unloads the plugin. Note that this *does not* call the destroy method. */
    bool unload();

    /** Run this plugin's initialization code. */
    void initialize();

    /** Destroy this plugin. */
    void destroy();

    /** Get the name of this plugin. */
    String name();

    /** Get the current refcount of this plugin. */
    int refcount();
private:
    typedef void(*InitFunc)();
    typedef void(*DestroyFunc)();
    typedef const char*(*NameFunc)();
    typedef int(*RefCountFunc)();

    DynamicLibrary mDL;
    InitFunc mInit;
    DestroyFunc mDestroy;
    NameFunc mName;
    RefCountFunc mRefCount;
    int mInitialized;
}; // class Plugin

} // namespace Sirikata

#endif //_SIRIKATA_PLUGIN_HPP_
