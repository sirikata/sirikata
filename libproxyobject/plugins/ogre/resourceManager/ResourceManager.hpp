/*  Meru
 *  ResourceManager.hpp
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
#ifndef _RESOURCE_MANAGER_HPP_
#define _RESOURCE_MANAGER_HPP_

#include "MeruDefs.hpp"
#include "Singleton.hpp"
#include <transfer/URI.hpp>
#include <boost/thread.hpp>
#include <map>
#include <vector>
#include "Factory.hpp"
#include "Event.hpp"
//#include <transfer/TransferManager.hpp>

#define INSTRUMENT_RESOURCE_LOADING

namespace Sirikata{namespace Transfer{
  class TransferManager;
}}

namespace Meru {
class ResourceManagerLookup;

/** Base class of resource managers.  This class is abstract and handles
 *  the general parts of handling resource open/get and put requests.
 */
class ResourceManager:public ManualSingleton<ResourceManager> {

  ::Sirikata::Transfer::TransferManager *mTransferManager;

public:
    /** Initializes the ResourceManager and starts CacheLevel threads if necessary
     *  \param transfermanager parent TransferManager to use
     */
    ResourceManager(::Sirikata::Transfer::TransferManager *transfermanager);

    /** Destroys the resource manager and all its cache levels
     */
    ~ResourceManager();

    /**
     * Requests a lookup on the name of a resource.
     * \param resource_id is the string with a URI for the resource
     * In the event that the name is already in a cache or contained in the URI
     * \param result is where it will return the ResourceHash to be downloaded.
     * \param callback callback to invoke when the lookup is complete
     * \returns true if result is in memory
     */
    bool nameLookup(const URI &resource_id, ResourceHash&result, std::tr1::function<void(const URI&,const ResourceHash*)> callback);
    void nameLookup(const URI &resource_id, std::tr1::function<void(const URI&,const ResourceHash*)> callback);
    /**
     * Adds a username/password pair to the list of eligable identities for this resource manager
     * \param username indicates which username should be added to authentication for urls like meru://username@/blah
     * \param password specifies the credentials which authenticate the username param
     */
    void cdnLogin (const String&username, const String&password);
    /**
     * Destroys credentials for
     * \param username
     * so that user can no longer login with this resource manager
     * and which probably prevents uploads and perhaps even downloads
     */
    void cdnLogout (const String&username);

    bool cdnIsAuthenticated(const String&username)const;
    /** Request that a resource be downloaded.  This will start
     *  the resource loading in the resource loading thread and
     *  will return immediately.  If the resource already exists
     *  locally this does nothing.
     *
     *  \param rid the ResourceID of the resource to be downloaded
     *  \param cb Callback containing the Download Event.
     *  \param range subset of resource to request
     */
    SubscriptionId request (
        const RemoteFileId &rid,
        const std::tr1::function<EventResponse(const EventPtr&)>&cb,
        Transfer::Range range=Transfer::Range(true));


    /** Create a new resource from in-memory data.
     *  \param request The name of the item to be written. If using level 1 CDN then the hash of data
     *  \param data pointer to the buffer of data for the new resource
     */
    //void write (const URI &request, ResourceBuffer data);
};

typedef Factory<ResourceManager> ResourceManagerFactory;

} // namespace Meru

#endif //_RESOURCE_MANAGER_HPP_
