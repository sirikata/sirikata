/*  Sirikata liboh -- Object Host Graphics Interface
 *  GraphicsObject.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#ifndef _SIRIKATA_GRAPHICS_OBJECT_HPP_
#define _SIRIKATA_GRAPHICS_OBJECT_HPP_

namespace Sirikata {
typedef Vector4f ColorAlpha;
typedef Vector3f Color;
class GraphicsCamera;
class GraphicsObject {
    enum CapabilityFlags {
        HAS_NO_CAPABILITY=0,
        HAS_MESH_CAPABILITY=1,
        HAS_LOD_CAPABILITY=2,
        HAS_SCALE_CAPABILITY=4,
        HAS_ANIMATION_CAPABILITY=8,
        HAS_CAMERA_CAPABILITY=16,
        HAS_BOUNDS_CAPABILITY=32,
        HAS_SELECTION_CAPABILITY=64,//+
        HAS_LIGHT_CAPABILITY=128,//
        HAS_ALL_CAPABILITY=HAS_MESH_CAPABILITY|HAS_LOD_CAPABILITY|HAS_SCALE_CAPABILITY|HAS_ANIMATION_CAPABILITY|HAS_CAMERA_CAPABILITY|HAS_BOUNDS_CAPABILITY|HAS_SELECTION_CAPABILITY|HAS_LIGHT_CAPABILITY
    };
    uint32 mCapabilities;
public:
    virtual ~GraphicsObject(){}
    GraphicsObject():mCapabilities(HAS_NO_CAPABILITY) {}
    virtual void show()=0;
    virtual void hide()=0;
    virtual bool isHidden()=0;
    bool hasMeshCapability() const {
        return 0!=(mCapabilities&HAS_MESH_CAPABILITY);
    }
    virtual void unloadMesh(){}
    bool hasScaleCapability() const {
        return 0!=(mCapabilities&HAS_SCALE_CAPABILITY);
    }
    virtual bool setScale(const Vector3f&scale){
        return false;
    }
    virtual Vector3f getScale()const {
        return Vector3f(1,1,1);
    }
    bool hasCameraCapability() const {
        return 0!=(mCapabilities&HAS_CAMERA_CAPABILITY);
    }
    virtual GraphicsCamera* getCamera() const{
        return NULL;
    }
    bool hasBoundsCapability() {
        return 0!=(mCapabilities&HAS_BOUNDS_CAPABILITY);
    }
    bool setPrescaleBoundingBox(const BoundingBox3f3f&){
        return false;
    }
    bool setPrescaleBoundingSphere(const BoundingSphere3f&){
        return false;
    }
    BoundingBox3f3f getPrescaleBoundingBox()const{
        return BoundingBox3f3f::null();
    }
    BoundingSphere3f getPrescaleBoundingSphere()const{
        return BoundingSphere3f::null();
    }
    virtual void frame(const Duration& dt)=0;
    bool hasLocationCapability() {
        return true;
    }
    virtual bool setLocation(const Location&location)=0;
    virtual Location getLocation() const =0;
    bool hasSelectedCapability() {
        return 0!=(mCapabilities&HAS_SELECTION_CAPABILITY);
    }
    bool hasAnimationCapability() {
        return 0!=(mCapabilities&HAS_ANIMATION_CAPABILITY);
    }
    virtual bool triggerAnimation(const String&name_uri,float weight, bool loop, const String&mask_uri=""){
        return false;
    }
    virtual bool cancelAnimation(const String&name_uri) {
        return false;
    }
    virtual bool adjustAnimationLooping(const String&name_uri, bool turn_on_looping) {
        return false;
    }
    virtual bool adjustAnimationWeight(const String&name_uri,float weight, const String&mask_uri=""){
        return false;
    }
    unsigned int getNumBones()const{return 0;}
    String getBoneName(unsigned int which)const;
    ///get worldspace transform of the given bone
    Transform getBoneTransform(unsigned int which)const;
    
    virtual bool attachObject(GraphicsObject*object,
                              const Transform&localPosition);
    virtual bool attachObjectToBone(GraphicsObject*object,
                                    const Transform&localPosition,
                                    String boneName);
    virtual bool detatchObject(GraphicsObject*object);
    virtual bool detatchAllObjects();
    bool hasLODCapability() {
        return 0!=(mCapabilities&HAS_ANIMATION_CAPABILITY);
    }
    virtual bool setMeshLODBias(float32 factor, unsigned int max_level=(1<<31), unsigned int min_level=0){
        return false;
    }
    virtual bool setMaterialLODBias(float32 factor, unsigned int max_level=(1<<31), unsigned int min_level=0){
        return false;
    }
    bool hasLightCapability() {
        return 0!=(mCapabilities&HAS_LIGHT_CAPABILITY);
    }
    virtual bool setLightDiffuseColor(const Color&c){return false;}
    virtual bool setLightSpecularColor(const Color&c){return false;}
    virtual bool setLightPower(float32 c){return false;}
    ///ogre only looks at the absolute value of this
    virtual bool setLightAmbientColor(const Color&c){return false;}
    ///ogre only looks at the absolute value of this
    virtual bool setLightShadowColor(const Color&c){return false;}
    virtual bool setLightRange(float64 maxRange){return false;}
    virtual bool setLightFalloff(float32 constant, float32 linear,float32 quadratic){return false;}
    virtual bool setLightSpotlightCone(float32 innerRadians, float32 outerRadians,float32 coneFalloff){return false;}
    enum LightTypes {
        POINT_LIGHT,SPOTLIGHT_LIGHT,DIRECTIONAL_LIGHT,NUM_LIGHT_TYPES//defaults to point=0?
    };
    virtual bool setLightType(String type){return false;}
    virtual bool setCastsShadow(bool shouldCastShadow){return false;}
};
}
#endif
