// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualMaterialLoader.hpp"
#include <OgreTextureUnitState.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <sirikata/core/transfer/URL.hpp>

namespace Sirikata {
namespace Graphics {

using namespace Sirikata::Mesh;

// A few utilities
namespace {

Ogre::TextureUnitState::TextureAddressingMode translateWrapMode(MaterialEffectInfo::Texture::WrapMode w) {
    switch(w) {
      case MaterialEffectInfo::Texture::WRAP_MODE_CLAMP:
        SILOG(ogre,insane,"CLAMPING");
        return Ogre::TextureUnitState::TAM_CLAMP;
      case MaterialEffectInfo::Texture::WRAP_MODE_MIRROR:
        SILOG(ogre,insane,"MIRRORING");
        return Ogre::TextureUnitState::TAM_MIRROR;
      case MaterialEffectInfo::Texture::WRAP_MODE_WRAP:
      default:
        SILOG(ogre,insane,"WRAPPING");
        return Ogre::TextureUnitState::TAM_WRAP;
    }
}

void fixupTextureUnitState(Ogre::TextureUnitState*tus, const MaterialEffectInfo::Texture&tex) {
    tus->setTextureAddressingMode(translateWrapMode(tex.wrapS),
                                  translateWrapMode(tex.wrapT),
                                  translateWrapMode(tex.wrapU));

}

}

ManualMaterialLoader::ManualMaterialLoader(
    VisualPtr visptr,
    const String name,
    const MaterialEffectInfo&mat,
    const Transfer::URI& uri,
    TextureBindingsMapPtr textureFingerprints)
 : mTextureFingerprints(textureFingerprints)
{
    // mVisualPtr keeps mMat pointer valid
    mVisualPtr = visptr;
    mName = name;
    mMat=&mat;
    mURI=uri;
}

void ManualMaterialLoader::loadResource(Ogre::Resource *r) {
    using namespace Ogre;
    Material* material= dynamic_cast <Material*> (r);
    material->setCullingMode(CULL_NONE);
    Ogre::Technique* tech=material->getTechnique(0);
    bool useAlpha=false;
    if (mMat->textures.empty()) {
        Ogre::Pass*pass=tech->getPass(0);
        pass->setDiffuse(ColourValue(1,1,1,1));
        pass->setAmbient(ColourValue(1,1,1,1));
        pass->setSelfIllumination(ColourValue(0,0,0,0));
        pass->setSpecular(ColourValue(1,1,1,1));
    }
    for (size_t i=0;i<mMat->textures.size();++i) {
        // NOTE: We're currently disabling using alpha when the
        // texture is specified for OPACITY because we're not
        // handling it in the code below, leading to artifacts as
        // it seemingly uses random segments of memory for the
        // alpha values (maybe because it tries reading them from
        // some texture other texture that doesn't specify
        // alpha?).
        if (mMat->textures[i].affecting==MaterialEffectInfo::Texture::OPACITY &&
            (/*mMat->textures[i].uri.length() > 0 ||*/
                (mMat->textures[i].uri.length() == 0 && mMat->textures[i].color.w<1.0))) {
            useAlpha=true;
            break;
        }
    }
    unsigned int valid_passes=0;
    {
        Ogre::Pass* pass = tech->getPass(0);
        pass->setAmbient(ColourValue(0,0,0,0));
        pass->setDiffuse(ColourValue(0,0,0,0));
        pass->setSelfIllumination(ColourValue(0,0,0,0));
        pass->setSpecular(ColourValue(0,0,0,0));
    }
    for (size_t i=0;i<mMat->textures.size();++i) {
        MaterialEffectInfo::Texture tex=mMat->textures[i];
        Ogre::Pass*pass = tech->getPass(0);
        if (tex.uri.length()==0) { // Flat colors
            switch (tex.affecting) {
              case MaterialEffectInfo::Texture::DIFFUSE:
                pass->setDiffuse(ColourValue(tex.color.x,
                        tex.color.y,
                        tex.color.z,
                        tex.color.w));
                break;
              case MaterialEffectInfo::Texture::AMBIENT:
                pass->setAmbient(ColourValue(tex.color.x,
                        tex.color.y,
                        tex.color.z,
                        tex.color.w));
                break;
              case MaterialEffectInfo::Texture::EMISSION:
                pass->setSelfIllumination(ColourValue(tex.color.x,
                        tex.color.y,
                        tex.color.z,
                        tex.color.w));
                break;
              case MaterialEffectInfo::Texture::SPECULAR:
                pass->setSpecular(ColourValue(tex.color.x,
                        tex.color.y,
                        tex.color.z,
                        tex.color.w));
                pass->setShininess(mMat->shininess);
                break;
              default:
                break;
            }
        }
        else if (tex.affecting==MaterialEffectInfo::Texture::DIFFUSE ||
            tex.affecting==MaterialEffectInfo::Texture::AMBIENT)
        { // or textured
            // FIXME other URI schemes besides URL
            Transfer::URL url(mURI);
            // Non-empty parent URL or absolute texture URL
            assert(!url.empty() || (!Transfer::URI(tex.uri).empty()) );
            Transfer::URI mat_uri( Transfer::URL(url.context(), tex.uri).toString() );
            TextureBindingsMap::iterator where = mTextureFingerprints->find(mat_uri.toString());
            if (where!=mTextureFingerprints->end()) {
                String ogreTextureName = where->second;
                Ogre::TextureUnitState*tus;
                //tus->setTextureName(tex.uri);
                //tus->setTextureCoordSet(tex.texCoord);
                if (useAlpha==false) {
                    pass->setAlphaRejectValue(.5);
                    pass->setAlphaRejectSettings(CMPF_GREATER,128,true);
                    if (true||i==0) {
                        pass->setSceneBlending(SBF_ONE,SBF_ZERO);
                    } else {
                        pass->setSceneBlending(SBF_ONE,SBF_ONE);
                    }
                }else {
                    pass->setDepthWriteEnabled(false);
                    pass->setDepthCheckEnabled(true);
                    if (true||i==0) {
                        pass->setSceneBlending(SBF_SOURCE_ALPHA,SBF_ONE_MINUS_SOURCE_ALPHA);
                    }else {
                        pass->setSceneBlending(SBF_SOURCE_ALPHA,SBF_ONE);
                    }
                }
                switch (tex.affecting) {
                  case MaterialEffectInfo::Texture::DIFFUSE:
                    if (tech->getNumPasses()<=valid_passes) {
                        pass = tech->createPass();
                        ++valid_passes;
                    }
                    pass->setDiffuse(ColourValue(1,1,1,1));

                    //pass->setIlluminationStage(IS_PER_LIGHT);
                    if (pass->getTextureUnitState(ogreTextureName) == 0) {
                        tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                        fixupTextureUnitState(tus,tex);
                        tus->setColourOperation(LBO_MODULATE);
                    }

                    break;
                  case MaterialEffectInfo::Texture::AMBIENT:
                    if (tech->getNumPasses()<=valid_passes) {
                        pass = tech->createPass();
                        ++valid_passes;
                    }

                    pass->setAmbient(ColourValue(1.0,1.0,1.0,1));
                    // NOTE: Currently ambient and diffuse are mutually
                    // exclusive. We need to make sure that the diffuse is
                    // non-zero so it actually shades. Otherwise, the
                    // ambient seems to have no effect. To do that, we set
                    // it to black, but with non-zero alpha.
                    pass->setDiffuse(ColourValue(0,0,0,1));

                    if (pass->getTextureUnitState(ogreTextureName) == 0) {
                        tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                        fixupTextureUnitState(tus,tex);
                        tus->setColourOperation(LBO_MODULATE);
                    }

                    pass->setIlluminationStage(IS_AMBIENT);
                    break;
                  case MaterialEffectInfo::Texture::EMISSION:
                    if (pass->getTextureUnitState(ogreTextureName) == 0) {
                        tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                        fixupTextureUnitState(tus,tex);
                        //pass->setSelfIllumination(ColourValue(1,1,1,1));
                        tus->setColourOperation(LBO_ADD);
                    }

                    //pass->setIlluminationStage(IS_DECAL);
                    break;
                  case MaterialEffectInfo::Texture::SPECULAR:
                    if (tech->getNumPasses()<=valid_passes) {
                        pass = tech->createPass();
                        ++valid_passes;
                    }

                    //pass->setIlluminationStage(IS_PER_LIGHT);

                    if (pass->getTextureUnitState(ogreTextureName) == 0) {
                        tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                        fixupTextureUnitState(tus,tex);
                        tus->setColourOperation(LBO_MODULATE);
                    }
                    pass->setSpecular(ColourValue(1,1,1,1));
                    break;
                  default:
                    break;
                }
            }
        }
    }
}

} // namespace Graphics
} // namespace Sirikata
