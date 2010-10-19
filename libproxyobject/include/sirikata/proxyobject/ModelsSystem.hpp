/*  Sirikata Object Host -- Models Creation and Destruction system
 *  ModelsSystem.hpp
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

#ifndef _SIRIKATA_MODELS_SYSTEM_
#define _SIRIKATA_MODELS_SYSTEM_

#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/transfer/TransferData.hpp>

#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/Range.hpp>

#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include <sirikata/proxyobject/Meshdata.hpp>

namespace Sirikata {

/**
 * An interface for a class that is responsible for data model objects.
 */
class SIRIKATA_PROXYOBJECT_EXPORT ModelsSystem
    :   public ProxyCreationListener
{
    public:

    protected:
        ModelsSystem () {}
        ModelsSystem ( ModelsSystem const& rhs ); // not implemented
        ModelsSystem& operator = ( ModelsSystem const& rhs ); // not implemented

    public:
        virtual ~ModelsSystem () {}

        // Interface from ProxyCreationListener
        virtual void onCreateProxy ( ProxyObjectPtr object ) = 0;
        virtual void onDestroyProxy ( ProxyObjectPtr object ) = 0;

        /** Load a mesh into a Meshdata object. */
        virtual MeshdataPtr load(const Transfer::URI& uri, std::tr1::shared_ptr<Transfer::ChunkRequest> request,
            std::tr1::shared_ptr<const Transfer::DenseData> response) = 0;

        /** Convert a Meshdata to the format for this ModelsSystem. */
        virtual void convertMeshdata(const Meshdata& meshdata, const std::string& filename) = 0;

    protected:

};

} // namespace Sirikata

#endif // _SIRIKATA_MODELS_SYSTEM_
