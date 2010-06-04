/*  Sirikata Object Host
 *  LightInfo.cpp
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

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/proxyobject/LightInfo.hpp>
#include <ProxyObject_Sirikata.pbj.hpp>

namespace Sirikata {

LightInfo::LightInfo(const Protocol::LightInfoProperty&msg) :
        mWhichFields(0),
        mDiffuseColor(1.0f,1.0f,1.0f),
        mSpecularColor(1.0f,1.0f,1.0f),
        mPower(1.0f),
        mAmbientColor(.05f,.05f,.05f),
        mShadowColor(.02f,.02f,.02f),
        mLightRange(256.0f),
        mConstantFalloff(1.0f),
        mLinearFalloff(.01f),
        mQuadraticFalloff(.02f),
        mConeInnerRadians(0.0f),
        mConeOuterRadians(3.1415926536f),
        mConeFalloff(0.0f),
        mType(POINT),
        mCastsShadow(true)
{
    mWhichFields = 0;
    if (msg.has_diffuse_color()) {
        mDiffuseColor = msg.diffuse_color();
        mWhichFields |= DIFFUSE_COLOR;
    }
    if (msg.has_ambient_color()) {
        mAmbientColor = msg.ambient_color();
        mWhichFields |= AMBIENT_COLOR;
    }
    if (msg.has_specular_color()) {
        mSpecularColor = msg.specular_color();
        mWhichFields |= SPECULAR_COLOR;
    }
    if (msg.has_shadow_color()) {
        mShadowColor = msg.shadow_color();
        mWhichFields |= SHADOW_COLOR;
    }
    if (msg.has_power()) {
        mPower = msg.power();
        mWhichFields |= POWER;
    }
    if (msg.has_light_range()) {
        mLightRange = msg.light_range();
        mWhichFields |= LIGHT_RANGE;
    }
    if (msg.has_constant_falloff() || msg.has_linear_falloff() || msg.has_quadratic_falloff()) {
        mConstantFalloff=msg.constant_falloff();
        mLinearFalloff=msg.linear_falloff();
        mQuadraticFalloff=msg.quadratic_falloff();
        mWhichFields|=FALLOFF;
    }
    if (msg.has_cone_inner_radians() || msg.has_cone_outer_radians() || msg.has_cone_falloff()) {
        mConeInnerRadians=msg.cone_inner_radians();
        mConeOuterRadians=msg.cone_outer_radians();
        mConeFalloff=msg.cone_falloff();
        mWhichFields|=CONE;
    }
    if (msg.has_type()) {
        mWhichFields |= TYPE;
        switch (msg.type()) {
          case Protocol::LightInfoProperty::POINT:
            mType = POINT;
            break;
          case Protocol::LightInfoProperty::SPOTLIGHT:
            mType = SPOTLIGHT;
            break;
          case Protocol::LightInfoProperty::DIRECTIONAL:
            mType = DIRECTIONAL;
            break;
          default:
            mType = LightTypes();
            mWhichFields &= (~TYPE);
            break;
        }
    }
    if (msg.has_casts_shadow()) {
        mCastsShadow=msg.casts_shadow();
        mWhichFields|=CAST_SHADOW;
    }
}

void LightInfo::toProtocol(Protocol::LightInfoProperty &msg) const {
    if (mWhichFields & DIFFUSE_COLOR) {
        msg.set_diffuse_color(mDiffuseColor);
    }
    if (mWhichFields & AMBIENT_COLOR) {
        msg.set_ambient_color(mAmbientColor);
    }
    if (mWhichFields & SPECULAR_COLOR) {
        msg.set_specular_color(mSpecularColor);
    }
    if (mWhichFields & SHADOW_COLOR) {
        msg.set_shadow_color(mShadowColor);
    }
    if (mWhichFields & POWER) {
        msg.set_power(mPower);
    }
    if (mWhichFields & LIGHT_RANGE) {
        msg.set_light_range(mLightRange);
    }
    if (mWhichFields & FALLOFF) {
        msg.set_constant_falloff(mConstantFalloff);
        msg.set_linear_falloff(mLinearFalloff);
        msg.set_quadratic_falloff(mQuadraticFalloff);
    }
    if (mWhichFields & CONE) {
        msg.set_cone_inner_radians(mConeInnerRadians);
        msg.set_cone_outer_radians(mConeOuterRadians);
        msg.set_cone_falloff(mConeFalloff);
    }
    if (mWhichFields & TYPE) {
        switch (mType) {
          case POINT:
            msg.set_type(Protocol::LightInfoProperty::POINT);
            break;
          case SPOTLIGHT:
            msg.set_type(Protocol::LightInfoProperty::SPOTLIGHT);
            break;
          case DIRECTIONAL:
            msg.set_type(Protocol::LightInfoProperty::DIRECTIONAL);
            break;
          default:
            break;
        }
    }
    if (mWhichFields & CAST_SHADOW) {
        msg.set_casts_shadow(mCastsShadow);
    }
}

}
