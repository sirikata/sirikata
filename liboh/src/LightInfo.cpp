#include <util/Standard.hh>
#include <oh/Platform.hpp>
#include "oh/LightInfo.hpp"
#include <ObjectHost_Sirikata.pbj.hpp>

namespace Sirikata {

LightInfo::LightInfo(const Protocol::LightInfoProperty&msg) :
        mWhichFields(0),
        mDiffuseColor(1,1,1),
        mSpecularColor(1,1,1),
        mPower(1),
        mAmbientColor(.05,.05,.05),
        mShadowColor(.02,.02,.02),
        mLightRange(256),
        mConstantFalloff(1.0),
        mLinearFalloff(.01),
        mQuadraticFalloff(.02),
        mConeInnerRadians(0),
        mConeOuterRadians(3.1415926536),
        mConeFalloff(0),
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
