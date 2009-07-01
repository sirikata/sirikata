/*  Sirikata Graphical Object Host
 *  LightEntity.cpp
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

#include <oh/Platform.hpp>
#include <oh/LightListener.hpp>
#include "LightEntity.hpp"
#include <OgreLight.h>
#include <oh/ProxyLightObject.hpp>

namespace Sirikata {
namespace Graphics {

LightEntity::LightEntity(OgreSystem *scene, const std::tr1::shared_ptr<ProxyLightObject> &plo, const std::string &id)
    : Entity(scene,
             plo,
             id.length()?id:ogreLightName(plo->getObjectReference()),
             scene->getSceneManager()->createLight(id.length()?id:ogreLightName(plo->getObjectReference()))) {
    getProxy().LightProvider::addListener(this);
}

LightEntity::~LightEntity() {    
    Ogre::Light *toDestroy=getOgreLight();
    init(NULL);
    mScene->getSceneManager()->destroyLight(toDestroy);
    getProxy().LightProvider::removeListener(this);
}

float LightEntity::computeClosestPower(
        const Color &source,
        const Color &target,
        float32 power) {
    //minimize sqrt((source.r*power - target.r)^2 + (source.g*power - target.g) + (source.b*power - target.b));
    return power * source.dot(target) / source.length();
}

void LightEntity::notify(const LightInfo& linfo){
    float32 ambientPower, shadowPower;
    ambientPower = computeClosestPower(linfo.mDiffuseColor, linfo.mAmbientColor, linfo.mPower);
    shadowPower = computeClosestPower(linfo.mSpecularColor, linfo.mShadowColor,  linfo.mPower);
    Ogre::ColourValue diffuse_ambient (
        toOgreRGBA(linfo.mDiffuseColor, ambientPower));
    getOgreLight()->setDiffuseColour(diffuse_ambient);
    
    Ogre::ColourValue specular_shadow (
        toOgreRGBA(linfo.mSpecularColor, shadowPower));
    getOgreLight()->setSpecularColour(specular_shadow);
    
    getOgreLight()->setPowerScale(linfo.mPower);
    switch (linfo.mType) {
      case LightInfo::POINT:
        getOgreLight()->setType(Ogre::Light::LT_POINT);
        break;
      case LightInfo::SPOTLIGHT:
        getOgreLight()->setType(Ogre::Light::LT_SPOTLIGHT);
        break;
      case LightInfo::DIRECTIONAL:
        getOgreLight()->setType(Ogre::Light::LT_DIRECTIONAL);
        break;
      default:break;
    }
    if (linfo.mType!=LightInfo::DIRECTIONAL) {
        getOgreLight()->setAttenuation(
            linfo.mLightRange,
            linfo.mConstantFalloff,
            linfo.mLinearFalloff,
            linfo.mQuadraticFalloff);
    }
    if (linfo.mType==LightInfo::SPOTLIGHT) {
        getOgreLight()->setSpotlightRange(
            Ogre::Radian(linfo.mConeInnerRadians),
            Ogre::Radian(linfo.mConeOuterRadians),
            linfo.mConeFalloff);
    }
    getOgreLight()->setCastShadows(linfo.mCastsShadow);
}
std::string LightEntity::ogreLightName(const SpaceObjectReference&ref) {
    return "Light:"+ref.toString();
}
std::string LightEntity::ogreMovableName()const{
    return ogreLightName(id());
}

}
}
