/*  Sirikata Utilities -- Sirikata Listener Pattern
 *  LightInfo.hpp
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

#ifndef _SIRIKATA_LIGHT_INFO_HPP_
#define _SIRIKATA_LIGHT_INFO_HPP_

namespace Sirikata {
typedef Vector3f Color;
struct LightInfo {
    enum LightTypes {
        POINT,SPOTLIGHT,DIRECTIONAL,NUM_TYPES//defaults to point=0?
    };
    enum Fields {
        NONE=0,
        DIFFUSE_COLOR=1,
        SPECULAR_COLOR=2,
        POWER=8,
        AMBIENT_COLOR=16,
        SHADOW_COLOR=32,
        LIGHT_RANGE=64,
        FALLOFF=128,
        CONE=256,
        TYPE=512,
        CAST_SHADOW=1024,
        ALL=2047
    };
    int32 mWhichFields;
    LightInfo() :
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
        mCastsShadow(true) {
    }
    Color mDiffuseColor;
    Color mSpecularColor;
    float32 mPower;
    Color mAmbientColor;
    Color mShadowColor;
    float64 mLightRange;
    float32 mConstantFalloff;
    float32 mLinearFalloff;
    float32 mQuadraticFalloff;
    float32 mConeInnerRadians;
    float32 mConeOuterRadians;
    float32 mConeFalloff;
    LightTypes mType;
    bool mCastsShadow;
    LightInfo& setLightDiffuseColor(const Color&c){
        mWhichFields|=DIFFUSE_COLOR;
        mDiffuseColor=c;
        return *this;
    }
    LightInfo& setLightSpecularColor(const Color&c) {
        mWhichFields|=SPECULAR_COLOR;
        mSpecularColor=c;        
        return *this;
    }
    LightInfo& setLightPower(float32 c){
        mWhichFields|=POWER;
        mPower=c;        
        return *this;
    }
    ///ogre only looks at the absolute value of this
    LightInfo& setLightAmbientColor(const Color&c){
        mWhichFields|=AMBIENT_COLOR;
        mAmbientColor=c;
        return *this;
    }
    ///ogre only looks at the absolute value of this
    LightInfo& setLightShadowColor(const Color&c){
        mWhichFields|=SHADOW_COLOR;
        mShadowColor=c;
        return *this;
    }
    LightInfo& setLightRange(float64 maxRange){
        mWhichFields|=LIGHT_RANGE;
        mLightRange=maxRange;
        return *this;
    }
    LightInfo& setLightFalloff(float32 constant, float32 linear,float32 quadratic){
        mWhichFields|=FALLOFF;
        mConstantFalloff=constant;
        mLinearFalloff=linear;
        mQuadraticFalloff=quadratic;
        return *this;
    }
    LightInfo& setLightSpotlightCone(float32 innerRadians, float32 outerRadians,float32 coneFalloff){
        mWhichFields|=CONE;
        mConeInnerRadians=innerRadians;
        mConeOuterRadians=outerRadians;
        mConeFalloff=coneFalloff;
        return *this;
    }
    LightInfo& setLightType(LightTypes type){
        mWhichFields|=TYPE;
        mType=type;
        return *this;
    }
    LightInfo& setCastsShadow(bool shouldCastShadow){
        mWhichFields|=CAST_SHADOW;
        mCastsShadow=shouldCastShadow;
        return *this;
    }
    LightInfo& operator=(const LightInfo&other) {
        if (other.mWhichFields&DIFFUSE_COLOR) {
            mDiffuseColor=other.mDiffuseColor;
            mWhichFields|=DIFFUSE_COLOR;
        }
        if (other.mWhichFields&SPECULAR_COLOR) {
            mSpecularColor=other.mSpecularColor;
            mWhichFields|=SPECULAR_COLOR;
        }
        if (other.mWhichFields&POWER) {
            mPower=other.mPower;
            mWhichFields|=POWER;
        }
        if (other.mWhichFields&AMBIENT_COLOR) {
            mAmbientColor=other.mAmbientColor;
            mWhichFields|=AMBIENT_COLOR;
        }
        if (other.mWhichFields&SHADOW_COLOR) {
            mShadowColor=other.mShadowColor;
            mWhichFields|=SHADOW_COLOR;
        }
        if (other.mWhichFields&LIGHT_RANGE) {
            mLightRange=other.mLightRange;
            mWhichFields|=LIGHT_RANGE;
        }
        if (other.mWhichFields&FALLOFF) {
            mConstantFalloff=other.mConstantFalloff;
            mLinearFalloff=other.mLinearFalloff;
            mQuadraticFalloff=other.mQuadraticFalloff;
            mWhichFields|=FALLOFF;
        }
        if (other.mWhichFields&CONE) {
            mConeInnerRadians=other.mConeInnerRadians;
            mConeOuterRadians=other.mConeOuterRadians;
            mConeFalloff=other.mConeFalloff;
            mWhichFields|=CONE;
        }
        if (other.mWhichFields&TYPE) {
            mType=other.mType;
            mWhichFields|=TYPE;
        }
        if (other.mWhichFields&CAST_SHADOW) {
            mCastsShadow=other.mCastsShadow;
            mWhichFields|=CAST_SHADOW;
        }
        return *this;
    }
};
}

#endif
