/*  Sirikata Graphical Object Host
 *  Light.hpp
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
#ifndef SIRIKATA_GRAPHICS_LIGHT_HPP__
#define SIRIKATA_GRAPHICS_LIGHT_HPP__

#include <oh/LightListener.hpp>
#include "Entity.hpp"
#include <OgreLight.h>

namespace Sirikata {
namespace Graphics {

class Light
    : public Entity,
      public LightListener {
public:
    Light(OgreSystem *scene, const UUID &id)
        : Entity(scene,
                 id,
                 scene->getSceneManager()->createLight(id.readableHexData())) {
    }

    virtual ~Light() {
        mSceneNode->detachAllObjects();
        mScene->getSceneManager()->destroyLight(mId.readableHexData());
    }

    inline Ogre::Light &light() {
        return *static_cast<Ogre::Light*>(mOgreObject);
    }

    float computeClosestPower(
            const Color &target,
            const Color &source) {
      //minimize sqrt((source.r*power - target.r)^2 + (source.g*power - target.g) + (source.b*power - target.b));
      return source.dot(target) / source.lengthSquared();
    }

    virtual void notify(const LightInfo& linfo){
        float32 ambientPower, shadowPower;
        ambientPower = computeClosestPower(linfo.mDiffuseColor,
            linfo.mAmbientColor * linfo.mPower);
        shadowPower = computeClosestPower(linfo.mSpecularColor,
            linfo.mShadowColor * linfo.mPower);
        Ogre::ColourValue diffuse_ambient (
            toOgreRGBA(linfo.mDiffuseColor, ambientPower));
        light().setDiffuseColour(diffuse_ambient);

        Ogre::ColourValue specular_shadow (
            toOgreRGBA(linfo.mSpecularColor, shadowPower));
        light().setSpecularColour(specular_shadow);

        light().setPowerScale(linfo.mPower);
        switch (linfo.mType) {
          case LightInfo::POINT:
            light().setType(Ogre::Light::LT_POINT);
            break;
          case LightInfo::SPOTLIGHT:
            light().setType(Ogre::Light::LT_SPOTLIGHT);
            break;
          case LightInfo::DIRECTIONAL:
            light().setType(Ogre::Light::LT_DIRECTIONAL);
            break;
          default:break;
        }
        if (linfo.mType!=LightInfo::DIRECTIONAL) {
            light().setAttenuation(
                linfo.mLightRange,
                linfo.mConstantFalloff,
                linfo.mLinearFalloff,
                linfo.mQuadraticFalloff);
        }
        if (linfo.mType==LightInfo::SPOTLIGHT) {
            light().setSpotlightRange(
                Ogre::Radian(linfo.mConeInnerRadians),
                Ogre::Radian(linfo.mConeOuterRadians),
                linfo.mConeFalloff);
        }
        light().setCastShadows(linfo.mCastsShadow);
    }
};

}
}

#endif
