/*  Sirikata
 *  MeshView.cpp
 *
 *  Copyright (c) 2011, Stanford University
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

#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>

#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/ogre/Ogre.hpp>

int main(int argc, char** argv) {
    using namespace Sirikata;
    using namespace Sirikata::Graphics;

    InitOptions();
    ParseOptions(argc, argv);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS) );

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* iostrand = ios->createStrand();

    OgreRenderer* renderer = new OgreRenderer();

    ios->run();

    delete renderer;

    delete iostrand;
    Network::IOServiceFactory::destroyIOService(ios);

    return 0;
}
