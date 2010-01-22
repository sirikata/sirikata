/*  Meru
 *  ResourceManager.cpp
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

#include "ResourceManager.hpp"
#include "ResourceTransfer.hpp"
#include "../meruCompat/EventSource.hpp"
#include <util/Sha256.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include <boost/thread.hpp>
/*
#include <boost/bind.hpp>
#ifdef _WIN32
#include <boost/filesystem.hpp>
#else
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#endif
*/
#include <transfer/TransferManager.hpp>
namespace Meru {

MANUAL_SINGLETON_STORAGE(ResourceManager);
ResourceManager::ResourceManager(::Sirikata::Transfer::TransferManager*transferManager)
 :mTransferManager(transferManager) {
}
ResourceManager::~ResourceManager(){
}

/*
void ResourceManager::write (const URI &request, ResourceBuffer data){
    //write through cache
    // FIXME: what is this supposed to do?????
    mTransferManager->upload(request,data,EventListener());
}
*/

// FIXME: UNIMPLEMENTED
void ResourceManager::cdnLogin(const String&username, const String&password) {
    //mNameLookup->cdnLogin(username,password);
}
void ResourceManager::cdnLogout(const String&username) {
    //mNameLookup->cdnLogout(username);
}
bool ResourceManager::cdnIsAuthenticated(const String&username)const{
    //return mNameLookup->cdnIsAuthenticated(username);
    return false;
}

/*
void ResourceManager::handleExternalUploadComplete(const EventPtr&transferEvent){
    UploadEventPtr tevt(std::tr1::dynamic_pointer_cast<UpEventPtr>(transferEvent));
    if (tevt) {
        if (tevt->success()) {
            EventSource::getSingleton().dispatch (new UploadCompleteEvent(tevt->uri()));
        } else {
            EventSource::getSingleton().dispatch (new UploadFailedEvent(tevt->uri()));
        }
    }
}
void ResourceManager::handleExternalDownloadComplete(const EventPtr&transferEvent){
    DownloadEventPtr tevt(std::tr1::dynamic_pointer_cast<DownloadEventPtr>(transferEvent));
    if (tevt) {
        if (tevt->success()) {
            EventSource::getSingleton().dispatch (new DownloadCompleteEvent(tevt->uri(),tevt->data()));
            ResourceManagerLookup::DownloadCallbackMap::iterator where=mCallbacks->whereNotify.lower_bound(std::pair<URI,URI>(tevt->uri(),URI())),
                where_end=mCallbacks->whereNotify.end(),
                iter;
            for (iter=where;iter!=where_end&&iter->first==tevt->uri();++iter) {
                EventSource::getSingleton().dispatch (new DownloadCompleteEvent(iter->second,tevt->data()));
            }
            mCallbacks->whereNotify.erase(where,iter);
        } else {
            EventSource::getSingleton().dispatch (new DownloadFailedEvent(tevt->uri()));
            ResourceManagerLookup::DownloadCallbackMap::iterator where=mCallbacks->whereNotify.lower_bound(std::pair<URI,URI>(tevt->uri(),URI())),where_end=mCallbacks->whereNotify.end(),iter;
            for (iter=where;iter!=where_end&&iter->first==tevt->uri();++iter) {
                EventSource::getSingleton().dispatch (new DownloadFailedEvent(iter->second));
            }
            mCallbacks->whereNotify.erase(where,iter);
        }
    }
}
*/

Sirikata::Task::SubscriptionId ResourceManager::request (const RemoteFileId &request, const std::tr1::function<EventResponse(const EventPtr&)>&downloadFunctor, Transfer::Range range){
    return mTransferManager->downloadByHash(request,downloadFunctor,range);
}

void ResourceManager::nameLookup(const URI &resource_id, std::tr1::function<void(const URI&,const ResourceHash*)>callback) {
    mTransferManager->downloadName(resource_id,callback);
}

bool ResourceManager::nameLookup(const URI &resource_id, ResourceHash &result, std::tr1::function<void(const URI&,const ResourceHash*)>callback) {
    mTransferManager->downloadName(resource_id,callback);
    return false;
}

} // namespace Meru
