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
#include <sirikata/ogre/OgreRenderer.hpp>
#include <sirikata/ogre/Camera.hpp>
#include <sirikata/ogre/Entity.hpp>

#include <sirikata/core/trace/Trace.hpp>

using namespace Sirikata;
using namespace Sirikata::Graphics;

class MeshViewCamera : public Graphics::Camera {
public:
  MeshViewCamera(OgreRenderer* scene)
    : Graphics::Camera(scene, "MeshView") // Only support 1 camera
  {}
    virtual ~MeshViewCamera() {}

  virtual Vector3d getGoalPosition() { return Vector3d(0, 0, 2); }
  virtual Quaternion getGoalOrientation() { return Quaternion::identity(); }
  virtual BoundingSphere3f getGoalBounds() { return BoundingSphere3f(Vector3f(0, 0, 0), 1.f); }

};

class MeshViewEntity : public Graphics::Entity, public Graphics::EntityListener {
public:
    MeshViewEntity(OgreRenderer* scene, const String& screenshot)
     : Graphics::Entity(scene, "MeshViewEntity"), // Only support 1 entity
       mScreenshot(screenshot)
    {
        this->addListener(this);
    }
    virtual ~MeshViewEntity() {}

    // Entity Interface
    virtual BoundingSphere3f bounds() { return BoundingSphere3f(Vector3f(0,0,0), 1.f); }
    virtual float32 priority() { return 1.f; }

    // Entity Listener Interface
    virtual void entityLoaded(Entity* ent, bool success) {
        if (!mScreenshot.empty()) {
            if (success)
                mScene->screenshotNextFrame(mScreenshot);
            else
                SILOG(meshview,error, "Failed to load mesh.");
            mScene->quit();
        }
    }
private:
    String mScreenshot;
};

int main(int argc, char** argv) {
    InitOptions();

    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue("mesh","",Sirikata::OptionValueType<String>(),"Mesh to load and display."))
        .addOption(new OptionValue("screenshot","",Sirikata::OptionValueType<String>(),"If non-empty, trigger a screenshot to the given filename and exit."))
        ;

    ParseOptions(argc, argv);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS) );
    // FIXME this should be an option
    plugins.loadList( "colladamodels,common-filters,nvtt" );

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* iostrand = ios->createStrand();

    Trace::Trace* trace = new Trace::Trace("meshview.log");
    Time epoch = Timer::now();

    Context* ctx = new Context("MeshView", ios, iostrand, trace, epoch);

    OgreRenderer* renderer = new OgreRenderer(ctx);
    renderer->initialize("", false);

    MeshViewCamera* cam = new MeshViewCamera(renderer);
    cam->initialize();
    cam->attach("", 0, 0, Vector4f(.7,.7,.7,1));

    MeshViewEntity* ent = new MeshViewEntity(renderer, GetOptionValue<String>("screenshot"));
    ent->setOgrePosition(Vector3d(0, 0, 0));
    ent->setOgreOrientation(Quaternion::identity());
    ent->processMesh(Transfer::URI(GetOptionValue<String>("mesh")));

    ctx->add(ctx);
    ctx->add(renderer);
    ctx->run(1);

    delete ent;
    delete cam;
    delete renderer;

    ctx->cleanup();
    trace->prepareShutdown();

    delete ctx;

    trace->shutdown();
    delete trace;

    delete iostrand;
    Network::IOServiceFactory::destroyIOService(ios);

    return 0;
}
