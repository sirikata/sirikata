// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_CLUTTER_RENDERER_HPP_
#define _SIRIKATA_OH_CLUTTER_RENDERER_HPP_

#include <sirikata/oh/Simulation.hpp>
#include <sirikata/core/service/Context.hpp>

namespace Sirikata {

class HostedObject;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;

/** ClutterRenderer is a Clutter-based renderer for 2D worlds.
 */
class ClutterRenderer : public Simulation {
public:
    // Some setup for Clutter needs to be performed early to ensure
    // threads don't cause any problems.
    static void preinit();

    ClutterRenderer();

    // Service Interface
    virtual void start();
    virtual void stop();

    // Invokable Interface
    virtual boost::any invoke(std::vector<boost::any>& params);

private:

    void rendererThread();

    Thread* mRendererThread;
};

} // namespace Sirikata

#endif //_SIRIKATA_OH_CLUTTER_RENDERER_HPP_
