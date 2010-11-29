/*  Sirikata libproxyobject -- COLLADA Models Document Loader
 *  ColladaDocumentLoader.cpp
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

#include "ColladaDocumentLoader.hpp"

#include "ColladaErrorHandler.hpp"
#include "ColladaDocumentImporter.hpp"

// OpenCOLLADA headers
#include "COLLADAFWRoot.h"
#include "COLLADASaxFWLLoader.h"


#include <iostream>

#define COLLADA_LOG(lvl,msg) SILOG(collada, lvl, "[COLLADA] " << msg);

namespace Sirikata { namespace Models {

ColladaDocumentLoader::ColladaDocumentLoader (Transfer::URI const& uri, const SHA256& hash)
    :   mErrorHandler ( new ColladaErrorHandler ),
        mSaxLoader ( new COLLADASaxFWL::Loader ( mErrorHandler ) ),
        mDocumentImporter ( new ColladaDocumentImporter ( uri, hash ) ),
        mFramework ( new COLLADAFW::Root ( mSaxLoader, mDocumentImporter ) )
{
    COLLADA_LOG(insane, "ColladaDocumentLoader::ColladaDocumentLoader() entered");

}

ColladaDocumentLoader::~ColladaDocumentLoader ()
{
    COLLADA_LOG(insane, "ColladaDocumentLoader::~ColladaDocumentLoader() entered");

    delete mFramework;
    delete mDocumentImporter;
    delete mSaxLoader;
    delete mErrorHandler;
}

/////////////////////////////////////////////////////////////////////

bool ColladaDocumentLoader::load ( char const* buffer, size_t bufferLength )
{
    bool ok = mFramework->loadDocument ( mDocumentImporter->documentURI(), buffer, bufferLength );

    return ok;
}


ColladaDocumentPtr ColladaDocumentLoader::getDocument () const
{
    return mDocumentImporter->getDocument ();
}

std::tr1::shared_ptr<Meshdata> ColladaDocumentLoader::getMeshdata() const
{
  return mDocumentImporter->getMeshdata();
}

/////////////////////////////////////////////////////////////////////


} // namespace Models
} // namespace Sirikata
