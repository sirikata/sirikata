/*  Meru
 *  ResourceLoadingQueue.cpp
 *
 *  Copyright (c) 2009, Stanford University
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

#include "ResourceLoadingQueue.hpp"
#include "../OgreHeaders.hpp"
#include <OgreDataStream.h>
#include <OgreResourceBackgroundQueue.h>
#include <OgreResourceManager.h>
#include "../meruCompat/EventSource.hpp"
#include "ResourceManager.hpp"
#include <sirikata/core/util/UUID.hpp>

#if OGRE_THREAD_SUPPORT==0

template<> Meru::ResourceLoadingQueue* Ogre::Singleton<Meru::ResourceLoadingQueue>::ms_Singleton=NULL;
namespace Meru {

ResourceLoadingQueue::ResourceLoadingQueue() :mNextTicketID(0),mHowLongAgoHadNothing(0),mProcessPerFrame(1){
    EventSource::getSingleton().addListener(EventTypes::Tick,EVENT_CALLBACK(ResourceLoadingQueue, operator(), this));
}

Ogre::BackgroundProcessTicket ResourceLoadingQueue::load (const Ogre::String&resType, const Ogre::String&name, const Ogre::String&group,
    bool isManual,
    Ogre::ManualResourceLoader*loader,
    const Ogre::NameValuePairList *loadParams,
    Ogre::ResourceBackgroundQueue::Listener* listener){
    Request req;
    req.type=RT_LOAD_RESOURCE;
    req.resourceType=resType;
    req.resourceName=name;
    req.groupName=group;
    req.isManual=isManual;
    req.loader=loader;
    req.loadParams=loadParams;
    req.listener=listener;
    mRequests.push(req);
    return req.ticketID;
}

Ogre::BackgroundProcessTicket ResourceLoadingQueue::unload (const Ogre::String&resType,
    const Ogre::String&name,
    Ogre::ResourceBackgroundQueue::Listener* listener){
    Request req;
    req.type=RT_UNLOAD_RESOURCE;
    req.resourceType=resType;
    req.resourceName=name;
    req.listener=listener;
    req.ticketID=++mNextTicketID;
    mRequests.push(req);
    return req.ticketID;
}

Ogre::BackgroundProcessTicket ResourceLoadingQueue::unload (const Ogre::String&resType,
    Ogre::ResourceHandle handle,
    Ogre::ResourceBackgroundQueue::Listener* listener){
    Request req;
    req.type=RT_UNLOAD_RESOURCE;
    req.resourceType=resType;
    req.resourceHandle=handle;
    req.listener=listener;
    req.ticketID=++mNextTicketID;
    mRequests.push(req);
    return req.ticketID;
}

void ResourceLoadingQueue::processBackgroundEvent() {
    Request req=mRequests.front();
    mRequests.pop();
    Ogre::ResourceManager *rm=NULL;
    switch( req.type) {
      case RT_LOAD_RESOURCE:
        rm = Ogre::ResourceGroupManager::getSingleton()._getResourceManager(req.resourceType);
        rm->load(req.resourceName, req.groupName, req.isManual,
            req.loader, req.loadParams);
        break;
      case RT_UNLOAD_RESOURCE:
        rm = Ogre::ResourceGroupManager::getSingleton()._getResourceManager(req.resourceType);
        if (req.resourceName.empty())
            rm->unload(req.resourceHandle);
        else
            rm->unload(req.resourceName);
        break;
      default:
        assert("Unable to process requests of types not LOAD or UNLOAD"&&0);
    }
    if (req.listener) {
        req.listener->operationCompletedInThread(req.ticketID);
        req.listener->operationCompleted(req.ticketID);
    }
}

void ResourceLoadingQueue::processBackgroundEvents(const Meru::EventPtr&) {
    if (mRequests.size()) {
        mHowLongAgoHadNothing++;
        if (mHowLongAgoHadNothing>mProcessPerFrame*mHowLongAgoHadNothing) {
            mProcessPerFrame++;
            mHowLongAgoHadNothing=0;
        }
        for (size_t i=0;mRequests.size()&&i<mProcessPerFrame;++i) {
            this->processBackgroundEvent();
        }
    }else {
        mHowLongAgoHadNothing=0;
        mProcessPerFrame=1;
    }
}

void ResourceLoadingQueue::operator() (const Meru::EventPtr& e){
    this->processBackgroundEvents(e);
}

ResourceLoadingQueue::~ResourceLoadingQueue() {
    EventSource::getSingleton().removeListener(EventTypes::Tick,EVENT_CALLBACK_ID(ResourceLoadingQueue, operator(), this));
}

}
#endif
