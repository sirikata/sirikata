/*  Sirikata libproxyobject -- COLLADA Models System
 *  ColladaSystem.cpp
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

#include "ColladaSystem.hpp"

#include "ColladaErrorHandler.hpp"
#include "ColladaDocumentImporter.hpp"
#include "ColladaDocumentLoader.hpp"

#include <sirikata/core/options/Options.hpp>

// OpenCOLLADA headers

#include "COLLADAFWRoot.h"
#include "COLLADASaxFWLLoader.h"

#include "MeshdataToCollada.hpp"


#include <iostream>
#include <fstream>

#define COLLADA_LOG(lvl,msg) SILOG(collada, lvl, "[COLLADA] " << msg);

using namespace std;
using namespace Sirikata;
using namespace Sirikata::Transfer;

namespace Sirikata { namespace Models {

ColladaSystem::ColladaSystem ()
    :   mDocuments ()
{
    COLLADA_LOG(insane, "ColladaSystem::ColladaSystem() entered");
}

ColladaSystem::~ColladaSystem ()
{
    COLLADA_LOG(insane, "ColladaSystem::~ColladaSystem() entered");
}

ColladaSystem* ColladaSystem::create (String const& options)
{
    COLLADA_LOG(insane, "ColladaSystem::create( " << options << ") entered");
    ColladaSystem* system ( new ColladaSystem );

    if ( system->initialize (options ) )
        return system;
    delete system;
    return 0;
}

bool ColladaSystem::initialize(String const& options)
{
    COLLADA_LOG(insane, "ColladaSystem::initialize() entered");

    InitializeClassOptions ( "colladamodels", this, NULL );
    OptionSet::getOptions ( "colladamodels", this )->parse ( options );

    return true;
}

/////////////////////////////////////////////////////////////////////
// overrides from ModelsSystem

bool ColladaSystem::canLoad(std::tr1::shared_ptr<const Transfer::DenseData> data) {
    // There's no magic number for collada files. Instead, search for
    // a <COLLADA> tag (just the beginning since it has other content
    // in it).  Originally we'd check for the closing tag too, but to
    // keep this check minimal, we only check the beginning of the
    // document, so we can't check for the closing tag.
    if (!data) return false;

    // Create a string out of the first 1K
    int32 sublen = std::min((int)data->length(), (int)1024);
    std::string subset((const char*)data->begin(), (std::size_t)sublen);

    if (subset.find("<COLLADA") != subset.npos)
        return true;

    return false;
}

MeshdataPtr ColladaSystem::load(const Transfer::URI& uri, const Transfer::Fingerprint& fp,
            std::tr1::shared_ptr<const Transfer::DenseData> data)
{
    ColladaDocumentLoader loader(uri, fp);

      SparseData data_reflatten = SparseData();
      data_reflatten.addValidData(data);

      Transfer::DenseDataPtr flatData = data_reflatten.flatten();

      char const* buffer = reinterpret_cast<char const*>(flatData->begin());
      loader.load(buffer, flatData->length());


      return loader.getMeshdata();
}

bool ColladaSystem::convertMeshdata(const Meshdata& meshdata, const String& format, const String& filename) {
    // format is ignored, we only know one format
    int result = meshdataToCollada(meshdata, filename);
    return (result == 0);
}


} // namespace Models
} // namespace Sirikata
