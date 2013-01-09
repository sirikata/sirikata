// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ClutterRenderer.hpp"
#include <clutter/clutter.h>

#define TWLOG(level,msg) SILOG(clutter-renderer,level,msg)

namespace Sirikata {

namespace {
gboolean delayed_clutter_quit(gpointer /*data*/) {
    clutter_main_quit();
    return FALSE;
}
}

void ClutterRenderer::preinit() {
    g_thread_init(NULL);
    clutter_threads_init();
}

ClutterRenderer::ClutterRenderer()
 : mRendererThread(NULL)
{
}

void ClutterRenderer::start() {
    TWLOG(debug, "Starting ClutterRenderer, initializing renderer thread.");
    mRendererThread = new Sirikata::Thread("ClutterRenderer", std::tr1::bind(&ClutterRenderer::rendererThread, this));
}

void ClutterRenderer::stop() {
    if (mRendererThread) {
        TWLOG(debug, "Stopping ClutterRenderer. Signaling quit to clutter thread and waiting for thread to exit.");
        clutter_threads_add_idle(delayed_clutter_quit, NULL);

        mRendererThread->join();
        delete mRendererThread;
        TWLOG(debug, "ClutterRenderer Clutter thread stopped.");
    }
}

void ClutterRenderer::rendererThread() {
    // Dummy args because clutter doesn't let you init without them
    int argc = 0;
    char** argv = NULL;
    clutter_init(&argc, &argv);

    clutter_threads_enter();

    ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };

    ClutterActor *stage = clutter_stage_get_default ();
    clutter_actor_set_size (stage, 200, 200);
    clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

    clutter_actor_show (stage);

    clutter_main();

    clutter_threads_leave();
}

boost::any ClutterRenderer::invoke(std::vector<boost::any>& params) {
    // Decode the command. First argument is the "function name"
    if (params.empty() || !Invokable::anyIsString(params[0]))
        return boost::any();

    String name = Invokable::anyAsString(params[0]);

    if (name == "help") {
        return Invokable::asAny(String("ClutterRenderer is a 2D renderer for Clutter data associated with objects."));
    }

    return boost::any();
}


} // namespace Sirikata
