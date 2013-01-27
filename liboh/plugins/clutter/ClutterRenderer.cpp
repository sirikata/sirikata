// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ClutterRenderer.hpp"
#include "clutter-circle.hpp"

#define TWLOG(level,msg) SILOG(clutter-renderer,level,msg)

namespace Sirikata {

namespace {
gboolean delayed_clutter_quit(gpointer /*data*/) {
    clutter_main_quit();
    return FALSE;
}

// Job submitted to clutter which performs a synchronous op in another
// thread. This notifies the waiting thread, then waits to be notified when the
// operation is done.
gboolean delayed_synchronous_op(gpointer data);
struct SyncOpClosure {
    boost::mutex mutex;
    boost::condition_variable cond;
    bool ready;
    bool done;
    bool deletable;

    // Only for the creator, a lock maintained internally to make the usage
    // simple.
    boost::unique_lock<boost::mutex> creator_lock;

    // If you need something executed inside the clutter thread, you
    // can wrap it up and pass it in. Instead of waiting for you to do
    // your work while keeping clutter blocked, we'll execute your
    // code for you
    std::tr1::function<void()> work;

    // Sets up, then waits until other thread is ready for us to do processing
    SyncOpClosure()
     : done(false),
       deletable(false),
       creator_lock(mutex) // acquires lock
    {
        clutter_threads_add_idle(delayed_synchronous_op, this);
        cond.wait(creator_lock);
    }

    // Sets up, then waits until other thread is ready for us to do processing
    SyncOpClosure(std::tr1::function<void()> work_)
     : done(false),
       deletable(false),
       creator_lock(mutex), // acquires lock
       work(work_)
    {
        clutter_threads_add_idle(delayed_synchronous_op, this);
        cond.wait(creator_lock);
    }

    // Owner indicates they are finished
    void finish() {
        done = true;
        cond.notify_one();
        // Need to unlock this explicitly to free the mutex since we don't use
        // the caller's scope to manage the lock
        creator_lock.unlock();
        deletable = true;
    }
};
gboolean delayed_synchronous_op(gpointer data) {
    SyncOpClosure* closure = static_cast<SyncOpClosure*>(data);
    {
        // Wait until we see the other thread finish
        boost::unique_lock<boost::mutex> lock(closure->mutex);
        // If they specified some work to do, do it for them
        if (closure->work)
            closure->work();
        // Indicate that the other thread can continue. In lock since this can't
        // proceed without other thread and it makes it simpler to ensure the other
        // thread gets the notification.
        closure->cond.notify_one();
        while(!closure->done)
            closure->cond.wait(lock);
    }
    while(!closure->deletable)
        continue;
    delete closure;
    return FALSE;
}
}

void ClutterRenderer::preinit() {
    g_thread_init(NULL);
    clutter_threads_init();
}

ClutterRenderer::ClutterRenderer()
 : mRendererThread(NULL),
   mStage(NULL),
   mActorIDSource(1)
{
    // Register Invokable handlers
    mInvokeHandlers["help"] = std::tr1::bind(&ClutterRenderer::invoke_help, this, _1);
    mInvokeHandlers["stage_set_size"] = std::tr1::bind(&ClutterRenderer::invoke_stage_set_size, this, _1);
    mInvokeHandlers["stage_set_color"] = std::tr1::bind(&ClutterRenderer::invoke_stage_set_color, this, _1);
    mInvokeHandlers["stage_set_key_focus"] = std::tr1::bind(&ClutterRenderer::invoke_stage_set_key_focus, this, _1);

    mInvokeHandlers["actor_set_position"] = std::tr1::bind(&ClutterRenderer::invoke_actor_set_position, this, _1);
    mInvokeHandlers["actor_set_size"] = std::tr1::bind(&ClutterRenderer::invoke_actor_set_size, this, _1);
    mInvokeHandlers["actor_show"] = std::tr1::bind(&ClutterRenderer::invoke_actor_show, this, _1);
    mInvokeHandlers["actor_destroy"] = std::tr1::bind(&ClutterRenderer::invoke_actor_destroy, this, _1);

    mInvokeHandlers["rectangle_create"] = std::tr1::bind(&ClutterRenderer::invoke_rectangle_create, this, _1);
    mInvokeHandlers["rectangle_set_color"] = std::tr1::bind(&ClutterRenderer::invoke_rectangle_set_color, this, _1);

    mInvokeHandlers["text_create"] = std::tr1::bind(&ClutterRenderer::invoke_text_create, this, _1);
    mInvokeHandlers["text_set_color"] = std::tr1::bind(&ClutterRenderer::invoke_text_set_color, this, _1);
    mInvokeHandlers["text_set_text"] = std::tr1::bind(&ClutterRenderer::invoke_text_set_text, this, _1);
    mInvokeHandlers["text_get_text"] = std::tr1::bind(&ClutterRenderer::invoke_text_get_text, this, _1);
    mInvokeHandlers["text_set_font"] = std::tr1::bind(&ClutterRenderer::invoke_text_set_font, this, _1);
    mInvokeHandlers["text_set_editable"] = std::tr1::bind(&ClutterRenderer::invoke_text_set_editable, this, _1);
    mInvokeHandlers["text_set_single_line"] = std::tr1::bind(&ClutterRenderer::invoke_text_set_single_line, this, _1);
    mInvokeHandlers["text_on_activate"] = std::tr1::bind(&ClutterRenderer::invoke_text_on_activate, this, _1);

    mInvokeHandlers["texture_create_from_file"] = std::tr1::bind(&ClutterRenderer::invoke_texture_create_from_file, this, _1);

    mInvokeHandlers["circle_create"] = std::tr1::bind(&ClutterRenderer::invoke_circle_create, this, _1);
    mInvokeHandlers["circle_set_radius"] = std::tr1::bind(&ClutterRenderer::invoke_circle_set_radius, this, _1);
    mInvokeHandlers["circle_set_fill_color"] = std::tr1::bind(&ClutterRenderer::invoke_circle_set_fill_color, this, _1);
    mInvokeHandlers["circle_set_border_color"] = std::tr1::bind(&ClutterRenderer::invoke_circle_set_border_color, this, _1);
    mInvokeHandlers["circle_set_border_width"] = std::tr1::bind(&ClutterRenderer::invoke_circle_set_border_width, this, _1);
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
    mStage = CLUTTER_STAGE (stage);
    clutter_actor_set_size (stage, 200, 200);
    clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

    clutter_actor_show (stage);

    clutter_main();

    clutter_threads_leave();
}

ClutterActor* ClutterRenderer::get_actor_by_id(int actor_id) {
    if (mActors.find(actor_id) == mActors.end()) return NULL;
    return mActors[actor_id];
}


namespace {
// clutter_color_init isn't available on all versions of clutter
void clutter_color_init_(ClutterColor* c, guint8 r, guint8 g, guint8 b, guint8 a) {
    c->red = r;
    c->green = g;
    c->blue = b;
    c->alpha = a;
}
}



boost::any ClutterRenderer::invoke(std::vector<boost::any>& params) {
    // Decode the command. First argument is the "function name"
    if (params.empty() || !Invokable::anyIsString(params[0]))
        return boost::any();

    String name = Invokable::anyAsString(params[0]);
    InvokeHandlerMap::iterator it = mInvokeHandlers.find(name);
    if (it == mInvokeHandlers.end()) return boost::any();

    std::vector<boost::any> subparams(params.begin()+1, params.end());
    return it->second(subparams);
}

boost::any ClutterRenderer::invoke_help(std::vector<boost::any>& params) {
    return Invokable::asAny(String("ClutterRenderer is a 2D renderer for Clutter data associated with objects."));
}

boost::any ClutterRenderer::invoke_stage_set_size(std::vector<boost::any>& params) {
    assert(params.size() == 2 &&
        Invokable::anyIsNumeric(params[0]) && // x
        Invokable::anyIsNumeric(params[1]) // y
    );

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_actor_set_size (CLUTTER_ACTOR(mStage), Invokable::anyAsNumeric(params[0]), Invokable::anyAsNumeric(params[1]));

    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_stage_set_color(std::vector<boost::any>& params) {
    assert((params.size() == 3 || params.size() == 4) &&
        Invokable::anyIsNumeric(params[0]) && // r
        Invokable::anyIsNumeric(params[1]) && // g
        Invokable::anyIsNumeric(params[2]) // b
    );

    SyncOpClosure* closure = new SyncOpClosure;
    ClutterColor color;
    clutter_color_init_(&color, Invokable::anyAsNumeric(params[0]), Invokable::anyAsNumeric(params[1]), Invokable::anyAsNumeric(params[2]), (params.size() == 4 ? Invokable::anyAsNumeric(params[3]) : 255));
    clutter_stage_set_color(mStage, &color);
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_stage_set_key_focus(std::vector<boost::any>& params) {
    assert(params.size() == 1 &&
        Invokable::anyIsNumeric(params[0]) // actor
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_stage_set_key_focus (mStage, actor);

    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_actor_set_position(std::vector<boost::any>& params) {
    assert(params.size() == 3 &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsNumeric(params[1]) && // x
        Invokable::anyIsNumeric(params[2]) // y
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_actor_set_position (actor, Invokable::anyAsNumeric(params[1]), Invokable::anyAsNumeric(params[2]));
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_actor_set_size(std::vector<boost::any>& params) {
    assert(params.size() == 3 &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsNumeric(params[1]) && // x
        Invokable::anyIsNumeric(params[2]) // y
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_actor_set_size (actor, Invokable::anyAsNumeric(params[1]), Invokable::anyAsNumeric(params[2]));

    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_actor_show(std::vector<boost::any>& params) {
    assert(params.size() == 1 &&
        Invokable::anyIsNumeric(params[0]) // actor_id
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_container_add_actor (CLUTTER_CONTAINER (mStage), actor);
    clutter_actor_show (actor);
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_actor_destroy(std::vector<boost::any>& params) {
    assert(params.size() == 1 &&
        Invokable::anyIsNumeric(params[0]) // actor_id
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    mActors.erase(actor_id);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_actor_destroy(actor);
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_rectangle_create(std::vector<boost::any>& params) {
    int actor_id = mActorIDSource++;
    ClutterColor default_color = { 0x00, 0x00, 0x00, 0xff };

    SyncOpClosure* closure = new SyncOpClosure;
    ClutterActor *rect = clutter_rectangle_new_with_color(&default_color);
    closure->finish();

    mActors[actor_id] = rect;

    return Invokable::asAny(actor_id);
}

boost::any ClutterRenderer::invoke_rectangle_set_color(std::vector<boost::any>& params) {
    assert((params.size() == 4 || params.size() == 5) &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsNumeric(params[1]) && // r
        Invokable::anyIsNumeric(params[2]) && // g
        Invokable::anyIsNumeric(params[3]) // b
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    SyncOpClosure* closure = new SyncOpClosure;
    ClutterColor color;
    clutter_color_init_(&color, Invokable::anyAsNumeric(params[1]), Invokable::anyAsNumeric(params[2]), Invokable::anyAsNumeric(params[3]), (params.size() == 5 ? Invokable::anyAsNumeric(params[4]) : 255));
    clutter_rectangle_set_color(CLUTTER_RECTANGLE(actor), &color);
    closure->finish();

    return Invokable::asAny(true);
}


boost::any ClutterRenderer::invoke_text_create(std::vector<boost::any>& params) {
    int actor_id = mActorIDSource++;

    SyncOpClosure* closure = new SyncOpClosure;
    ClutterActor *text = clutter_text_new();
    closure->finish();

    mActors[actor_id] = text;

    return Invokable::asAny(actor_id);
}

boost::any ClutterRenderer::invoke_text_set_color(std::vector<boost::any>& params) {
    assert((params.size() == 4 || params.size() == 5) &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsNumeric(params[1]) && // r
        Invokable::anyIsNumeric(params[2]) && // g
        Invokable::anyIsNumeric(params[3]) // b
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    SyncOpClosure* closure = new SyncOpClosure;
    ClutterColor color;
    clutter_color_init_(&color, Invokable::anyAsNumeric(params[1]), Invokable::anyAsNumeric(params[2]), Invokable::anyAsNumeric(params[3]), (params.size() == 5 ? Invokable::anyAsNumeric(params[4]) : 255));
    clutter_text_set_color(CLUTTER_TEXT(actor), &color);
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_text_set_text(std::vector<boost::any>& params) {
    assert(params.size() == 2 &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsString(params[1]) // text
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);
    String new_text = Invokable::anyAsString(params[1]);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_text_set_text(CLUTTER_TEXT(actor), new_text.c_str());
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_text_get_text(std::vector<boost::any>& params) {
    assert(params.size() == 1 &&
        Invokable::anyIsNumeric(params[0])// actor_id
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    SyncOpClosure* closure = new SyncOpClosure;
    String text_text(clutter_text_get_text(CLUTTER_TEXT(actor)));
    closure->finish();

    return Invokable::asAny(text_text);
}

boost::any ClutterRenderer::invoke_text_set_font(std::vector<boost::any>& params) {
    assert(params.size() == 2 &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsString(params[1]) // font name
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    String font_name = Invokable::anyAsString(params[1]);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_text_set_font_name(CLUTTER_TEXT(actor), font_name.c_str());
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_text_set_editable(std::vector<boost::any>& params) {
    assert(params.size() == 2 &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsBoolean(params[1]) // editable bool
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    bool is_editable = Invokable::anyAsBoolean(params[1]);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_text_set_editable(CLUTTER_TEXT(actor), (is_editable ? TRUE : FALSE));
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_text_set_single_line(std::vector<boost::any>& params) {
    assert(params.size() == 2 &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsBoolean(params[1]) // single line mode
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    bool is_single_line = Invokable::anyAsBoolean(params[1]);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_text_set_single_line_mode(CLUTTER_TEXT(actor), (is_single_line ? TRUE : FALSE));
    closure->finish();

    return Invokable::asAny(true);
}

static void clutter_text_on_activate_cb(ClutterText* self, gpointer data) {
    Invokable* cb = static_cast<Invokable*>(data);
    cb->invoke();
}

boost::any ClutterRenderer::invoke_text_on_activate(std::vector<boost::any>& params) {
    assert(params.size() == 2 &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsInvokable(params[1]) // callback
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    Invokable* cb = Invokable::anyAsInvokable(params[1]);

    SyncOpClosure* closure = new SyncOpClosure;
    g_signal_connect(G_OBJECT(actor), "activate", G_CALLBACK(clutter_text_on_activate_cb), cb);
    closure->finish();

    return Invokable::asAny(true);
}

namespace {
void allocate_texture(const String& fname, ClutterActor** texture, GError** error) {
    *texture = clutter_texture_new_from_file(fname.c_str(), error);
}
}
boost::any ClutterRenderer::invoke_texture_create_from_file(std::vector<boost::any>& params) {
    assert(params.size() == 1 &&
        Invokable::anyIsString(params[0]) // filename
    );
    String fname = Invokable::anyAsString(params[0]);

    int actor_id = mActorIDSource++;

    ClutterActor *texture;
    GError* error = NULL;
    SyncOpClosure* closure = new SyncOpClosure( std::tr1::bind(&allocate_texture, fname, &texture, &error) );
    closure->finish();

    if (texture == NULL) {
        SILOG(clutter, error, "Error creating texture from " << fname << ": " << error->message);
        g_error_free(error);
        return Invokable::asAny(false);
    }
    mActors[actor_id] = texture;

    return Invokable::asAny(actor_id);
}



boost::any ClutterRenderer::invoke_circle_create(std::vector<boost::any>& params) {
    int actor_id = mActorIDSource++;

    SyncOpClosure* closure = new SyncOpClosure;
    ClutterActor *circle = clutter_circle_new();
    clutter_circle_set_angle_start(CLUTTER_CIRCLE(circle), 0);
    clutter_circle_set_angle_stop(CLUTTER_CIRCLE(circle), 360);
    clutter_circle_set_border_width(CLUTTER_CIRCLE(circle), 1);
    closure->finish();

    mActors[actor_id] = circle;

    return Invokable::asAny(actor_id);
}

boost::any ClutterRenderer::invoke_circle_set_radius(std::vector<boost::any>& params) {
    assert(params.size() == 2 &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsNumeric(params[1]) // radius
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    float32 radius = Invokable::anyAsNumeric(params[1]);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_circle_set_radius(CLUTTER_CIRCLE(actor), (guint)radius);
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_circle_set_fill_color(std::vector<boost::any>& params) {
    assert((params.size() == 4 || params.size() == 5) &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsNumeric(params[1]) && // r
        Invokable::anyIsNumeric(params[2]) && // g
        Invokable::anyIsNumeric(params[3]) // b
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    SyncOpClosure* closure = new SyncOpClosure;
    ClutterColor color;
    clutter_color_init_(&color, Invokable::anyAsNumeric(params[1]), Invokable::anyAsNumeric(params[2]), Invokable::anyAsNumeric(params[3]), (params.size() == 5 ? Invokable::anyAsNumeric(params[4]) : 255));
    clutter_circle_set_color(CLUTTER_CIRCLE(actor), &color);
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_circle_set_border_color(std::vector<boost::any>& params) {
    assert((params.size() == 4 || params.size() == 5) &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsNumeric(params[1]) && // r
        Invokable::anyIsNumeric(params[2]) && // g
        Invokable::anyIsNumeric(params[3]) // b
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    SyncOpClosure* closure = new SyncOpClosure;
    ClutterColor color;
    clutter_color_init_(&color, Invokable::anyAsNumeric(params[1]), Invokable::anyAsNumeric(params[2]), Invokable::anyAsNumeric(params[3]), (params.size() == 5 ? Invokable::anyAsNumeric(params[4]) : 255));
    clutter_circle_set_border_color(CLUTTER_CIRCLE(actor), &color);
    closure->finish();

    return Invokable::asAny(true);
}

boost::any ClutterRenderer::invoke_circle_set_border_width(std::vector<boost::any>& params) {
    assert(params.size() == 2 &&
        Invokable::anyIsNumeric(params[0]) && // actor_id
        Invokable::anyIsNumeric(params[1]) // border width
    );
    int actor_id = Invokable::anyAsNumeric(params[0]);
    ClutterActor* actor = get_actor_by_id(actor_id);
    if (actor == NULL) return Invokable::asAny(false);

    float32 border_width = Invokable::anyAsNumeric(params[1]);

    SyncOpClosure* closure = new SyncOpClosure;
    clutter_circle_set_border_width(CLUTTER_CIRCLE(actor), (guint)border_width);
    closure->finish();

    return Invokable::asAny(true);
}

} // namespace Sirikata
