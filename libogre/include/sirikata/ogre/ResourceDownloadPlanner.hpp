// Copyright (c) 2009 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _RESOURCE_DOWNLOAD_PLANNER_HPP
#define _RESOURCE_DOWNLOAD_PLANNER_HPP

#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/ogre/Camera.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>

namespace Sirikata {
namespace Graphics {

class Entity;

/** Interface for service which plans when and what order to download meshes and
 *  their associated resources like textures and then passes them off to Ogre
 *  for loading. It is given proxies directly and it does all the work of
 *  deduplicating requests for identical meshes, planning the download of
 *  complete assets (including parsing and downloading sub-resources), and
 *  passing them off for loading.
 */
class SIRIKATA_OGRE_EXPORT ResourceDownloadPlanner : public PollingService
{
public:
    ResourceDownloadPlanner(Context* c, OgreRenderer* renderer);
    ~ResourceDownloadPlanner();

    virtual void addNewObject(Graphics::Entity *ent, const Transfer::URI& mesh);
    virtual void addNewObject(ProxyObjectPtr p, Graphics::Entity *mesh);
    virtual void updateObject(ProxyObjectPtr p);
    virtual void removeObject(ProxyObjectPtr p);
    virtual void removeObject(Graphics::Entity* mesh);
    virtual void setCamera(Graphics::Camera *entity);

    //PollingService interface
    virtual void poll();
    virtual void stop();

    virtual int32 maxObjects();
    virtual void setMaxObjects(int32 new_max);

    OgreRenderer* getScene() const { return mScene; }
protected:
    Context* mContext;
    OgreRenderer* mScene;
    Graphics::Camera *camera;
    int32 mMaxLoaded;

    typedef boost::recursive_mutex RMutex;
    //prevents multiple threads from simultaneously accessing
    //mObjects,mLoadedObjects,mWatingObjects,and assetMap.  can always split
    //this into multiple mutexes if performance suffers.
    RMutex mDlPlannerMutex;
};

} // namespace Graphics
} // namespace Sirikata

#endif
