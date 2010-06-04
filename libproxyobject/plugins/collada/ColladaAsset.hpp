/*  Sirikata libproxyobject -- COLLADA Models Asset
 *  ColladaAsset.hpp
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

#ifndef _SIRIKATA_COLLADA_ASSET_
#define _SIRIKATA_COLLADA_ASSET_

#include <sirikata/proxyobject/Platform.hpp>

namespace COLLADAFW {

    class FileInfo;

}

namespace Sirikata { namespace Models {

/////////////////////////////////////////////////////////////////////

class ColladaDocumentImporter;

/**
 *  A class that represents a COLLADA asset.
 */
class SIRIKATA_PLUGIN_EXPORT ColladaAsset
{
    public:
        ColladaAsset ();
        ColladaAsset ( ColladaAsset const& );
        ColladaAsset& operator = ( ColladaAsset const& );
        ~ColladaAsset ();

        bool import ( ColladaDocumentImporter& importer, COLLADAFW::FileInfo const& asset );
        //        bool export ( ColladaDocumentExporter& exporter, ... );

    protected:

    private:
        void computeUpAxis ( COLLADAFW::FileInfo const& asset );

        float64 mUnitMeter;
        String mUnitName;
        Vector3< float > mUpAxis;
};


} // namespace Models
} // namespace Sirikata

#endif // _SIRIKATA_COLLADA_ASSET_
