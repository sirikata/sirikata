/*  Sirikata Graphical Object Host
 *  LightEntity.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
#ifndef SIRIKATA_GRAPHICS_LIGHTENTITY_HPP__
#define SIRIKATA_GRAPHICS_LIGHTENTITY_HPP__

#include <oh/LightListener.hpp>
#include "Entity.hpp"
#include <OgreLight.h>
#include <oh/ProxyLightObject.hpp>
namespace Sirikata {
namespace Graphics {

class LightEntity
    : public Entity,
      public LightListener {
public:
    ProxyLightObject &getProxy() const {
        return *std::tr1::static_pointer_cast<ProxyLightObject>(mProxy);
    }
    LightEntity(OgreSystem *scene, const std::tr1::shared_ptr<ProxyLightObject> &plo, const std::string &id=std::string());

    virtual ~LightEntity();

    inline Ogre::Light *getOgreLight() {
        return static_cast<Ogre::Light*>(mOgreObject);
    }

    static float computeClosestPower(
            const Color &source,
            const Color &target,
            float32 power);

    virtual void notify(const LightInfo& linfo);
    static std::string ogreLightName(const SpaceObjectReference&ref);
    virtual std::string ogreMovableName() const;
};

}
}

#endif
