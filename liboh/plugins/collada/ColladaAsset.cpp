/*  Sirikata liboh -- COLLADA Models Asset
 *  ColladaAsset.cpp
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

#include "ColladaAsset.hpp"

#include "COLLADAFWFileInfo.h"

#include <cassert>
#include <iostream>

namespace Sirikata { namespace Models {

ColladaAsset::ColladaAsset ()
{
    assert((std::cout << "MCB: ColladaAsset::ColladaAsset() entered" << std::endl,true));
    
}

ColladaAsset::ColladaAsset ( ColladaAsset const& rhs )
{
    assert((std::cout << "MCB: ColladaAsset::ColladaAsset(copy) entered" << std::endl,true));
    
}

ColladaAsset::ColladaAsset& ColladaAsset::operator = ( ColladaAsset const& rhs )
{
    assert((std::cout << "MCB: ColladaAsset::operator=() entered" << std::endl,true));
    return *this;
}

ColladaAsset::~ColladaAsset ()
{
    assert((std::cout << "MCB: ColladaAsset::~ColladaAsset() entered" << std::endl,true));
    
}

/////////////////////////////////////////////////////////////////////

bool ColladaAsset::import ( ColladaDocumentImporter& importer, COLLADAFW::FileInfo const& asset )
{
    assert((std::cout << "MCB: ColladaAsset::import() entered" << std::endl,true));

    bool ok = false;
    
    computeUpAxisRotation ( importer, asset );

    mUnitName = asset.getUnit ().getLinearUnitName ();
    mUnitMeter = asset.getUnit ().getLinearUnitMeter ();
    
    return ok;
}

/////////////////////////////////////////////////////////////////////

void ColladaAsset::computeUpAxisRotation ( ColladaDocumentImporter& importer, COLLADAFW::FileInfo const& asset )
{
    switch ( asset.getUpAxisType () )
    {
        case COLLADAFW::FileInfo::X_UP:
        {
            break;
        }
            
        case COLLADAFW::FileInfo::Y_UP:
        default:
        {
            break;
        }
            
        case COLLADAFW::FileInfo::Z_UP:
        {
            break;
        }
    }
}
    
/////////////////////////////////////////////////////////////////////


} // namespace Models
} // namespace Sirikata
