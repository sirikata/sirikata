/*  Meru
 *  ResourceLoadingQueue.hpp
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
#ifndef _RESOURCE_DEPENDENCY_TASK_H_
#define _RESOURCE_DEPENDENCY_TASK_H_

#include "../meruCompat/MeruDefs.hpp"
#include <OgrePrerequisites.h>
#include <OgreCommon.h>
#include <OgreSingleton.h>
#include <OgreResourceBackgroundQueue.h>
#include <queue>
#include "../meruCompat/Event.hpp"

namespace Meru {
#if OGRE_THREAD_SUPPORT!=0
typedef Ogre::ResourceBackgroundQueue ResourceLoadingQueue;
#else
  class ResourceLoadingQueue:public Ogre::Singleton<ResourceLoadingQueue> {
    enum RequestType {ZERO,ONE,TWO,RT_LOAD_RESOURCE,FOUR,RT_UNLOAD_RESOURCE} ;
    struct Request
    {
      Ogre::BackgroundProcessTicket ticketID;
      RequestType type;
      Ogre::String resourceName;
      Ogre::ResourceHandle resourceHandle;
      Ogre::String resourceType;
      Ogre::String groupName;
      bool isManual;
      Ogre::ManualResourceLoader* loader;
      const Ogre::NameValuePairList* loadParams;
      Ogre::ResourceBackgroundQueue::Listener* listener;
    };
    std::queue<Request> mRequests;
    Ogre::BackgroundProcessTicket mNextTicketID;
    unsigned int mHowLongAgoHadNothing;
    unsigned int mProcessPerFrame;
  public:
    Ogre::BackgroundProcessTicket load (const Ogre::String&resType, const Ogre::String&name, const Ogre::String&group,
               bool isManual,
               Ogre::ManualResourceLoader*loader=0,
               const Ogre::NameValuePairList *loadParams=0,
               Ogre::ResourceBackgroundQueue::Listener* listener=0);
    Ogre::BackgroundProcessTicket unload (const Ogre::String&resType,
                 const Ogre::String&name,
                 Ogre::ResourceBackgroundQueue::Listener* listener=0);
    Ogre::BackgroundProcessTicket unload (const Ogre::String&resType,
                 Ogre::ResourceHandle handle,
                 Ogre::ResourceBackgroundQueue::Listener* listener=0);
    ResourceLoadingQueue();
    ~ResourceLoadingQueue();
    void processBackgroundEvent();
    void processBackgroundEvents(const Meru::EventPtr&);
    void operator() (const Meru::EventPtr&);
  };
#endif
}
#endif
