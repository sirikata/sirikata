// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_MANUAL_SKELETON_LOADER_HPP_
#define _SIRIKATA_OGRE_MANUAL_SKELETON_LOADER_HPP_

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/ogre/Util.hpp>
#include <OgreResource.h>
#include <sirikata/mesh/Meshdata.hpp>

namespace Sirikata {
namespace Graphics {

class ManualSkeletonLoader : public Ogre::ManualResourceLoader {
public:
    ManualSkeletonLoader(Mesh::MeshdataPtr meshdata, const std::set<String>& animationList);

    
    static bool getTRS(const Matrix4x4f& bsm, Ogre::Vector3& translate, Ogre::Quaternion& quaternion, Ogre::Vector3& scale);

    void prepareResource(Ogre::Resource*r) {}
    void loadResource(Ogre::Resource *r);

    bool wasSkeletonLoaded() {
      return skeletonLoaded;
    }

private:
    Mesh::MeshdataPtr mdptr;
    bool skeletonLoaded;
    const std::set<String>& animations;
};



} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_MANUAL_SKELETON_LOADER_HPP_
