// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/proxyobject/Simulation.hpp>
#include <sirikata/core/service/Context.hpp>

namespace Sirikata {
namespace SDL {

class AudioSimulation : public Simulation {
public:
    AudioSimulation(Context* ctx);


    // Service Interface
    virtual void start();
    virtual void stop();

    // Invokable Interface
    virtual boost::any invoke(std::vector<boost::any>& params);

private:
    Context* mContext;
    bool mInitializedAudio;
    bool mOpenedAudio;
};

} //namespace SDL
} //namespace Sirikata
