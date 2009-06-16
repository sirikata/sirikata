/*  Sirikata Utilities -- Plugin Manager
 *  PluginManager.cpp
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
#include "util/Standard.hh"
#include "PluginManager.hpp"
#include <boost/filesystem.hpp>

namespace Sirikata {

/** Get a listing of all the files (no directories) in the given path. */
static std::list<String> getDirectoryFiles(const String& dirpath) {
    using namespace boost::filesystem;

    std::list<String> files;

    path dir = dirpath;
    if ( !exists(dir) || !is_directory(dir) )
        return files;

    directory_iterator end;
    for(directory_iterator di(dir); di != end; di++) {
        if (is_directory(di->status()))
            continue;
        files.push_back(di->path().string());
    }

    return files;
}


PluginManager::PluginManager() {
}

PluginManager::~PluginManager() {
}

void PluginManager::searchPath(const String& path) {
    std::list<String> files = getDirectoryFiles(path);

    for(std::list<String>::iterator it = files.begin(); it != files.end(); it++) {
        String filename = *it;
        load(filename);
    }
}

void PluginManager::load(const String& filename) {
    Plugin* plugin = new Plugin(filename);

    if (!plugin->load()) {
        std::cout << "dbm: plugin->load FAIL!" << std::endl;
        delete plugin;
        return;
    }
    std::cout << "dbm: plugin->load SUCCESS!" << std::endl;

    plugin->initialize();
    std::cout << "dbm: initialize done" << std::endl;

    PluginInfo* pi = new PluginInfo();
    pi->plugin = plugin;
    pi->filename = filename;
    pi->name = plugin->name();

    mPlugins.push_back(pi);
    std::cout << "dbm: leaving ::load" << std::endl;
}

void PluginManager::gc() {
    for(PluginInfoList::iterator it = mPlugins.begin(); it != mPlugins.end();) {
        PluginInfo* pi = *it;
        if (pi->plugin != NULL && pi->plugin->refcount() <= 1) {
            if (pi->plugin->refcount()==0) {
                pi->plugin->destroy();
                pi->plugin->unload();//unload plugin straight away, no outstanding variables
            }else {
                pi->plugin->destroy();//destroy the plugin, but leave program code for outstanding destructors
            }
            delete pi->plugin;
            pi->plugin = NULL;
            delete pi;
            PluginInfoList::iterator eraseme=it;
            ++it;
            mPlugins.erase(eraseme);
        }else{
            ++it;
        }
    }
}

} // namespace Sirikata
