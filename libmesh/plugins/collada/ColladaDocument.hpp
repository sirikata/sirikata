/*  Sirikata libproxyobject -- COLLADA Models Document
 *  ColladaDocument.hpp
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

#ifndef _SIRIKATA_COLLADA_DOCUMENT_
#define _SIRIKATA_COLLADA_DOCUMENT_

#include "ColladaAsset.hpp"

#include <sirikata/core/transfer/URI.hpp>

namespace COLLADAFW {

class FileInfo;
class Geometry;

}

namespace Sirikata { namespace Models {

/////////////////////////////////////////////////////////////////////

class ColladaDocumentImporter;

/**
 *  A class that represents a COLLADA document.
 */
class SIRIKATA_PLUGIN_EXPORT ColladaDocument
{
    public:
        explicit ColladaDocument ( Transfer::URI const& uri );
        ColladaDocument ( ColladaDocument const& );
        ColladaDocument& operator = ( ColladaDocument const& );
        ~ColladaDocument ();

        bool import ( ColladaDocumentImporter& importer, COLLADAFW::FileInfo const& asset );
        bool import ( ColladaDocumentImporter& importer, COLLADAFW::Geometry const& geometry );

//        bool export ( ColladaDocumentExporter& exporter, ... );

        Transfer::URI const& getURI () const { return mURI; }

        ColladaAsset const& getAsset () const { return mAsset; }

    protected:

    private:
        Transfer::URI mURI;
        ColladaAsset mAsset;
};

typedef std::tr1::shared_ptr< ColladaDocument > ColladaDocumentPtr;


} // namespace Models
} // namespace Sirikata

#endif // _SIRIKATA_COLLADA_DOCUMENT_
