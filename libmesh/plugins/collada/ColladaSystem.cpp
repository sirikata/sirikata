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

#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"

#include <sirikata/core/options/Options.hpp>

// OpenCOLLADA headers

#include "COLLADAFWRoot.h"
#include "COLLADASaxFWLLoader.h"

#include "MeshdataToCollada.hpp"


#include <iostream>
#include <fstream>

#define COLLADA_LOG(lvl,msg) SILOG(collada, lvl, msg);

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

bool ColladaSystem::canLoad(Transfer::DenseDataPtr data) {
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

void ColladaSystem::addHeaderData(const Transfer::RemoteFileMetadata& metadata, Mesh::MeshdataPtr mesh) {
    Mesh::ProgressiveDataPtr progData(new Mesh::ProgressiveData());

    const FileHeaders& headers = metadata.getHeaders();

    FileHeaders::const_iterator findProgHash = headers.find("Progresive-Stream");
    if (findProgHash == headers.end()) {
        return;
    }
    FileHeaders::const_iterator findProgNumTriangles = headers.find("Progresive-Stream-Num-Triangles");
    if (findProgNumTriangles == headers.end()) {
        return;
    }
    FileHeaders::const_iterator findMipmaps = headers.find("Mipmaps");
    if (findMipmaps == headers.end()) {
        return;
    }

    //Parse the hash of the progressive stream
    Transfer::Fingerprint progHash;
    try {
        progHash = Transfer::Fingerprint::convertFromHex(findProgHash->second);
    } catch (std::invalid_argument const&) {
        COLLADA_LOG(warn, "Error parsing progressive hash from headers");
        return;
    }
    COLLADA_LOG(detailed, "adding meshdata hash = " << progHash)
    progData->progressiveHash = progHash;

    //Parse number of triangles in the progressive stream
    uint32 prog_triangles;
    try {
        prog_triangles = boost::lexical_cast<uint32>(findProgNumTriangles->second);
    } catch (boost::bad_lexical_cast const&) {
        COLLADA_LOG(warn, "Error parsing progressive-stream-num-triangles from headers");
        return;
    }
    COLLADA_LOG(detailed, "adding meshdata triangles = " << prog_triangles)
    progData->numProgressiveTriangles = prog_triangles;

    //Parse number of mipmaps
    uint32 num_mipmaps;
    try {
        num_mipmaps = boost::lexical_cast<uint32>(findMipmaps->second);
    } catch (boost::bad_lexical_cast const&) {
        COLLADA_LOG(warn, "Error parsing number of mipmaps from headers");
        return;
    }
    Mesh::ProgressiveMipmapMap allMipmaps;
    for (uint32 i=0; i<num_mipmaps; i++) {
        std::string mipmapBaseName = "Mipmap-" + boost::lexical_cast<std::string>(i);

        std::string mipmapNameHeader = mipmapBaseName + "-Name";
        std::string mipmapHashHeader = mipmapBaseName + "-Hash";
        FileHeaders::const_iterator findMipmapName = headers.find(mipmapNameHeader);
        if (findMipmapName == headers.end()) {
            COLLADA_LOG(warn, "Could not find mipmap name from headers " << mipmapNameHeader);
            return;
        }
        FileHeaders::const_iterator findMipmapHash = headers.find(mipmapHashHeader);
        if (findMipmapHash == headers.end()) {
            COLLADA_LOG(warn, "Could not find mipmap hash from headers " << mipmapHashHeader);
            return;
        }

        Mesh::ProgressiveMipmapArchive mipmapArchive;
        mipmapArchive.name = findMipmapName->second;
        Transfer::Fingerprint mipmapHash;
        try {
            mipmapHash = Transfer::Fingerprint::convertFromHex(findMipmapHash->second);
        } catch (std::invalid_argument const&) {
            COLLADA_LOG(warn, "Error parsing mipmap hash from headers");
            return;
        }
        mipmapArchive.archiveHash = mipmapHash;
        COLLADA_LOG(detailed, "mipmap name " << mipmapArchive.name << " hash " << mipmapArchive.archiveHash);

        Mesh::ProgressiveMipmaps mipmapList;
        uint32 level = 0;
        FileHeaders::const_iterator findMipmapLevel;
        do {
            std::string mipmapLevelHeader = mipmapBaseName + "-Level-" + boost::lexical_cast<std::string>(level);
            findMipmapLevel = headers.find(mipmapLevelHeader);
            if (findMipmapLevel != headers.end()) {
                Mesh::ProgressiveMipmapLevel mipmapLevel;
                std::vector<std::string> tokens;
                boost::split(tokens, findMipmapLevel->second, boost::is_any_of(","));
                if (tokens.size() != 4) {
                    COLLADA_LOG(warn, "Got wrong number of tokens when splitting mipmap level");
                    return;
                }
                try {
                    mipmapLevel.offset = boost::lexical_cast<uint32>(tokens[0]);
                    mipmapLevel.length = boost::lexical_cast<uint32>(tokens[1]);
                    mipmapLevel.width = boost::lexical_cast<uint32>(tokens[2]);
                    mipmapLevel.height = boost::lexical_cast<uint32>(tokens[3]);
                } catch (boost::bad_lexical_cast const&) {
                    COLLADA_LOG(warn, "Error converting mipmap level tokens to integers");
                    return;
                }
                COLLADA_LOG(detailed, "mipmap level " << level << " has " << mipmapLevel.offset << "," << mipmapLevel.length << "," << mipmapLevel.width << "," << mipmapLevel.height);
                mipmapList[level] = mipmapLevel;
            }
            level++;
        } while (findMipmapLevel != headers.end());
        mipmapArchive.mipmaps = mipmapList;

        allMipmaps[mipmapArchive.name] = mipmapArchive;
    }
    progData->mipmaps = allMipmaps;

    mesh->progressiveData = progData;
}

Mesh::VisualPtr ColladaSystem::load(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp,
            Transfer::DenseDataPtr data)
{
    ColladaDocumentLoader loader(metadata.getURI(), fp);

    SparseData data_reflatten = SparseData();
    data_reflatten.addValidData(data);

    Transfer::DenseDataPtr flatData = data_reflatten.flatten();

    char const* buffer = reinterpret_cast<char const*>(flatData->begin());
    loader.load(buffer, flatData->length());

    Mesh::MeshdataPtr mesh = loader.getMeshdata();
    addHeaderData(metadata, mesh);

    return mesh;
}

bool ColladaSystem::convertVisual(const Mesh::VisualPtr& visual, const String& format, const String& filename) {
    Mesh::MeshdataPtr meshdata(std::tr1::dynamic_pointer_cast<Mesh::Meshdata>(visual));
    if (!meshdata) return false;
    // format is ignored, we only know one format
    int result = meshdataToCollada(*meshdata, filename);
    return (result == 0);
}


} // namespace Models
} // namespace Sirikata
