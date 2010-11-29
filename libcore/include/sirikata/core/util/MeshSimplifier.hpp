/*  Sirikata
 *  MeshSimplifier.hpp
 *
 *  Copyright (c) 2010, Tahir Azim.
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>

#include <sirikata/core/transfer/TransferData.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>

#include <sirikata/space/LocationService.hpp>


#include <sirikata/mesh/Meshdata.hpp>

namespace Sirikata {

class SIRIKATA_EXPORT MeshSimplifier {
private:

  double invert(Matrix4x4f& inv, Matrix4x4f& orig);

  typedef struct QSlimStruct {
    float mCost;
    int mGeomIdx, mPrimitiveIdx, mPrimitiveIndicesIdx;
    enum VectorCombination {ONE_TWO, TWO_THREE, ONE_THREE} ;

    VectorCombination mCombination;

    Vector3f mReplacementVector;

    QSlimStruct(float cost, int i, int j, int k, VectorCombination c, Vector3f v) {
      mCost = cost;
      mGeomIdx = i;
      mPrimitiveIdx = j;
      mPrimitiveIndicesIdx = k;
      mCombination = c;
      mReplacementVector = v;
    }

    bool operator < (const QSlimStruct& qs) const {
      return mCost < qs.mCost;
    }

  } QSlimStruct;


public:
  void simplify(std::tr1::shared_ptr<Meshdata> agg_mesh, uint32 numVerticesLeft);

};

}
