// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_CLUTTER_RENDERER_HPP_
#define _SIRIKATA_OH_CLUTTER_RENDERER_HPP_

#include <sirikata/oh/Simulation.hpp>
#include <sirikata/core/service/Context.hpp>
#include <clutter/clutter.h>

namespace Sirikata {

/** ClutterRenderer is a Clutter-based renderer for 2D worlds.
 */
class ClutterRenderer : public Simulation {
public:
    // Some setup for Clutter needs to be performed early to ensure
    // threads don't cause any problems.
    static void preinit();

    ClutterRenderer(Network::IOStrandPtr strand);

    // Service Interface
    virtual void start();
    virtual void stop();

    // Invokable Interface
    virtual boost::any invoke(std::vector<boost::any>& params);

private:

    void rendererThread();

    // Invokable handlers
    boost::any invoke_help(std::vector<boost::any>& params);
    boost::any invoke_stage_set_size(std::vector<boost::any>& params);
    boost::any invoke_stage_set_color(std::vector<boost::any>& params);
    boost::any invoke_stage_set_key_focus(std::vector<boost::any>& params);

    boost::any invoke_actor_set_position(std::vector<boost::any>& params);
    boost::any invoke_actor_set_size(std::vector<boost::any>& params);
    boost::any invoke_actor_show(std::vector<boost::any>& params);
    boost::any invoke_actor_destroy(std::vector<boost::any>& params);
    boost::any invoke_actor_on_mouse_enter(std::vector<boost::any>& params);
    boost::any invoke_actor_on_mouse_motion(std::vector<boost::any>& params);
    boost::any invoke_actor_on_mouse_leave(std::vector<boost::any>& params);

    boost::any invoke_rectangle_create(std::vector<boost::any>& params);
    boost::any invoke_rectangle_set_color(std::vector<boost::any>& params);

    boost::any invoke_text_create(std::vector<boost::any>& params);
    boost::any invoke_text_set_color(std::vector<boost::any>& params);
    boost::any invoke_text_set_text(std::vector<boost::any>& params);
    boost::any invoke_text_get_text(std::vector<boost::any>& params);
    boost::any invoke_text_set_font(std::vector<boost::any>& params);
    boost::any invoke_text_set_editable(std::vector<boost::any>& params);
    boost::any invoke_text_set_single_line(std::vector<boost::any>& params);
    boost::any invoke_text_on_activate(std::vector<boost::any>& params);

    boost::any invoke_texture_create_from_file(std::vector<boost::any>& params);

    boost::any invoke_circle_create(std::vector<boost::any>& params);
    boost::any invoke_circle_set_radius(std::vector<boost::any>& params);
    boost::any invoke_circle_set_fill_color(std::vector<boost::any>& params);
    boost::any invoke_circle_set_border_color(std::vector<boost::any>& params);
    boost::any invoke_circle_set_border_width(std::vector<boost::any>& params);

    ClutterActor* get_actor_by_id(int actor_id);

    Network::IOStrandPtr mStrand;

    Thread* mRendererThread;
    ClutterStage* mStage;

    typedef std::tr1::unordered_map<String, std::tr1::function<boost::any(std::vector<boost::any>& params)> > InvokeHandlerMap;
    InvokeHandlerMap mInvokeHandlers;

    typedef std::tr1::unordered_map<int, ClutterActor*> ActorMap;
    int mActorIDSource;
    ActorMap mActors;
};

} // namespace Sirikata

#endif //_SIRIKATA_OH_CLUTTER_RENDERER_HPP_
