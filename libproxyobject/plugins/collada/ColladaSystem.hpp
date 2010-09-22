/*  Sirikata libproxyobject -- Collada Models System
 *  ColladaSystem.hpp
 *
 *  Copyright (c) 2009, Mark C. Barnes
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

#ifndef _SIRIKATA_COLLADA_SYSTEM_
#define _SIRIKATA_COLLADA_SYSTEM_

#include "ColladaDocument.hpp"

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/proxyobject/ModelsSystem.hpp>
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/transfer/TransferManager.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/Range.hpp>
#include <sirikata/core/task/EventManager.hpp>
#include <set>

/////////////////////////////////////////////////////////////////////

namespace Sirikata {

class OptionValue;

namespace Models {

/////////////////////////////////////////////////////////////////////

class SIRIKATA_PLUGIN_EXPORT ColladaSystem
    :   public ModelsSystem
{
    public:
        virtual ~ColladaSystem ();

        static ColladaSystem* create ( Provider< ProxyCreationListener* >* proxyManager, String const& options );

//        void loadDocument ( Transfer::URI const& what, ProxyMeshObject* proxy  );
        void loadDocument(std::tr1::weak_ptr<ProxyMeshObject>(proxy), std::tr1::shared_ptr<Transfer::ChunkRequest> request,
            std::tr1::shared_ptr<Transfer::DenseData> response);

        // documents that have been transfered, parsed, and loaded.

        std::tr1::shared_ptr<Transfer::TransferPool> transferPool();

    protected:
	void metadataFinished(std::tr1::weak_ptr<ProxyMeshObject> proxy, std::tr1::shared_ptr<Transfer::MetadataRequest> request,
				     std::tr1::shared_ptr<Transfer::RemoteFileMetadata>response);
	void chunkFinished(std::tr1::weak_ptr<ProxyMeshObject> proxy, std::tr1::shared_ptr<Transfer::ChunkRequest> request,
				  std::tr1::shared_ptr<Transfer::DenseData> response);
	std::tr1::shared_ptr<Transfer::TransferPool> mTransferPool;
	Transfer::TransferMediator *mTransferMediator;

  private:
        ColladaSystem (); // called by create()
        ColladaSystem ( ColladaSystem const& ); // not implemented
        ColladaSystem& operator = ( ColladaSystem const & ); // not implemented

        bool initialize ( Provider< ProxyCreationListener* >* proxyManager, String const& options );

        Task::EventResponse downloadFinished ( Task::EventPtr evbase, Transfer::URI const& what, std::tr1::weak_ptr<ProxyMeshObject>(proxy) );

        // things we need to integrate with Sirikata
        OptionValue* mEventManager; // MCB: managed object
        OptionValue* mTransferManager; // MCB: managed object
        OptionValue* mWorkQueue; // MCB: managed object

        // documents that have been transfered, parsed, and loaded.
        // MCB: make this a map when/if a key becomes useful

        typedef std::set< ColladaDocumentPtr > DocumentSet;
        DocumentSet mDocuments;

    // interface from ModelsSystem
    public:
    protected:

    // interface from ProxyCreationListener
    public:
        virtual void onCreateProxy ( ProxyObjectPtr object );
        virtual void onDestroyProxy ( ProxyObjectPtr object );

    protected:

};

} // namespace Models
} // namespace Sirikata

#endif // _SIRIKATA_COLLADA_SYSTEM_
