/*  Sirikata Utilities -- Plugin Manager
 *  PluginManager.hpp
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

#ifndef _SIRIKATA_PLUGIN_MANAGER_HPP_
#define _SIRIKATA_PLUGIN_MANAGER_HPP_

#include <sirikata/core/util/Plugin.hpp>

namespace Sirikata {

/** PluginManager handles loading and unloading plugins, directory searching,
 *  and reference counts.
 */
class SIRIKATA_EXPORT PluginManager {
public:
    PluginManager();
    ~PluginManager();

    /** Search the given path for new plugins, automatically loading and
     *  initializing them. */
    void searchPath(const String& path);

    /** Load a specific plugin from the specified file. */
    void load(const String& filename);

    /** Perform garbage collection on plugins, unloading any currently unused
     *  plugins.  */
    void gc();

private:
    struct PluginInfo {
        String filename; // filename to load from
        String name; // name derived from filename, for easier, platform neutral reference
        Plugin* plugin;
    };

    typedef std::list<PluginInfo*> PluginInfoList;
    PluginInfoList mPlugins;
}; // class PluginManager

} // namespace Sirikata

#endif //_SIRIKATA_PLUGIN_MANAGER_HPP_
