/*  Sirikata libproxyobject -- Collada Models Document Loader
 *  ColladaDocumentLoader.hpp
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

#ifndef _SIRIKATA_COLLADA_DOCUMENT_LOADER_
#define _SIRIKATA_COLLADA_DOCUMENT_LOADER_

#include <sirikata/mesh/Meshdata.hpp>
#include "ColladaDocument.hpp"


/////////////////////////////////////////////////////////////////////

namespace COLLADAFW {

class Root;

}

namespace COLLADASaxFWL {

class Loader;

}

/////////////////////////////////////////////////////////////////////

namespace Sirikata { namespace Models {

class ColladaDocumentImporter;
class ColladaErrorHandler;

/////////////////////////////////////////////////////////////////////

/**
 * A class designed to (one-shot) load a single COLLADA document using OpenCOLLADA.
 */
class SIRIKATA_PLUGIN_EXPORT ColladaDocumentLoader
{
    public:
    explicit ColladaDocumentLoader ( Transfer::URI const& uri, const SHA256& hash);
        ~ColladaDocumentLoader ();

        bool load ( char const* buffer, size_t bufferLength );
        ColladaDocumentPtr getDocument () const;

        std::tr1::shared_ptr<Meshdata> getMeshdata() const;


    protected:

    private:
        ColladaDocumentLoader ( ColladaDocumentLoader const& ); // not implemented
        ColladaDocumentLoader& operator = ( ColladaDocumentLoader const & ); // not implemented

        // things we need to integrate with OpenCollada (declared in order of initialization, do not change)
        ColladaErrorHandler* mErrorHandler; // 1st
        COLLADASaxFWL::Loader* mSaxLoader; // next
        ColladaDocumentImporter* mDocumentImporter; // next
        COLLADAFW::Root* mFramework; // last
};


} // namespace Models
} // namespace Sirikata

#endif // _SIRIKATA_COLLADA_DOCUMENT_LOADER_
