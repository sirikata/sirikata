/*  Sirikata Object Host -- Proxy Creation and Destruction manager
 *  ProxyManager.hpp
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
#include <util/ListenerProvider.hpp>
#include "TimeSteppedSimulation.hpp"
#include "ProxyObject.hpp"
namespace Sirikata {

/** An interface for a class that keeps track of proxy object references. */
class SIRIKATA_OH_EXPORT ProxyManager : 
//        public MessageService,
        public Provider<ProxyCreationListener*> {
public:
    ProxyManager();
    virtual ~ProxyManager();
    ///Called after providers attached
    virtual void initialize()=0;
    ///Called before providers detatched
    virtual void destroy()=0;

    ///Adds to internal ProxyObject map and calls creation listeners.
    virtual void createObject(const ProxyObjectPtr &newObj)=0;

    ///Removes from internal ProxyObject map, calls destruction listeners, and calls newObj->destroy().
    virtual void destroyObject(const ProxyObjectPtr &newObj)=0;

    /// Ask for a proxy object by ID. Returns ProxyObjectPtr() if it doesn't exist.
    virtual ProxyObjectPtr getProxyObject(const SpaceObjectReference &id) const=0;


};
}
