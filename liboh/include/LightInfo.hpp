
#ifndef _SIRIKATA_LIGHT_INFO_HPP_
#define _SIRIKATA_LIGHT_INFO_HPP_

namespace Sirikata {
class LightInfo {
    enum LightTypes {
        POINT_LIGHT,SPOTLIGHT_LIGHT,DIRECTIONAL_LIGHT,NUM_LIGHT_TYPES//defaults to point=0?
    };
    enum Fields {
        NONE=0,
        DIFFUSE_COLOR=1,
        SPECULAR_COLOR=2,
        POWER=8,
        AMBIENT_POWER=16,
        SHADOW_POWER=32,
        LIGHT_RANGE=64,
        FALLOFF=128,
        CONE=256,
        TYPE=512,
        CAST_SHADOW=1024
    }
    int32 mWhichFields;
public:
    LightInfo() {
        mWhichFields=0;
        mDiffuseColor=Color(1,1,1);
        mSpecularColor=Color(1,1,1);
        mPower=1;
        mAmbientPower=.05;
        mShadowPower=.02;
        mLightRange=256;
        mConstantFalloff=1.0;
        mLinearFalloff=.01;
        mQuadraticFalloff=.02;
        mConeInnerRadians=0;
        mConeOuterRadians=3.1415926536;
        mConeFalloff=0;
        mType=POINT;
        mCastsShadow=true;
    }
    Color mDiffuseColor;
    Color mSpecularColor;
    float32 mPower;
    float32 mAmbientPower;
    float32 mShadowPower;
    float64 mLightRnage;
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
    LightInfo& setLightAmbientPower(float32 p){
        mWhichFields|=AMBIENT_POWER;
        mAmbientPower=p;
        return *this;
    }
    ///ogre only looks at the absolute value of this
    LightInfo& setLightShadowColor(float32 p){
        mWhichFields|=SHADOW_POWER;
        mShadowPower=p;
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
        if (other.mWhichFields&AMBIENT_POWER) {
            mAmbientPower=other.mAmbientPower;
            mWhichFields|=AMBIENT_POWER;
        }
        if (other.mWhichFields&SHADOW_POWER) {
            mShadowPower=other.mShadowPower;
            mWhichFields|=SHADOW_POWER;
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
            mType=type;
            mWhichFields|=TYPE;
        }
        if (other.mWhichFields&CAST_SHADOW) {
            mCastsShadow=shouldCastShadow;
            mWhichFields|=CAST_SHADOW;
        }
    }
}
}

#endif
