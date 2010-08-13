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

#include "../meruCompat/MeruDefs.hpp"
#include "../meruCompat/Singleton.hpp"
#include <sirikata/core/transfer/URI.hpp>
#include <boost/thread.hpp>
#include <map>
#include <vector>
#include "../meruCompat/Factory.hpp"
#include "../meruCompat/Event.hpp"

#define INSTRUMENT_RESOURCE_LOADING

namespace Sirikata{namespace Transfer{

}}

namespace Meru {
class ResourceManagerLookup;

/** Base class of resource managers.  This class is abstract and handles
 *  the general parts of handling resource open/get and put requests.
 */
class ResourceManager:public ManualSingleton<ResourceManager> {


public:
    /** Initializes the ResourceManager and starts CacheLevel threads if necessary
     *  \param transfermanager parent TransferManager to use
     */
    ResourceManager();

    /** Destroys the resource manager and all its cache levels
     */
    ~ResourceManager();

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



    /** Create a new resource from in-memory data.
     *  \param request The name of the item to be written. If using level 1 CDN then the hash of data
     *  \param data pointer to the buffer of data for the new resource
     */
    //void write (const URI &request, ResourceBuffer data);
};

typedef Factory<ResourceManager> ResourceManagerFactory;

} // namespace Meru

#endif //_RESOURCE_MANAGER_HPP_
