// -*- c-basic-offset: 2; -*-
/*  Sirikata
 *  AggregateManager.cpp
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

#include <sirikata/space/Platform.hpp>
#include "MeshAggregateManager.hpp"
#include "Options.hpp"

#include <sirikata/mesh/ModelsSystemFactory.hpp>
#include <sirikata/mesh/Bounds.hpp>
#include <sirikata/mesh/CompositeFilter.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/network/IOWork.hpp>

#include <sirikata/core/transfer/AggregatedTransferPool.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>

#include <sirikata/core/transfer/URL.hpp>
#include <json_spirit/json_spirit.h>
#include <boost/filesystem.hpp>

#include <sirikata/core/transfer/OAuthHttpManager.hpp>
#include <prox/rtree/RTreeCore.hpp>

#include <sirikata/core/command/Commander.hpp>

#include <boost/filesystem.hpp>



#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#define snprintf _snprintf
#endif

//This is an estimate of the solid angle for one pixel on a 2560*1600 screen resolution;
//Human Eye FOV = 4.17 sr, dividing that by (2560*1600).
#define HUMAN_FOV  4.17
#define ONE_PIXEL_SOLID_ANGLE (HUMAN_FOV/(2560.0*1600.0))
#define TWO_PI (2.0*3.14159)

#define AGG_LOG(lvl, msg) SILOG(aggregate-manager, lvl, msg)

using namespace std::tr1::placeholders;
using namespace Sirikata::Transfer;
using namespace Sirikata::Mesh;

namespace Sirikata {

Vector3f applyTransform(const Matrix4x4f& transform,
                        const Vector3f& v)
{
  Vector4f jth_vertex_4f = transform * Vector4f(v.x, v.y, v.z, 1.0f);
  return Vector3f(jth_vertex_4f.x, jth_vertex_4f.y, jth_vertex_4f.z);
}


// The following functions eigdc, mesh_covariance and pca_get_rotate_matrix
// copied and adapted from trimesh2 library
// (http://gfx.cs.princeton.edu/proj/trimesh2/)
// distributed under the GPL.
// Eigenvector decomposition for real, symmetric matrices,
// a la Bowdler et al. / EISPACK / JAMA
// Entries of d are eigenvalues, sorted smallest to largest.
// A changed in-place to have its columns hold the corresponding eigenvectors.
// Note that A must be completely filled in on input.
template <class T, int N>
static inline void eigdc(T A[N][N], T d[N])
{
        using namespace std;

        // Householder
        T e[N];
        for (int j = 0; j < N; j++) {
                d[j] = A[N-1][j];
                e[j] = T(0);
        }
        for (int i = N-1; i > 0; i--) {
                T scale = T(0);
                for (int k = 0; k < i; k++)
                        scale += fabs(d[k]);
                if (scale == T(0)) {
                        e[i] = d[i-1];
                        for (int j = 0; j < i; j++) {
                                d[j] = A[i-1][j];
                                A[i][j] = A[j][i] = T(0);
                        }
                        d[i] = T(0);
                } else {
                        T h(0);
                        T invscale = T(1) / scale;
                        for (int k = 0; k < i; k++) {
                                d[k] *= invscale;
                                h += (d[k]*d[k]);
                        }
                        T f = d[i-1];
                        T g = (f > T(0)) ? -sqrt(h) : sqrt(h);
                        e[i] = scale * g;
                        h -= f * g;
                        d[i-1] = f - g;
                        for (int j = 0; j < i; j++)
                                e[j] = T(0);
                        for (int j = 0; j < i; j++) {
                                f = d[j];
                                A[j][i] = f;
                                g = e[j] + f * A[j][j];
                                for (int k = j+1; k < i; k++) {
                                        g += A[k][j] * d[k];
                                        e[k] += A[k][j] * f;
                                }
                                e[j] = g;
                        }
                        f = T(0);
                        T invh = T(1) / h;
                        for (int j = 0; j < i; j++) {
                                e[j] *= invh;
                                f += e[j] * d[j];
                        }
                        T hh = f / (h + h);
                        for (int j = 0; j < i; j++)
                                e[j] -= hh * d[j];
                        for (int j = 0; j < i; j++) {
                                f = d[j];
                                g = e[j];
                                for (int k = j; k < i; k++)
                                        A[k][j] -= f * e[k] + g * d[k];
                                d[j] = A[i-1][j];
                                A[i][j] = T(0);
                        }
                        d[i] = h;
                }
        }

        for (int i = 0; i < N-1; i++) {
                A[N-1][i] = A[i][i];
                A[i][i] = T(1);
                T h = d[i+1];
                if (h != T(0)) {
                        T invh = T(1) / h;
                        for (int k = 0; k <= i; k++)
                                d[k] = A[k][i+1] * invh;
                        for (int j = 0; j <= i; j++) {
                                T g = T(0);
                                for (int k = 0; k <= i; k++)
                                        g += A[k][i+1] * A[k][j];
                                for (int k = 0; k <= i; k++)
                                        A[k][j] -= g * d[k];
                        }
                }
                for (int k = 0; k <= i; k++)
                        A[k][i+1] = T(0);
        }
        for (int j = 0; j < N; j++) {
                d[j] = A[N-1][j];
                A[N-1][j] = T(0);
        }
        A[N-1][N-1] = T(1);

        // QL
        for (int i = 1; i < N; i++)
                e[i-1] = e[i];
        e[N-1] = T(0);
        T f = T(0), tmp = T(0);
        const T eps = T(pow(2.0, -52.0));
        for (int l = 0; l < N; l++) {
                tmp = max(tmp, fabs(d[l]) + fabs(e[l]));
                int m = l;
                while (m < N) {
                        if (fabs(e[m]) <= eps * tmp)
                                break;
                        m++;
                }
                if (m > l) {
                        do {
                                T g = d[l];
                                T p = (d[l+1] - g) / (e[l] + e[l]);
                                T r = T(hypot(p, T(1)));
                                if (p < T(0))
                                        r = -r;
                                d[l] = e[l] / (p + r);
                                d[l+1] = e[l] * (p + r);
                                T dl1 = d[l+1];
                                T h = g - d[l];
                                for (int i = l+2; i < N; i++)
                                        d[i] -= h;
                                f += h;
                                p = d[m];
                                T c = T(1), c2 = T(1), c3 = T(1);
                                T el1 = e[l+1], s = T(0), s2 = T(0);
                                for (int i = m - 1; i >= l; i--) {
                                        c3 = c2;
                                        c2 = c;
                                        s2 = s;
                                        g = c * e[i];
                                        h = c * p;
                                        r = T(hypot(p, e[i]));
                                        e[i+1] = s * r;
                                        s = e[i] / r;
                                        c = p / r;
                                        p = c * d[i] - s * g;
                                        d[i+1] = h + s * (c * g + s * d[i]);
                                        for (int k = 0; k < N; k++) {
                                                h = A[k][i+1];
                                                A[k][i+1] = s * A[k][i] + c * h;
                                                A[k][i] = c * A[k][i] - s * h;
                                        }
                                }
                                p = -s * s2 * c3 * el1 * e[l] / dl1;
                                e[l] = s * p;
                                d[l] = c * p;
                        } while (fabs(e[l]) > eps * tmp);
                }
                d[l] += f;
                e[l] = T(0);
        }

        // Sort
        for (int i = 0; i < N-1; i++) {
                int k = i;
                T p = d[i];
                for (int j = i+1; j < N; j++) {
                        if (d[j] < p) {
                                k = j;
                                p = d[j];
                        }
                }
                if (k == i)
                        continue;
                d[k] = d[i];
                d[i] = p;
                for (int j = 0; j < N; j++) {
                        p = A[j][i];
                        A[j][i] = A[j][k];
                        A[j][k] = p;
                }
        }
}

// Compute covariance of faces (area-weighted) in a mesh
void mesh_covariance(std::tr1::unordered_set<Vector3f, Vector3f::Hasher>& positionVectors,
                     std::tr1::unordered_set<FaceContainer, FaceContainer::Hasher>& faceSet,
                     float C[3][3])
{
        for (int j = 0; j < 3; j++)
                for (int k = 0; k < 3; k++)
                        C[j][k] = 0.0f;

        float totarea = 0.0f;
        for (std::tr1::unordered_set<FaceContainer, FaceContainer::Hasher>::iterator face_it = faceSet.begin();
             face_it != faceSet.end();
             face_it++)
        {
                FaceContainer f = *face_it;
                Vector3f c = (f.v1 + f.v2 + f.v3) / 3.0f;
                float area = (f.v1-f.v2).cross(f.v1-f.v3).length() * 0.5;
                totarea += area;

                // Covariance of triangle relative to centroid
                float vweight = area / 12.0f;
                for (int v = 0; v < 3; v++) {
                        Vector3f currv;
                        if (v == 0) currv = f.v1;
                        else if (v == 1) currv = f.v2;
                        else if (v == 2) currv = f.v3;
                        Vector3f pc = currv - c;
                        for (int j = 0; j < 3; j++)
                                for (int k = j; k < 3; k++)
                                        C[j][k] += vweight * pc[j] * pc[k];
                }

                // Covariance of centroid
                for (int j = 0; j < 3; j++)
                        for (int k = j; k < 3; k++)
                                C[j][k] += area * c[j] * c[k];
        }

        for (int j = 0; j < 3; j++)
                for (int k = j; k < 3; k++)
                        C[j][k] /= totarea;

        C[1][0] = C[0][1];        C[2][0] = C[0][2];
        C[2][1] = C[1][2];
}


void MeshAggregateManager::pca_get_rotate_matrix(MeshdataPtr mesh, Matrix4x4f& rot_mat, Matrix4x4f& rot_mat_inv) {
        std::tr1::unordered_set<Vector3f, Vector3f::Hasher> positionVectors;
        std::tr1::unordered_set<FaceContainer, FaceContainer::Hasher> faceSet;
        getVertexAndFaceList(mesh, positionVectors, faceSet);

        float C[3][3];
        mesh_covariance(positionVectors, faceSet, C);
        float e[3];
        eigdc<float,3>(C, e);

        // Sorted in order from smallest to largest, so grab third column
        Vector3f first(C[0][2], C[1][2], C[2][2]);
        int npos = 0;
        int nv = positionVectors.size();
        for (std::tr1::unordered_set<Vector3f, Vector3f::Hasher>::iterator it = positionVectors.begin();
             it != positionVectors.end(); it++)
        {
                if ( (*it).dot(first) > 0.0f)
                        npos++;
        }
        if (npos < nv/2)
                first = -first;

        Vector3f second(C[0][1], C[1][1], C[2][1]);
        npos = 0;
        for (std::tr1::unordered_set<Vector3f, Vector3f::Hasher>::iterator it = positionVectors.begin();
             it != positionVectors.end(); it++) {
                if ( (*it).dot(second) > 0.0f)
                        npos++;
        }
        if (npos < nv/2)
                second = -second;

        Vector3f third = first.cross(second);

        Matrix4x4f xf = Matrix4x4f::identity();
        xf(0,0) = first[0];  xf(1,0) = first[1];  xf(2,0) = first[2];
        xf(0,1) = second[0]; xf(1,1) = second[1]; xf(2,1) = second[2];
        xf(0,2) = third[0];  xf(1,2) = third[1];  xf(2,2) = third[2];

        rot_mat_inv = xf;
        xf.invert(rot_mat);
}


MeshAggregateManager::MeshAggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username)
 :  mLoc(loc),
    mAtlasingNeeded(false),
    mSizeOfSeenTextures(0),
    mOAuth(oauth),
    mCDNUsername(username),
    mModelTTL(Duration::minutes(60)),
    mCDNKeepAlivePoller(NULL),
    mRawAggregateUpdates(0),
    mAggregatesQueued(0),
    mAggregatesGenerated(0),
    mAggregatesFailedToGenerate(0),
    mAggregatesUploaded(0),
    mAggregatesFailedToUpload(0),
    mAggregateCumulativeGenerationTime(Duration::zero()),
    mAggregateCumulativeUploadTime(Duration::zero()),
    mAggregateCumulativeDataSize(0),
    mErrorSum(0.0),
    mErrorSequenceNumber(0),
    mSizeSum(0), noMoreGeneration(false),
    mCurrentInsertionNumber(0)
{
    String local_path = GetOptionValue<String>(OPT_AGGMGR_LOCAL_PATH);
    String local_url_prefix = GetOptionValue<String>(OPT_AGGMGR_LOCAL_URL_PREFIX);
    uint16 n_gen_threads = GetOptionValue<uint16>(OPT_AGGMGR_GEN_THREADS);
    uint16 n_upload_threads = GetOptionValue<uint16>(OPT_AGGMGR_UPLOAD_THREADS);
    bool skip_gen = GetOptionValue<bool>(OPT_AGGMGR_SKIP_GENERATE);
    bool skip_upload = GetOptionValue<bool>(OPT_AGGMGR_SKIP_UPLOAD);
    mLocalPath = local_path;
    mLocalURLPrefix = local_url_prefix;
    mNumGenerationThreads = std::min(n_gen_threads, (uint16)MAX_NUM_GENERATION_THREADS);
    mNumUploadThreads = std::min(n_upload_threads, (uint16)MAX_NUM_UPLOAD_THREADS);
    mSkipGenerate = skip_gen;
    mSkipUpload = skip_gen || skip_upload;

    mModelsSystem = NULL;
    if (ModelsSystemFactory::getSingleton().hasConstructor("any"))
        mModelsSystem = ModelsSystemFactory::getSingleton().getConstructor("any")("");
    mLoc->addListener(this, true);

    std::vector<String> names_and_args;
    names_and_args.push_back("triangulate"); names_and_args.push_back("all");
    names_and_args.push_back("center"); names_and_args.push_back("");
    mCenteringFilter = new Mesh::CompositeFilter(names_and_args);

    names_and_args.clear();
    names_and_args.push_back("squash-instanced-geometry"); names_and_args.push_back("");

    mSquashFilter = new Mesh::CompositeFilter(names_and_args);

    mTransferMediator = &(Transfer::TransferMediator::getSingleton());

    static char x = '1';
    mTransferPool = mTransferMediator->registerClient<Transfer::AggregatedTransferPool>(String("SpaceAggregator_")+x);
    x++;

    char id='1';
    memset(mUploadThreads, 0, sizeof(mUploadThreads));
    memset(mUploadServices, 0, sizeof(mUploadServices));
    memset(mUploadStrands, 0, sizeof(mUploadStrands));
    memset(mUploadWorks, 0, sizeof(mUploadWorks));
    for (uint8 i = 0; i < mNumUploadThreads; i++) {
      mUploadServices[i] = new Network::IOService(String("MeshAggregateManager::UploadService")+id);
      mUploadStrands[i] = mUploadServices[i]->createStrand(String("MeshAggregateManager::UploadStrand")+id);
      mUploadWorks[i] =new Network::IOWork(mUploadServices[i], String("MeshAggregateManager::UploadWork")+id);
      mUploadThreads[i] = new Thread("MeshAggregateManager Upload", std::tr1::bind(&MeshAggregateManager::uploadThreadMain, this, i));

      id++;
    }

    // Start the processing threads
    id = '1';
    memset(mAggregationThreads, 0, sizeof(mAggregationThreads));
    memset(mAggregationServices, 0, sizeof(mAggregationServices));
    memset(mAggregationStrands, 0, sizeof(mAggregationStrands));
    memset(mIOWorks, 0, sizeof(mIOWorks));
    for (uint8 i = 0; i < mNumGenerationThreads; i++) {
      mAggregationServices[i] = new Network::IOService(String("MeshAggregateManager::AggregationService")+id);
      mAggregationStrands[i] = mAggregationServices[i]->createStrand(String("MeshAggregateManager::AggregationStrand")+id);
      mIOWorks[i] =new Network::IOWork(mAggregationServices[i], String("MeshAggregateManager::AggregationWork")+id);
      mAggregationThreads[i] = new Thread(String("MeshAggregateManager Thread ")+id, std::tr1::bind(&MeshAggregateManager::aggregationThreadMain, this, i));

      id++;
    }

    if (mAggregationStrands[0]) {
      mCDNKeepAlivePoller =  new Poller( mAggregationStrands[0],
        std::tr1::bind(&MeshAggregateManager::sendKeepAlives, this),
        "MeshAggregateManager CDN Keep-Alive Poller",
        Duration::minutes(5)  );
    }

    removeStaleLeaves();

    if (mCDNKeepAlivePoller)
      mCDNKeepAlivePoller->start();

    if (mLoc->context()->commander()) {
      mLoc->context()->commander()->registerCommand(
        "space.aggregates.stats",
        std::tr1::bind(&MeshAggregateManager::commandStats, this, _1, _2, _3)
      );
    }
}

MeshAggregateManager::~MeshAggregateManager() {
  // We need to make sure we clean this up before the IOService and IOStrand
  // it's running on.
  if (mCDNKeepAlivePoller) {
    mCDNKeepAlivePoller->stop();
    delete mCDNKeepAlivePoller;
  }

    // Shut down the main processing thread
    for (uint8 i = 0; i < mNumGenerationThreads; i++) {
      if (mAggregationThreads[i] != NULL) {
        if (mAggregationServices[i] != NULL)
            mAggregationServices[i]->stop();
        mAggregationThreads[i]->join();
      }

      delete mIOWorks[i];
      delete mAggregationStrands[i];
      delete mAggregationServices[i];

      delete mAggregationThreads[i];
    }


    //Shutdown the upload threads.
    for (uint8 i = 0; i < mNumUploadThreads; i++) {
      if (mUploadThreads[i] != NULL) {
        if (mUploadServices[i] != NULL)
            mUploadServices[i]->stop();
        mUploadThreads[i]->join();
      }

      delete mUploadWorks[i];
      delete mUploadStrands[i];
      delete mUploadServices[i];

      delete mUploadThreads[i];
    }

    delete mCenteringFilter;
    delete mSquashFilter;
    //Delete the model system.
    delete mModelsSystem;
}

// The main loop for the prox processing thread
void MeshAggregateManager::aggregationThreadMain(uint8 i) {
  mAggregationServices[i]->run();
}

void MeshAggregateManager::uploadThreadMain(uint8 i) {
  mUploadServices[i]->run();
}

void MeshAggregateManager::addAggregate(const UUID& uuid) {
  AGG_LOG(detailed, "addAggregate called: uuid=" << uuid.toString() << "\n");
  mRawAggregateUpdates++;

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  mAggregateObjects[uuid] = AggregateObjectPtr (new AggregateObject(uuid, UUID::null(), false));
}

void MeshAggregateManager::getVertexAndFaceList(
                             Mesh::MeshdataPtr md,
                             std::tr1::unordered_set<Vector3f, Vector3f::Hasher>& positionVectors,
                             std::tr1::unordered_set<FaceContainer, FaceContainer::Hasher>& faceSet
                           )
{
  Meshdata::GeometryInstanceIterator geoinst_it = md->getGeometryInstanceIterator();
  uint32 geoinst_idx;
  Matrix4x4f geoinst_pos_xform;

  std::tr1::unordered_map<Vector3f, std::tr1::unordered_set<Vector3f, Vector3f::Hasher>, Vector3f::Hasher> neighborVertices;

  geoinst_it = md->getGeometryInstanceIterator();
  while( geoinst_it.next(&geoinst_idx, &geoinst_pos_xform) ) {
    const GeometryInstance& geomInstance = md->instances[geoinst_idx];
    SubMeshGeometry& curGeometry = md->geometry[geomInstance.geometryIndex];
    int i = geomInstance.geometryIndex;

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {

      if (curGeometry.primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;

      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {

        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];

        Vector3f pos1 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx]);
        Vector3f pos2 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx2]);
        Vector3f pos3 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx3]);

        FaceContainer face(pos1,pos2,pos3);
        if (faceSet.find(face) != faceSet.end()) {
          continue;
        }
        faceSet.insert(FaceContainer(pos1,pos2,pos3));

        positionVectors.insert(pos1);
        positionVectors.insert(pos2);
        positionVectors.insert(pos3);

      }
    }
  }
}

bool MeshAggregateManager::cleanUpChild(const UUID& parent_id, const UUID& child_id) {
    // *Must* have already locked mAggregateObjectsMutex

    // This cleans up the tree so we don't have any stale pointers.
    //
    // The approach isn't really ideal, but since we add individual leaf objects
    // as aggregates and don't get feedback on their removal, we have to take
    // this approach.
    //
    // You would think that we could just remove all children since we should
    // get removals bottom up. While we do get removals in that order, it is
    // also possible for a tree to end up collapsing -- if a node ends up with
    // only one child, the parent can be removed, violating the expected
    // condition that the children will be leaves (and therefore have no
    // children of their own). So we detect these two possibilities and either
    // remove the leaf object or cleanup the parent pointer for the aggregate
    // (since we sometimes walk up parent pointers and it will clearly not be
    // valid anymore).

    if (mAggregateObjects.find(child_id) == mAggregateObjects.end()) {
      return true;
    }

    if (!mAggregateObjects[child_id]->leaf) {
        // Aggregate that's getting removed because it only has one child
        // left, just remove the parent pointer
        mAggregateObjects[child_id]->mParentUUIDs.erase(parent_id);
        return false;
    }

    return true;
}

void MeshAggregateManager::removeAggregate(const UUID& uuid) {
  AGG_LOG(detailed, "removeAggregate: " << uuid.toString() << "\n");
  mRawAggregateUpdates++;

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

  // Cleans up children if necessary, or makes sure they at least don't refer to
  // this object anymore. See cleanUpChild for details.
  AggregateObjectPtr agg = mAggregateObjects[uuid];
  const std::vector<AggregateObjectPtr> children = agg->getChildrenCopy();
  for (std::vector<AggregateObjectPtr>::const_iterator child_it = children.begin(); child_it != children.end(); child_it++)
    cleanUpChild(uuid, (*child_it)->mUUID);

  mAggregateObjects.erase(uuid);
}

void MeshAggregateManager::addChild(const UUID& uuid, const UUID& child_uuid) {
  AggregateObjectPtr agg;
  {
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    assert(mAggregateObjects.find(uuid) != mAggregateObjects.end());
    agg = mAggregateObjects[uuid];
    assert(agg);
  }

  if (!agg->hasChild(child_uuid)) {
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    if (mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
        // TODO(ewencp,tazim) This alone clearly isn't right -- we'll
        // never get a signal that these "aggregates" (which are
        // really just individual objects that don't need aggregation)
        // have been removed, so they'll never be cleaned up... Currently we
        // just remove them when they are left as children of an aggregate (see
        // removeAggregate).
        mAggregateObjects[child_uuid] = std::tr1::shared_ptr<AggregateObject> (new AggregateObject(child_uuid, uuid, true));
    }
    else {
      mAggregateObjects[child_uuid]->mParentUUIDs.insert(uuid);
    }

    agg->addChild(mAggregateObjects[child_uuid]);

    updateChildrenTreeLevel(uuid, mAggregateObjects[uuid]->mTreeLevel);

    addDirtyAggregates(child_uuid);

    mAggregateGenerationStartTime = Timer::now();

    lock.unlock();

    AGG_LOG(detailed, "addChild:  "  << uuid.toString() << " CHILD " << child_uuid.toString() << "\n");
    mRawAggregateUpdates++;

    if (mAggregationStrands[0]) {
      mAggregationStrands[0]->post(
        Duration::seconds(10),
        std::tr1::bind(&MeshAggregateManager::queueDirtyAggregates, this, mAggregateGenerationStartTime),
        "MeshAggregateManager::queueDirtyAggregates"
      );
    }
  }
}

void MeshAggregateManager::iRemoveChild(const UUID& uuid, const UUID& child_uuid) {
  AGG_LOG(detailed, "removeChild:  "  << uuid.toString() << " CHILD " << child_uuid.toString() << "\n");
  mRawAggregateUpdates++;

  assert(mAggregateObjects.find(uuid) != mAggregateObjects.end());
  AggregateObjectPtr agg = mAggregateObjects[uuid];
  assert(agg);

  if ( agg->hasChild(child_uuid)  ) {
    agg->removeChild( child_uuid );

    // Cleans up the child if necessary, or makes sure it doesn't still refer to
    // this object anymore. See cleanUpChild for details.
    bool child_removed = cleanUpChild(uuid, child_uuid);

    addDirtyAggregates(uuid);

    mAggregateGenerationStartTime =  Timer::now();

    if (mAggregationStrands[0]) {
      mAggregationStrands[0]->post(
        Duration::seconds(10),
        std::tr1::bind(&MeshAggregateManager::queueDirtyAggregates, this, mAggregateGenerationStartTime),
        "MeshAggregateManager::queueDirtyAggregates"
      );
    }
  }

}

void MeshAggregateManager::removeChild(const UUID& uuid, const UUID& child_uuid) {
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

  iRemoveChild(uuid, child_uuid);
}

void MeshAggregateManager::aggregateObserved(const UUID& objid, uint32 nobservers, uint32 nchildren) {
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

  if (mAggregateObjects.find(objid) != mAggregateObjects.end()) {
    AggregateObjectPtr aggObj = mAggregateObjects[objid];
    aggObj->mNumObservers = nobservers;

    /*int64 serializedSize;
    if (aggObj->childrenSize() > 0) {
      serializedSize = aggObj->mSerializedSize;
    }
    else {
      std::tr1::shared_ptr<LocationInfo> locInfoForUUID = getCachedLocInfo(objid);
      String name = locInfoForUUID->mesh();
      assert(mMeshSizeMap.find(name) != mMeshSizeMap.end());
      serializedSize =  mMeshSizeMap[name];
    }

    std::cout << mErrorSum << " : previous error\n";
    std::cout << mSizeSum << " : previous size\n";

    if (nobservers == 0) {
      mErrorSum -= aggObj->geometricError;
      mSizeSum -= serializedSize;
      if (mSizeSum < 0) mSizeSum = 0;
      if (mErrorSum < 0) mErrorSum = 0;
      mObservers.erase(aggObj->mUUID);
    }
    else {
      assert(mObservers.find(aggObj->mUUID) == mObservers.end());
      mObservers.insert(aggObj->mUUID);
      mErrorSum += aggObj->geometricError;
      mSizeSum += serializedSize;
      mErrorSequenceNumber++;

      //mUploadStrands[0]->post(Duration::seconds(5),
      //    std::tr1::bind(&MeshAggregateManager::showCutErrorSum, this, mErrorSequenceNumber),
      //    "MeshAggregateManager::showCutErrorSum");
    }

    std::cout << mErrorSum << " : new error\n";
    std::cout << mSizeSum << " : new size\n";
    fflush(stdout);

    assert(nobservers < 2);
    */
    std::cout << "Observed tree level " << aggObj->mTreeLevel << "_"
              << aggObj->mUUID  <<  " : nobservers=" <<  nobservers << "\n";

    if (aggObj->mParentUUIDs.begin() != aggObj->mParentUUIDs.end()) {

      for (std::set<UUID>::iterator it = aggObj->mParentUUIDs.begin();
           it != aggObj->mParentUUIDs.end();  it++)
      {
        if (*it != UUID::null()) std::cout << " parent "  << (*it)   << "\n";
      }
    }
  }
}

void MeshAggregateManager::generateAggregateMesh(const UUID& uuid, const Duration& delayFor) {
  mRawAggregateUpdates++;
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) return;
  std::tr1::shared_ptr<AggregateObject> aggObject = mAggregateObjects[uuid];
  lock.unlock();
  generateAggregateMesh(uuid, aggObject, delayFor);
}

void MeshAggregateManager::generateAggregateMesh(const UUID& uuid, AggregateObjectPtr aggObject, const Duration& delayFor) {
  if (mModelsSystem == NULL) return;
  if (mDirtyAggregateObjects.find(uuid) != mDirtyAggregateObjects.end()) return;

  AGG_LOG(detailed,"Setting up aggregate " << uuid << " to generate aggregate mesh with " << aggObject->childrenSize() << " in " << delayFor);

  addDirtyAggregates(uuid);
  mAggregateGenerationStartTime = Timer::now();

  if (mAggregationStrands[0]) {
    mAggregateGenerationStartTime = Timer::now();
    mAggregationStrands[0]->post(
        Duration::seconds(10),
        std::tr1::bind(&MeshAggregateManager::queueDirtyAggregates, this, mAggregateGenerationStartTime),
        "MeshAggregateManager::queueDirtyAggregates"
    );
  }
}

void MeshAggregateManager::deduplicateMeshes(std::vector<AggregateObjectPtr>& children, bool isLeafAggregate,
                       String* meshURIs, std::vector<Matrix4x4f>& replacementAlignmentTransforms)
{
  std::tr1::unordered_map<uint32, bool> replacedURI;
  Prox::DescriptorReader* descriptorReader=Prox::DescriptorReader::getDescriptorReader();

  for (uint32 i = 0; i < children.size(); i++) {
    replacementAlignmentTransforms.push_back(Matrix4x4f::identity());
  }

  for (uint32 i=0; isLeafAggregate && i<children.size(); i++) {
    if (meshURIs[i] == "") continue;

    Prox::ZernikeDescriptor zd_i = descriptorReader->getZernikeDescriptor(meshURIs[i]);
    Prox::ZernikeDescriptor td_i = descriptorReader->getTextureDescriptor(meshURIs[i]);

    {
      boost::mutex::scoped_lock lock(mMeshStoreMutex);

      if (mMeshDescriptors.find(meshURIs[i]) != mMeshDescriptors.end()) {
        zd_i = mMeshDescriptors[meshURIs[i]];
        AGG_LOG(insane, meshURIs[i] << " : " << zd_i.toString() << " : Got zd 1");
      }
    }

    if (zd_i.size() == 0 || td_i.size() == 0) {
      AGG_LOG(debug, "Zernike Descriptor or Texture Descriptor invalid -- skipping mesh deduplication");
      continue;
    }

    for (uint32 j = i+1; j<children.size(); j++) {
      if (replacedURI.find(j) == replacedURI.end() || replacedURI[j] == false) {
        if (meshURIs[j] == "" || meshURIs[j] == meshURIs[i]) continue;

        Prox::ZernikeDescriptor zd_j = descriptorReader->getZernikeDescriptor(meshURIs[j]);
        {
          boost::mutex::scoped_lock lock(mMeshStoreMutex);

          if (mMeshDescriptors.find(meshURIs[j]) != mMeshDescriptors.end()) {
            zd_j = mMeshDescriptors[meshURIs[j]];
            AGG_LOG(insane, meshURIs[j] << " : " << zd_j.toString() << " : Got zd 2\n");
          }
        }

        AGG_LOG(insane, zd_i.toString() << "\t" << zd_j.toString() );
        float64 zd_diff = zd_i.minus(zd_j).l2Norm();

        if ( zd_diff < 0.01) {
          Prox::ZernikeDescriptor td_j = descriptorReader->getTextureDescriptor(meshURIs[j]);

          if (zd_j.size() == 0 || td_j.size() == 0) {
            AGG_LOG(debug, "Zernike Descriptor or Texture Descriptor invalid -- skipping mesh deduplication");
            continue;
          }

          float64 td_diff = td_j.minus(td_i).l1Norm();
          if (td_diff < 250) {
            boost::mutex::scoped_lock lock(mMeshStoreMutex);

            if (mMeshStore.find(meshURIs[j]) == mMeshStore.end()) continue;
            if (mMeshStore.find(meshURIs[i]) == mMeshStore.end()) continue;

            if (!mMeshStore[meshURIs[j]]  || !mMeshStore[meshURIs[i]] ) continue;


            Matrix4x4f xf1, xf1_inv;
            pca_get_rotate_matrix(mMeshStore[meshURIs[j] ] , xf1, xf1_inv);
            Matrix4x4f xf2, xf2_inv;
            pca_get_rotate_matrix(mMeshStore[meshURIs[i] ], xf2, xf2_inv);

            replacementAlignmentTransforms[j] = xf1_inv * xf2;

            AGG_LOG(info, "Replacing " << meshURIs[j]  << " with " << meshURIs[i] << " -- " <<
                         "zdiff: " << zd_diff << " , tdiff: "<< td_diff  << " : " <<replacementAlignmentTransforms[j] );

            replacedURI[j] = true;
            meshURIs[j] = meshURIs[i];
          }
        }
      }
    }
  }
}

uint32 MeshAggregateManager::checkLocAndMeshInfo(
                       const UUID& uuid, std::vector<AggregateObjectPtr>& children,
                       std::tr1::unordered_map<UUID, std::tr1::shared_ptr<LocationInfo> , UUID::Hasher>& currentLocMap)
{
  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;

    std::tr1::shared_ptr<LocationInfo> locInfoForChildUUID = getCachedLocInfo(child_uuid);
    if (!locInfoForChildUUID ) {
      AGG_LOG(detailed, "Missing chlid loc info: " << uuid << ", for child " << child_uuid);
      return MISSING_CHILD_LOC_INFO;
    }
    currentLocMap[child_uuid] = locInfoForChildUUID;

    if (isAggregate(child_uuid) && locInfoForChildUUID->mesh() == "") {
      AGG_LOG(info,  "Not yet generated: " << child_uuid << " for " << uuid);
      return CHILDREN_NOT_YET_GEN;
    }
  }

  return 0;
}

uint32 MeshAggregateManager::checkMeshesAvailable(const UUID& uuid, const Time& curTime,
                                              std::vector<AggregateObjectPtr>& children,
                                              std::tr1::unordered_map<UUID, std::tr1::shared_ptr<LocationInfo> , UUID::Hasher>& currentLocMap)
{
  bool allMeshesAvailable = true;
  String missingChildMesh= "";
  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      continue;
    }
    Mesh::MeshdataPtr m = mAggregateObjects[child_uuid]->mMeshdata;

    if (!m) {
      //request a download or generation of the mesh
      std::string meshName = currentLocMap[child_uuid]->mesh();

      if (meshName != "") {
        boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
        if (mMeshStore.find(meshName) == mMeshStore.end()) {

          Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &MeshAggregateManager::metadataFinished, this, curTime, uuid, child_uuid, meshName,
                                       1, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

          mTransferPool->addRequest(req);

          allMeshesAvailable = false;
          missingChildMesh=meshName;

          //Store an empty pointer in mMeshStore so that further transfer requests are
          //not made for the same meshname.
          mMeshStore[meshName] = MeshdataPtr();
          AGG_LOG(insane, "Mesh not available: " << meshName << " : requested..\n");
        }
        else if (!mMeshStore[meshName]) {
          AGG_LOG(insane, "Mesh not available, waiting in store: " << meshName);
          missingChildMesh = meshName;
          allMeshesAvailable = false;
        }
      }
      else {
      }
    }
  }
  if (!allMeshesAvailable) {
    AGG_LOG(insane, missingChildMesh  << " --  mesh not available\n");

    return MISSING_CHILD_MESHES;
  }

  // Make sure we've got all the Meshdatas
  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      continue;
    }
    MeshdataPtr m = mAggregateObjects[child_uuid]->mMeshdata;

    std::string meshName = currentLocMap[child_uuid]->mesh();
    if (!m && meshName != "") {
        boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
        if (mMeshStore.find(meshName) != mMeshStore.end()) {
          mAggregateObjects[child_uuid]->mMeshdata = mMeshStore[meshName];
        }
    }
  }

  return 0;
}

uint32 MeshAggregateManager::checkTextureHashesAvailable(std::vector<AggregateObjectPtr>& children,
                                                    std::tr1::unordered_map<String, String>& textureToHashMap)

{
    boost::mutex::scoped_lock textureToHashMapLock(mTextureNameToHashMapMutex);
    bool allTextureHashesAvailable = true;

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    for (uint32 i = 0; i < children.size(); i++) {
      UUID child_uuid = children[i]->mUUID;

      if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
        continue;
      }
      MeshdataPtr m = mAggregateObjects[child_uuid]->mMeshdata;
      if (!m) continue;

      for (uint32 j = 0; j < m->textures.size(); j++) {
        if (textureToHashMap.find(m->textures[j]) != textureToHashMap.end())
          continue;

        if (mTextureNameToHashMap.find(m->textures[j]) == mTextureNameToHashMap.end())
        {
          allTextureHashesAvailable = false;
          //std::cout << m->textures[j] << " : not available\n";
          Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(m->textures[j]), 1.0, std::tr1::bind(
                                       &MeshAggregateManager::textureDownloadedForCounting, this, m->textures[j],
                                       1, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

          mTransferPool->addRequest(req);

          continue;
        }
        textureToHashMap[m->textures[j]] = mTextureNameToHashMap[m->textures[j]];
      }
    }
    if (!allTextureHashesAvailable) {
      AGG_LOG(insane, "texture hash not available\n");
      return MISSING_CHILD_MESHES;
    }

    return 0;
}

void MeshAggregateManager::startDownloadsForAtlasing(const UUID& uuid, MeshdataPtr agg_mesh, AggregateObjectPtr aggObject, String localMeshName,
                                                 std::tr1::unordered_map<String, String>& textureSet,
                                                 std::tr1::unordered_map<String, MeshdataPtr>& textureToModelMap) {
    //Save the mesh to a temporary directory until textures are downloaded and atlased.
    {
      boost::mutex::scoped_lock modelSystemLock(mModelsSystemMutex);
      boost::filesystem::create_directory( "/tmp/sirikata/"+uuid.toString()+".dir");
      bool converted = mModelsSystem->convertVisual( agg_mesh, "colladamodels", "/tmp/sirikata/"+uuid.toString()+".dir/"+uuid.toString()+".dae");
    }
    std::vector<ResourceDownloadTaskPtr> downloadTasks;
    std::tr1::shared_ptr<std::tr1::unordered_map<String, int> > downloadedTexturesMap =
                        std::tr1::shared_ptr<std::tr1::unordered_map<String, int> >(new std::tr1::unordered_map<String, int>());
    std::tr1::shared_ptr<std::tr1::unordered_map<String, std::vector<String> > > hashToURIMap =
                        std::tr1::shared_ptr<std::tr1::unordered_map<String, std::vector<String> > >(new std::tr1::unordered_map<String, std::vector<String> > () );

    //download the textures or their low-res (128x128) mipmaps, if available.
    for(TextureList::iterator it = agg_mesh->textures.begin(); it != agg_mesh->textures.end(); it++) {
            std::string texName = *it;
            Transfer::URI texURI( texName );
            MeshdataPtr md = textureToModelMap[texName];

            ProgressiveMipmapMap::const_iterator findProgTex;
            if (md->progressiveData) {
              String lookupName = texName.substr(0,texName.find_last_of("/"));
              AGG_LOG(insane, texName << " : " << lookupName << "\n");
              lookupName = lookupName.substr(lookupName.find_last_of("/")+1);
              findProgTex = md->progressiveData->mipmaps.find("./" + lookupName);
              AGG_LOG(insane,  lookupName << " : lookupName\n");
            }

            if (md->progressiveData && findProgTex != md->progressiveData->mipmaps.end()) {
                AGG_LOG(insane, "findProgTex worked!\n");
                const ProgressiveMipmaps& progMipmaps = findProgTex->second.mipmaps;
                uint32 mipmapLevel = 0;
                for ( ; mipmapLevel < progMipmaps.size(); mipmapLevel++) {
                    if (progMipmaps.find(mipmapLevel)->second.width >= 1024 || progMipmaps.find(mipmapLevel)->second.height >= 1024) {
                        break;
                    }
                }
                if (mipmapLevel >= progMipmaps.size() ) mipmapLevel = progMipmaps.size() - 1;

                uint32 offset = progMipmaps.find(mipmapLevel)->second.offset;
                uint32 length = progMipmaps.find(mipmapLevel)->second.length;
                Transfer::Fingerprint hash = findProgTex->second.archiveHash;

                (*downloadedTexturesMap)[texName] = 0;
                (*hashToURIMap)[hash.toString()].push_back(texName);

                ResourceDownloadTaskPtr dl = ResourceDownloadTask::construct(
                  Transfer::Chunk(hash, Transfer::Range(offset, length, Transfer::LENGTH)), mTransferPool,
                  1.0,
                  std::tr1::bind(&MeshAggregateManager::textureChunkFinished, this, texName, hash, offset, length, agg_mesh, aggObject, textureSet, downloadedTexturesMap, hashToURIMap, 0, _1, _2, _3)
                );
                downloadTasks.push_back(dl);

                AGG_LOG(insane,  "Requesting... :  " << texName <<  " with hash " << hash  << "\n");

                boost::mutex::scoped_lock resourceDownloadLock(mResourceDownloadTasksMutex);
                mResourceDownloadTasks[dl->getIdentifier()+" : " +localMeshName + " : " + aggObject->mUUID.toString()] = dl;
            } else {
                (*downloadedTexturesMap)[texName] = 0;
                (*hashToURIMap)[texName].push_back(texName);
                ResourceDownloadTaskPtr dl = ResourceDownloadTask::construct(
                  texURI, mTransferPool,
                  1.0,
                  std::tr1::bind(&MeshAggregateManager::textureChunkFinished, this, texName, Transfer::Fingerprint(), 0, 0, agg_mesh, aggObject, textureSet, downloadedTexturesMap, hashToURIMap, 0,  _1, _2, _3)
               );
               downloadTasks.push_back(dl);
               AGG_LOG(insane,  "Requesting... :  " << texName << "\n");

               boost::mutex::scoped_lock resourceDownloadLock(mResourceDownloadTasksMutex);
               mResourceDownloadTasks[dl->getIdentifier() + " : " + localMeshName + " : " + aggObject->mUUID.toString()] = dl;
            }
    }

    //Start off the download tasks.
    for (uint32 i = 0; i < downloadTasks.size(); i++) {
      downloadTasks[i]->start();
    }
}

void MeshAggregateManager::replaceCityEngineTextures(MeshdataPtr m) {
      for(MaterialEffectInfoList::iterator mat_it = m->materials.begin();
          mat_it != m->materials.end(); mat_it++)
      {
          for(MaterialEffectInfo::TextureList::iterator tex_it = mat_it->textures.begin();
              tex_it != mat_it->textures.end(); tex_it++)
          {
            if (!tex_it->uri.empty() && tex_it->uri.find("road")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(0.296875, 0.296875, 0.296875, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("concrete")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(114/256.f,114/256.f,114/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("curb")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(146/256.f, 143/256.f, 136/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("dirtmap")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(239/256.f,239/256.f,239/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("flatroof")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(18/256.f,19/256.f,22/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("parkingLots")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(121/256.f,123/256.f,123/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("pavement")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(114/256.f,114/256.f,114/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("streets_texture_paintings")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(131/256.f,131/256.f,131/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("rooftile")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(9/256.f,8/256.f,6/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("yard_1_day")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(109/256.f,117/256.f,86/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("yard_6_day")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(84/256.f,99/256.f,69/256.f, 1);
            }
            if (!tex_it->uri.empty() && tex_it->uri.find("yard_9_day")!=String::npos) {
              tex_it->uri="";
              tex_it->color=Vector4f(117/256.f,121/256.f,85/256.f, 1);
            }
          }
      }

}

uint32 MeshAggregateManager::generateAggregateMeshAsync(const UUID uuid, Time postTime, bool generateSiblings) {
  Time curTime = Timer::now();

  if (mSkipGenerate) {
    mAggregatesGenerated++;
    return GEN_SUCCESS;
  }

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) {
    AGG_LOG(info, uuid.toString() <<" : not found in aggregate objects map" << "\n");

    /*Returning true here because this aggregate is no longer valid, so it should be
      removed from the list of aggregates whose meshes are pending. */
    return GEN_SUCCESS;
  }
  std::tr1::shared_ptr<AggregateObject> aggObject = mAggregateObjects[uuid];
  lock.unlock();

  /*Check if it makes sense to generate the aggregates now. Has the a
    aggregate been updated since the mesh generation command was posted?
    Does LOC contain info about this aggregate and its children?
    Are all the children's meshes available to generate the aggregate? */
  if (postTime < aggObject->mLastGenerateTime) {
    return NEWER_REQUEST;
  }
  if (postTime < mAggregateGenerationStartTime) {
    return NEWER_REQUEST;
  }

  std::tr1::unordered_map<UUID, std::tr1::shared_ptr<LocationInfo> , UUID::Hasher> currentLocMap;
  std::tr1::shared_ptr<LocationInfo> locInfoForUUID = getCachedLocInfo(uuid);
  /* Does LOC contain info about this aggregate and its children? */
  if (!locInfoForUUID) {
    return MISSING_LOC_INFO;
  }
  else {
    currentLocMap[uuid] = locInfoForUUID;
  }

  std::vector<AggregateObjectPtr> children = aggObject->getChildrenCopy();
                                                   //Set this to mLeaves if you want
                                                   //to generate directly from the leaves of the tree

  AGG_LOG(insane, aggObject->mTreeLevel << " : " << children.size() << " : treelvel : size");
  uint32 retval = checkLocAndMeshInfo(uuid, children, currentLocMap);
  if (retval != 0)
    return retval;

  /*Are the meshes of all the children available to generate the aggregate mesh? */
  retval = checkMeshesAvailable(uuid, curTime, children, currentLocMap);
  if (retval != 0)
    return retval;

  /* OK to generate the mesh! Go! */
  aggObject->mLastGenerateTime = curTime;
  MeshdataPtr agg_mesh =  MeshdataPtr( new Meshdata() );
  agg_mesh->globalTransform = Matrix4x4f::identity();
  agg_mesh->hasAnimations = false;

  Vector3f uuidPos = locInfoForUUID->currentPosition();
  float64 posX = uuidPos.x, posY = uuidPos.y, posZ = uuidPos.z;

  std::tr1::unordered_map<std::string, uint32> meshToStartIdxMapping;
  std::tr1::unordered_map<std::string, uint32> meshToStartMaterialsIdxMapping;
  std::tr1::unordered_map<std::string, uint32> meshToStartLightIdxMapping;
  std::tr1::unordered_map<std::string, uint32> meshToStartNodeIdxMapping;

  bool isLeafAggregate = true;
  //check if its a leaf aggregate
  for (uint32 i= 0; isLeafAggregate && i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    if (mAggregateObjects.find(child_uuid) != mAggregateObjects.end() &&
        mAggregateObjects[child_uuid]->childrenSize() != 0)
    {
	isLeafAggregate = false;
    }
  }

  std::tr1::unordered_map<String, String> textureToHashMap;
  if (isLeafAggregate)
  {
    retval = checkTextureHashesAvailable(children, textureToHashMap);
    if (retval != 0)
      return retval;
  }

  String* meshURIs = new String[children.size()];
  AGG_LOG(insane, children.size() << " : children.size()\n");
  for (uint32 i=0; i<children.size(); i++) {
    meshURIs[i] = currentLocMap[children[i]->mUUID]->mesh();
  }

  //do descriptor replacement.
  std::vector<Matrix4x4f> replacementAlignmentTransforms;
  deduplicateMeshes(children, isLeafAggregate, meshURIs, replacementAlignmentTransforms);

  {
    boost::mutex::scoped_lock textureToHashMapLock(mTextureNameToHashMapMutex);

    for (std::tr1::unordered_map<String,String>::iterator it = textureToHashMap.begin();
         it != textureToHashMap.end(); it++)
    {
       //FIXME: This should get erased here, but elements added to the hashmap should
       //have key = objectUUID+texturename instead of just texturename
       //mTextureNameToHashMap.erase(it->first);
    }
  }

  // And finally, when we do, perform the merge

  // Tracks textures so we can fill in agg_mesh->textures when we're
  // done copying data in. Also tracks mapping of texture filename ->
  // original texture URL so we can tell the CDN to reuse that data.
  std::tr1::unordered_map<String, String> textureSet;

  AGG_LOG(insane,  "FINALLY WE ARE READY! " << uuid << "\n");
  AGG_LOG(insane, mTextureNameToHashMap.size() << " : mTextureNameToHashMap.size()\n");
  AGG_LOG(insane, mMeshStore.size() << " : mMeshStore.size()\n");

  std::tr1::unordered_map<String, MeshdataPtr> textureToModelMap;
  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      continue;
    }
    lock.unlock();

    std::string meshName = meshURIs[i];
    MeshdataPtr m = MeshdataPtr();
    {
      boost::mutex::scoped_lock lock(mMeshStoreMutex);
      if (mMeshStore.find(meshName) != mMeshStore.end())
        m = mMeshStore[meshName];
    }
    if (!m || meshName == "") continue;

    //Compute the bounds for the child's mesh.
    BoundingBox3f3f originalMeshBoundingBox = BoundingBox3f3f::null();
    double originalMeshBoundsRadius=0;
    ComputeBounds( m, &originalMeshBoundingBox, &originalMeshBoundsRadius);

    // We me reuse more than one of the same mesh, e.g. the aggregate may have
    // two identical trees that are at different locations. In that case,
    // SubMeshGeometry, Textures, LightInfos, MaterialEffectInfos, and Nodes can
    // all be shared, and can therefore reuse offsets.
    // If necessary, add the offset in.
    if ( meshToStartIdxMapping.find(meshName) == meshToStartIdxMapping.end()) {
      meshToStartIdxMapping[ meshName ] = agg_mesh->geometry.size();
      meshToStartMaterialsIdxMapping[ meshName ] = agg_mesh->materials.size();
      meshToStartNodeIdxMapping[ meshName ] = agg_mesh->nodes.size();
      meshToStartLightIdxMapping[ meshName ] = agg_mesh->lights.size();

      // Copy SubMeshGeometries. We loop through so we can reset the numInstances
      for(uint32 smgi = 0; smgi < m->geometry.size(); smgi++) {
          SubMeshGeometry smg = m->geometry[smgi];

          agg_mesh->geometry.push_back(smg);
      }

      //HACK: Replace texture with color in the CityEngine scene
      replaceCityEngineTextures(m);
      Prox::DescriptorReader* descriptorReader = Prox::DescriptorReader::getDescriptorReader();

      // Replace texture with color.
      for(MaterialEffectInfoList::iterator mat_it = m->materials.begin();
          false && mat_it != m->materials.end(); mat_it++)
      {
          for(MaterialEffectInfo::TextureList::iterator tex_it = mat_it->textures.begin();
              tex_it != mat_it->textures.end(); tex_it++)
          {
		String daename = meshName.substr(meshName.find_last_of('/')+1);
                if (tex_it->uri == "") continue;
		AGG_LOG(insane,  tex_it->uri << " : tex_it->uri\n");
		String texname = tex_it->uri.substr(0,tex_it->uri.find_last_of('/'));
		texname = texname.substr(texname.find_last_of('/')+1);
		Prox::ZernikeDescriptor& zd = descriptorReader->getDominantColorDescriptor(daename+":"+texname);
		if (zd.size() == 3) {
		  AGG_LOG(insane, "Replacing texture with color\n");
                  tex_it->color=Vector4f(  zd.get(0)/256.0, zd.get(1)/256.0, zd.get(2)/256.0, 1);
		  tex_it->uri="";
		  AGG_LOG(insane,tex_it->color << "\n");
		}
          }
      }


      // Copy Materials
      agg_mesh->materials.insert(agg_mesh->materials.end(),
          m->materials.begin(),
          m->materials.end());
      // Copy names of textures from the materials into a set so we can fill in
      // the texture list when we finish adding all subobjects
      for(MaterialEffectInfoList::const_iterator mat_it = m->materials.begin(); mat_it != m->materials.end(); mat_it++) {
          for(MaterialEffectInfo::TextureList::const_iterator tex_it = mat_it->textures.begin(); tex_it != mat_it->textures.end(); tex_it++) {
              if (!tex_it->uri.empty()) {
                  Transfer::URI orig_tex_uri;

                  // We need to handle relative and absolute URIs:

                  // If we can decode a URL, then we may need to handle relative URLs.
                  Transfer::URL parent_url(meshName);
                  if (!parent_url.empty()) {
                      // The URL constructor will figure out absolute/relative, or just fail
                      // if it can't construct a valid URL.
                      Transfer::URL deriv_url(parent_url.context(), tex_it->uri);
                      if (!deriv_url.empty())
                          orig_tex_uri = Transfer::URI(deriv_url.toString());
                  }

                  // Otherwise, try to decode as a plain URI, ignoring the original URI. This
                  // will either succeed with a full URI or fail and return an
                  // empty URI
                  if (orig_tex_uri.empty())
                      orig_tex_uri = Transfer::URI(tex_it->uri);

                  textureSet[tex_it->uri] = orig_tex_uri.toString();
		  textureToModelMap[tex_it->uri] = m;
              }
	  }
      }


      // Copy Lights
      agg_mesh->lights.insert(agg_mesh->lights.end(),
          m->lights.begin(),
          m->lights.end());
    }

    // And always extract into convenience variables
    uint32 submeshGeomOffset = meshToStartIdxMapping[meshName];
    uint32 submeshMaterialsOffset = meshToStartMaterialsIdxMapping[meshName];
    uint32 submeshLightOffset = meshToStartLightIdxMapping[meshName];
    uint32 submeshNodeOffset = meshToStartNodeIdxMapping[meshName];

    // Extract the loc information we need for this object.
    Vector3f location = currentLocMap[child_uuid]->currentPosition();
    double scalingfactor = 1.0;

    //If the child is an aggregate, don't use the information from LOC blindly.
    //Fix that info up so that it corresponds with the actual position and size
    //of the aggregate mesh.
    if (isAggregate(child_uuid)) {
      Vector4f offsetFromCenter = m->globalTransform.getCol(3);
      offsetFromCenter = offsetFromCenter * -1.f;

      location.x += offsetFromCenter.x;
      location.y += offsetFromCenter.y;
      location.z += offsetFromCenter.z;

      //Scaling factor is set to 1 because an aggregate mesh is generated so that
      //its size is exactly whats required to be displayed.
      scalingfactor = 1.0;
    }
    else {
      scalingfactor = (currentLocMap[child_uuid]->bounds().fullBounds()).radius() / originalMeshBoundsRadius;
    }


    float64 locationX = location.x;
    float64 locationY = location.y;
    float64 locationZ = location.z;
    Quaternion orientation = currentLocMap[child_uuid]->currentOrientation();

    // Reuse geoinst_it and geoinst_idx from earlier, but with a new iterator.
    Meshdata::GeometryInstanceIterator geoinst_it = m->getGeometryInstanceIterator();
    Matrix4x4f orig_geo_inst_xform;
    uint32 geoinst_idx;

    while( geoinst_it.next(&geoinst_idx, &orig_geo_inst_xform) ) {
      // Copy the instance data.
      GeometryInstance geomInstance = m->instances[geoinst_idx];

      // Sanity check
      assert (geomInstance.geometryIndex < m->geometry.size());

      // Shift indices for
      //  Materials
      for(GeometryInstance::MaterialBindingMap::iterator mbit = geomInstance.materialBindingMap.begin();
          mbit != geomInstance.materialBindingMap.end(); mbit++)
      {
          mbit->second += submeshMaterialsOffset;
      }
      //  Geometry
      geomInstance.geometryIndex += submeshGeomOffset;
      //  Parent node
      //  FIXME see note below to understand why this ultimately has no
      //  effect. parentNode ends up getting overwritten with a new parent nodes
      //  index that flattens the node hierarchy.
      geomInstance.parentNode += submeshNodeOffset;

      //translation
      Matrix4x4f trs = Matrix4x4f( Vector4f(1,0,0, (locationX - posX)),
                                   Vector4f(0,1,0, (locationY - posY)),
                                   Vector4f(0,0,1, (locationZ - posZ)),
                                   Vector4f(0,0,0,1), Matrix4x4f::ROWS());

      //rotate
      float ox = orientation.normal().x;
      float oy = orientation.normal().y;
      float oz = orientation.normal().z;
      float ow = orientation.normal().w;

      Matrix4x4f rotateMatrix = Matrix4x4f( Vector4f(1-2*oy*oy - 2*oz*oz , 2*ox*oy - 2*ow*oz, 2*ox*oz + 2*ow*oy, 0),
                         Vector4f(2*ox*oy + 2*ow*oz, 1-2*ox*ox-2*oz*oz, 2*oy*oz-2*ow*ox, 0),
                         Vector4f(2*ox*oz-2*ow*oy, 2*oy*oz + 2*ow*ox, 1-2*ox*ox - 2*oy*oy,0),
                         Vector4f(0,0,0,1),                 Matrix4x4f::ROWS());

      trs *= rotateMatrix;

      //scaling
      trs *= Matrix4x4f::scale(scalingfactor);

      // Generate a node for this instance
      NodeIndex geom_node_idx = agg_mesh->nodes.size();
      // FIXME because we need to have trs * original_transform (i.e., left
      // instead of right multiply), the trs transform has to be at the
      // root. This means we can't trivially handle this with additional nodes
      // since the new node would have to be inserted at the root (and therefore
      // some may end up conflicting). For now, we just flatten these by
      // creating a new root node.

      agg_mesh->nodes.push_back( Node(trs * replacementAlignmentTransforms[i] * orig_geo_inst_xform) );

      agg_mesh->rootNodes.push_back(geom_node_idx);
      // Overwrite the parent node to make this new one with the correct
      // transform the one we use.
      geomInstance.parentNode = geom_node_idx;

      // Increase ref count on instanced geometry
      SubMeshGeometry& smgRef = agg_mesh->geometry[geomInstance.geometryIndex];

      //Push the instance into the Meshdata data structure
      agg_mesh->instances.push_back(geomInstance);
    }

    for (uint32 j = 0; j < m->lightInstances.size(); j++) {
      LightInstance& lightInstance = m->lightInstances[j];
      lightInstance.lightIndex += submeshLightOffset;
      agg_mesh->lightInstances.push_back(lightInstance);
    }
  }

  AGG_LOG(insane,  "Dealing with textures: " << uuid << "\n");

  bool doAtlasing=false;
  std::tr1::unordered_map<String, String> texFileNameToUrl;
  //HACKY way to deduplicate textures based on their filenames only.
  //This should really be based on the hashes of the
  //textures' content.
  for (std::tr1::unordered_map<String, String>::iterator it = textureSet.begin(); it != textureSet.end(); it++) {
    String url = it->first;

    size_t indexOfLastSlash = url.find_last_of('/');
    if (indexOfLastSlash == url.npos) {
	texFileNameToUrl[url] = url;
	continue;
    }

    String substrUrl = url.substr(0, indexOfLastSlash);

    indexOfLastSlash = substrUrl.find_last_of('/');
    if (indexOfLastSlash == substrUrl.npos) {
        texFileNameToUrl[url] = url;
        continue;
    }

    String texfilename = substrUrl.substr(indexOfLastSlash+1);
    if (url.find("tahirazim/apiupload/test_project_3") != url.npos ) {
      texFileNameToUrl[texfilename] = url;
    }
    else {
      // Only enable atlasing if at least one of the textures is
      // not a CityEngine texture. HACKY, but it works for now...
      texFileNameToUrl[url]=url;
      doAtlasing= true;
    }
  }


  // We should have all the textures in our textureSet since we looped through
  // all the materials, just fill in the list now.
  for (std::tr1::unordered_map<String, String>::iterator it = texFileNameToUrl.begin(); it != texFileNameToUrl.end(); it++)
  {
      agg_mesh->textures.push_back( it->second );
  }

  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    // May have left because of the tree getting shifted around, so we don't
    // want to assert that the aggregate object still has an entry. // FIXME
    // even resetting this here isn't currently really a good idea since we can
    // have multiple threads generating the same object at the same time due to
    // multiple requests going to different threads...
    if (mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) continue;
    mAggregateObjects[child_uuid]->mMeshdata = std::tr1::shared_ptr<Meshdata>();
  }

  //Find average number of vertices in the mesh;s geometries. If it's small, just squash!
  float32 averageVertices = 0;
  for (uint32 i=0; i<agg_mesh->geometry.size(); i++) {
    averageVertices += agg_mesh->geometry[i].positions.size();
  }
  averageVertices /= agg_mesh->geometry.size();

  AGG_LOG(warn, averageVertices << " : averageVertices\n");
  //This block squashes geometry to get baseline results without any optimizations.
  if (averageVertices <= 75) {
    boost::mutex::scoped_lock squashLock(mSquashFilterMutex);
    Mesh::MutableFilterDataPtr input_data(new Mesh::FilterData);
    input_data->push_back(agg_mesh);
    Mesh::FilterDataPtr output_data = mSquashFilter->apply(input_data);
    agg_mesh = std::tr1::dynamic_pointer_cast<Mesh::Meshdata> (output_data->get());
  }
  //Simplify the mesh...
  mMeshSimplifier.simplify(agg_mesh, 20000);

  //Set the mesh of this aggregate to the empty string until the new version gets uploaded. This is so that
  //higher level aggregates are not generated from the now out-of-date version of the mesh.
  mLoc->context()->mainStrand->post(
        std::tr1::bind(
            &MeshAggregateManager::updateAggregateLocMesh, this,
            uuid, ""
        ),
        "MeshAggregateManager::updateAggregateLocMesh"
  );


  //... and now create the collada file, upload to the CDN and update LOC.

  String localMeshName = boost::lexical_cast<String>(aggObject->mTreeLevel) +
                         "_aggregate_mesh_" +
                         uuid.toString() + ".dae";

  //uncomment to test performance with no atlasing!
  //doAtlasing = false;  //hack commented!

  if (agg_mesh->textures.size() > 0 && mAtlasingNeeded && doAtlasing) {
    startDownloadsForAtlasing(uuid, agg_mesh, aggObject, localMeshName, textureSet, textureToModelMap);
  }
  else {
     //If the mesh has no texture, upload to the CDN and update LOC.
     mUploadStrands[rand() % mNumUploadThreads]->post(
          std::tr1::bind(&MeshAggregateManager::uploadAggregateMesh, this, agg_mesh, "",
			 std::tr1::shared_ptr<char>(), 0, aggObject, textureSet, 0, Time::null()),
          "MeshAggregateManager::uploadAggregateMesh"
      );
  }

  AGG_LOG(info, "Generated aggregate: " << localMeshName << "\n");
  Duration aggregation_duration = (Timer::now() - curTime);
  AGG_LOG(warn, "Time to generate: " << aggregation_duration.toMilliseconds() );

  mAggregatesGenerated++;
  {
    boost::mutex::scoped_lock modelSystemLock(mStatsMutex);
    mAggregateCumulativeGenerationTime += aggregation_duration;
  }
  delete [] meshURIs;

  return GEN_SUCCESS;
}

//FIXME: This should really be replaced by a functor.
void MeshAggregateManager::textureChunkFinished(String texname, Transfer::Fingerprint hashprint, uint32 offset, uint32 length, MeshdataPtr agg_mesh, AggregateObjectPtr aggObj,
                                          std::tr1::unordered_map<String, String> textureSet,
                                          std::tr1::shared_ptr<std::tr1::unordered_map<String, int> > downloadedTexturesMap,
                                          std::tr1::shared_ptr<std::tr1::unordered_map<String, std::vector<String> > > hashToURIMap,
                                          int retryAttempt, ResourceDownloadTaskPtr dlPtr,
					  std::tr1::shared_ptr<Transfer::TransferRequest> request,
                                          std::tr1::shared_ptr<const Transfer::DenseData> response)
{
  String hash = hashprint.toString();

  if (!response) {
    AGG_LOG(insane,  texname << " " << retryAttempt << " : failed download\n");

    if (retryAttempt <= 15) {
      String localMeshName = boost::lexical_cast<String>(aggObj->mTreeLevel) +
                         "_aggregate_mesh_" +
                         aggObj->mUUID.toString() + ".dae";

      //its a mipmap
      if ( hashprint != Transfer::Fingerprint::null() ) {
        ResourceDownloadTaskPtr dl = ResourceDownloadTask::construct(
                Transfer::Chunk(hashprint, Transfer::Range(offset, length, Transfer::LENGTH)), mTransferPool,
                1.0,
                std::tr1::bind(&MeshAggregateManager::textureChunkFinished, this, texname, hashprint, offset, length, agg_mesh, aggObj, textureSet, downloadedTexturesMap, hashToURIMap, retryAttempt+1, _1, _2, _3)
                );

        mAggregationStrands[0]->post(Duration::seconds(pow(2,retryAttempt)), std::tr1::bind(&ResourceDownloadTask::start, dl.get()));

        boost::mutex::scoped_lock resourceDownloadLock(mResourceDownloadTasksMutex);
        mResourceDownloadTasks[dl->getIdentifier() + " : " + localMeshName + " : " + aggObj->mUUID.toString()] = dl;
      }
      else  {
        //its a full texture to download
        ResourceDownloadTaskPtr dl = ResourceDownloadTask::construct(
                  Transfer::URI(texname), mTransferPool,
                  1.0,
                  std::tr1::bind(&MeshAggregateManager::textureChunkFinished, this, texname, hashprint, offset, length, agg_mesh, aggObj, textureSet, downloadedTexturesMap, hashToURIMap, retryAttempt+1,  _1, _2, _3)
               );
        mAggregationStrands[0]->post(Duration::seconds(pow(2,retryAttempt)), std::tr1::bind(&ResourceDownloadTask::start, dl.get()));

        boost::mutex::scoped_lock resourceDownloadLock(mResourceDownloadTasksMutex);
        mResourceDownloadTasks[dl->getIdentifier() + " : " + localMeshName + " : " + aggObj->mUUID.toString()] = dl;
      }
    }

    return;
  }

  AGG_LOG(insane, hash << " , "  << texname << " : " << request->getIdentifier() << " : request->getIdentifier");

  String reqIdWithoutOffset = request->getIdentifier();
  reqIdWithoutOffset = reqIdWithoutOffset.substr(0, reqIdWithoutOffset.find_first_of('_'));
  //doing this so that the hashToURIMap is OK to pass to the texture atlas filter for metadata.
  if (hashToURIMap->find(reqIdWithoutOffset) == hashToURIMap->end() ) {
    (*hashToURIMap)[reqIdWithoutOffset].push_back(texname);
  }


  if (hashToURIMap->find(hash) != hashToURIMap->end()) {
    std::vector<String>& textures = (*hashToURIMap)[hash];

    if (textures.size() > 0) {
      for (uint32 i = 0; i < textures.size(); i++) {
        textureChunkFinished(textures[i], Transfer::Fingerprint::null(), offset, length, agg_mesh, aggObj, textureSet, downloadedTexturesMap, hashToURIMap, retryAttempt, dlPtr, request, response);
      }

      return;
    }
  }

  String uuid = aggObj->mUUID.toString();
  String localMeshName = boost::lexical_cast<String>(aggObj->mTreeLevel) +
                         "_aggregate_mesh_" +
                        uuid + ".dae";
  boost::mutex::scoped_lock atlasLock(aggObj->mAtlasAndUploadMutex);

  (*downloadedTexturesMap)[texname] = 1;
  String texPrefix = texname;

  for (std::tr1::unordered_map<String,int>::iterator it = downloadedTexturesMap->begin(); it !=downloadedTexturesMap->end(); it++) {
    AGG_LOG(insane, uuid << " : " << it->first << " : " << it->second << " -- Download status\n");
  }

  if (response) {
    //Set the bit to high
    AGG_LOG(insane,  "Downloaded texture " << texname << "\n");

    std::replace(texPrefix.begin(), texPrefix.end(), '/', '-');

    {
      String url = texname;

      size_t indexOfLastSlash = url.find_last_of('/');
      if (indexOfLastSlash == url.npos) {
        assert(false);
      }

      String substrUrl = url.substr(0, indexOfLastSlash);

      indexOfLastSlash = substrUrl.find_last_of('/');
      if (indexOfLastSlash == substrUrl.npos) {
          assert(false);
      }

      String texfilename = substrUrl.substr(indexOfLastSlash+1);
      texname = texfilename;
    }

    //store the texture
    AGG_LOG(insane,  ("/tmp/sirikata/"+uuid+".dir/" + texPrefix + texname) << " : saving file here\n");
    std::ofstream of (("/tmp/sirikata/"+uuid+".dir/" + texPrefix + texname).c_str(), std::ios::binary | std::ios::out);
    of.write( (const char*) response->data(), response->length());
    of.close();
  }

    //check if all textures have been downloaded
    bool allDownloaded = true;
    for (std::tr1::unordered_map<String, int>::iterator it = downloadedTexturesMap->begin();
         it != downloadedTexturesMap->end(); it++
 	)
    {
      if (it->second == 0) {
        allDownloaded = false;
        break;
      }
    }

    //if everthing was downloaded, atlas!
    if (allDownloaded) {
      //but first... change all the texture paths to relative paths!
      for (uint32 i = 0; i <  agg_mesh->textures.size(); i++) {
        String url = agg_mesh->textures[i];
        texPrefix = url;
        std::replace(texPrefix.begin(), texPrefix.end(), '/', '-');

        size_t indexOfLastSlash = url.find_last_of('/');
        if (indexOfLastSlash == url.npos) {
          continue;
        }

        String substrUrl = url.substr(0, indexOfLastSlash);

        indexOfLastSlash = substrUrl.find_last_of('/');
        if (indexOfLastSlash == substrUrl.npos) {
          continue;
        }

        String texfilename = substrUrl.substr(indexOfLastSlash+1);
        agg_mesh->textures[i] = "./" + texPrefix + texfilename;
      }

      for(MaterialEffectInfoList::iterator mat_it = agg_mesh->materials.begin(); mat_it != agg_mesh->materials.end(); mat_it++) {
        for(MaterialEffectInfo::TextureList::iterator tex_it = mat_it->textures.begin(); tex_it != mat_it->textures.end(); tex_it++) {
          if (!tex_it->uri.empty()) {
            String url = tex_it->uri;
            texPrefix = url;
            std::replace(texPrefix.begin(), texPrefix.end(), '/', '-');

            size_t indexOfLastSlash = url.find_last_of('/');
            if (indexOfLastSlash == url.npos) {
              continue;
            }

            String substrUrl = url.substr(0, indexOfLastSlash);

            indexOfLastSlash = substrUrl.find_last_of('/');
            if (indexOfLastSlash == substrUrl.npos) {
              continue;
            }

            String texfilename = substrUrl.substr(indexOfLastSlash+1);
            tex_it->uri = "./" + texPrefix  + texfilename;
          }
        }
      }

      AGG_LOG(insane,  "Atlasing " << uuid << " ...\n");
      // ok, now do the atlasing.
      std::vector<String> names_and_args;
      names_and_args.push_back("texture-atlas");
      names_and_args.push_back(""/*serializeHashToURIMap(hashToURIMap)*/);
      std::tr1::shared_ptr<Mesh::CompositeFilter> atlasingFilter =
	        std::tr1::shared_ptr<Mesh::CompositeFilter>(new Mesh::CompositeFilter(names_and_args));
      Mesh::MutableFilterDataPtr input_data(new Mesh::FilterData);
      agg_mesh->uri = "file:///tmp/sirikata/"+ uuid +".dir/"+uuid+".dae";
      input_data->push_back(agg_mesh);
      Mesh::FilterDataPtr output_data = atlasingFilter->apply(input_data);
      MeshdataPtr m = std::tr1::dynamic_pointer_cast<Mesh::Meshdata> (output_data->get());

      //read back the atlas into a buffer
      String uri_dir = agg_mesh->uri.substr(7);
      String uri_sans_protocol = uri_dir;
      size_t slash_pos = uri_dir.rfind("/");

      String file_name="";
      if (slash_pos == std::string::npos || slash_pos == uri_dir.size()-1) {
        uri_dir = "./";
        file_name = uri_sans_protocol;
      }
      else {
        uri_dir = uri_dir.substr(0, slash_pos+1);
        file_name = uri_sans_protocol.substr(slash_pos+1);
      }
      std::ifstream atlas( (uri_dir+file_name+".atlas.png").c_str());
      atlas.seekg(0,std::ios::end);
      uint32 length = atlas.tellg();
      atlas.seekg(0,std::ios::beg);
      std::tr1::shared_ptr<char> buffer = std::tr1::shared_ptr<char>(new char[length]);
      atlas.read(buffer.get(), length);
      atlas.close();

      //make the texture path relative instead of an absolute filename.
      for (uint32 i = 0; i <  agg_mesh->textures.size(); i++) {
        String url = agg_mesh->textures[i];
        agg_mesh->textures[i] = "./" + url.substr(url.find_last_of('/')+1);
      }
      for(MaterialEffectInfoList::iterator mat_it = agg_mesh->materials.begin(); mat_it != agg_mesh->materials.end(); mat_it++) {
        for(MaterialEffectInfo::TextureList::iterator tex_it = mat_it->textures.begin(); tex_it != mat_it->textures.end(); tex_it++) {
          if (!tex_it->uri.empty()) {
            String url = tex_it->uri;
            tex_it->uri = "./" + url.substr(url.find_last_of('/')+1);
          }
        }
      }

      {
        boost::mutex::scoped_lock modelSystemLock(mModelsSystemMutex);
        //finally launch the upload.
        bool converted = mModelsSystem->convertVisual( m, "colladamodels",
						     "/tmp/sirikata/"+uuid+".dir/"+uuid+".dae");
      }

      mUploadStrands[rand() % mNumUploadThreads]->post(
          std::tr1::bind(&MeshAggregateManager::uploadAggregateMesh, this, m, file_name+".atlas.png", buffer, length, aggObj, textureSet, 0, Time::null()),
          "MeshAggregateManager::uploadAggregateMesh"
      );
    }

  boost::mutex::scoped_lock resourceDownloadLock(mResourceDownloadTasksMutex);
  uint32 numErasedElements = mResourceDownloadTasks.erase(dlPtr->getIdentifier()+" : " + localMeshName + " : " + aggObj->mUUID.toString());
  AGG_LOG(insane,  "Erased elements: " << (dlPtr->getIdentifier()+" : " + localMeshName + " : " + aggObj->mUUID.toString())   <<  " : "  << numErasedElements << "\n");
}

void MeshAggregateManager::textureDownloadedForCounting(String texname, uint32 retryAttempt,
                                          std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                          std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response)
{
  if (!response && retryAttempt < 10) {
    Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(texname), 1.0, std::tr1::bind(
                                       &MeshAggregateManager::textureDownloadedForCounting, this, texname,
                                       retryAttempt+1, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);

    return;
  }

  if (!response) {
    boost::mutex::scoped_lock mTextureNameToHashMapLock(mTextureNameToHashMapMutex);
    mTextureNameToHashMap[texname] = Transfer::Fingerprint::null().toString();
    AGG_LOG(error, "USING EMPTY HASH FOR TEXTURE NAME : " << texname << "\n");
  }

  if (response) {
    String hash = response->getFingerprint().toString();

    if (mSeenTextureHashes.find(hash) == mSeenTextureHashes.end()) {

      mSeenTextureHashes.insert(hash);
      mSizeOfSeenTextures += response->getSize();

      AGG_LOG(insane, mSizeOfSeenTextures << " : mSizeOfSeenTextures\n");
      AGG_LOG(insane, mSeenTextureHashes.size() << " : mSeenTextureHashes.size()\n");
      if (!mAtlasingNeeded && (mSeenTextureHashes.size() > 128 || mSizeOfSeenTextures > 64*1048576)) {
        mAtlasingNeeded = true;
        AGG_LOG(insane, "ATLASING NEEDED\n");
      }
    }
    boost::mutex::scoped_lock mTextureNameToHashMapLock(mTextureNameToHashMapMutex);
    mTextureNameToHashMap[texname] = hash;
  }

}

String MeshAggregateManager::serializeHashToURIMap(std::tr1::shared_ptr<std::tr1::unordered_map<String, std::vector<String> > > hashToURIMap)
{
  String str = "";
  for (std::tr1::unordered_map<String, std::vector<String> >::iterator it = hashToURIMap->begin();
       it != hashToURIMap->end(); it++
      )
  {
    String hash = it->first;
    std::vector<String>& uris = it->second;

    for (uint32 i = 0; i < uris.size(); i++) {
      str += uris[i] + "," + hash + "\n";
    }
  }

  return str;
}


void MeshAggregateManager::uploadAggregateMesh(Mesh::MeshdataPtr agg_mesh,
					   String atlas_name,
					   std::tr1::shared_ptr<char> atlas_buffer,
					   uint32 atlas_length,
                                           AggregateObjectPtr aggObject,
                                           std::tr1::unordered_map<String, String> textureSet,
                                           uint32 retryAttempt, Time uploadStartTime)
{
  const UUID& uuid = aggObject->mUUID;

  String localMeshName = boost::lexical_cast<String>(aggObject->mTreeLevel) +
                         "_aggregate_mesh_" +
                         uuid.toString() + ".dae";
  String cdnMeshName = "";

  AGG_LOG(insane, "Trying  to upload : " << localMeshName);

  if (uploadStartTime == Time::null())
    uploadStartTime = Timer::now();

  //FIXME: This block needs to be removed in a production version.
  if (!mOAuth || mCDNUsername.empty()) {
    std::string path = "file:///tmp/sirikata/uploads/";
    if (!mLocalURLPrefix.empty()) {
      path = mLocalURLPrefix;
    }
    for (uint32 i = 0; i < agg_mesh->textures.size() && atlas_buffer; i++) {
      agg_mesh->textures[i] =   mLocalURLPrefix + agg_mesh->textures[i];
      AGG_LOG(insane,  "Changing urls to absolute in upload\n");
    }
    for(MaterialEffectInfoList::iterator mat_it = agg_mesh->materials.begin(); mat_it != agg_mesh->materials.end() && atlas_buffer; mat_it++) {
         for(MaterialEffectInfo::TextureList::iterator tex_it = mat_it->textures.begin(); tex_it != mat_it->textures.end(); tex_it++) {
              if (!tex_it->uri.empty()) {
 		tex_it->uri = mLocalURLPrefix + tex_it->uri;
              }
         }
    }
  }

  std::string serialized = "";
  {
    boost::mutex::scoped_lock modelSystemLock(mModelsSystemMutex);
    std::stringstream model_ostream(std::ofstream::out | std::ofstream::binary);

    bool converted = mModelsSystem->convertVisual( agg_mesh, "colladamodels", model_ostream);

    serialized = model_ostream.str();

    mAggregateCumulativeDataSize += serialized.size();
    aggObject->mSerializedSize = atlas_length + serialized.size();

    /* Debugging code: store the file locally for ease of inspection.
     String modelFilename = std::string("/tmp/") + localMeshName;
     std::ofstream model_ostream2(modelFilename.c_str(), std::ofstream::out | std::ofstream::binary);
     converted = mModelsSystem->convertVisual(agg_mesh, "colladamodels", model_ostream2);
     model_ostream2.close();
    */

  }

  // "Upload" that doesn't really do anything, useful for testing, required to
  // do automated tests that don't require a real account on the CDN
  if (mSkipUpload) {
    // This is bogus, but we need to fill in some URI
      cdnMeshName = "http://localhost/aggregate_meshes/" + localMeshName;
      agg_mesh->uri = cdnMeshName;

      //Update loc
      mLoc->context()->mainStrand->post(
        std::tr1::bind(
            &MeshAggregateManager::updateAggregateLocMesh, this,
            uuid, cdnMeshName
        ),
        "MeshAggregateManager::updateAggregateLocMesh"
      );

      mAggregatesUploaded++;
      AGG_LOG(info, "Uploaded successfully: " << localMeshName << "\n");

      addToInMemoryCache(cdnMeshName, agg_mesh);

      //aggObject->mLeaves.clear();

      Duration upload_time = Timer::now() - uploadStartTime;
      {
        boost::mutex::scoped_lock modelSystemLock(mStatsMutex);
        mAggregateCumulativeUploadTime += upload_time;
      }

      return;
  }

  // We have two paths here, the real CDN upload and the old, local approach
  // where we dump the file and run a script to "upload" it, which may just mean
  // moving it somewhere locally
  if (mOAuth && !mCDNUsername.empty()) {

      Transfer::UploadRequest::StringMap files;
      files[localMeshName] = std::string();
      // Swap to avoid a full copy, serialized won't be valid anymore
      // but we did everything else we needed to with it above
      files[localMeshName].swap(serialized);
      if (atlas_buffer)
        files[atlas_name] = String(atlas_buffer.get(), atlas_length);

      String upload_path = "aggregates/" + localMeshName;
      Transfer::UploadRequest::StringMap params;
      params["username"] = mCDNUsername;
      params["title"] = String("Aggregate Mesh ") + uuid.toString();
      params["main_filename"] = localMeshName;
      params["ephemeral"] = "1";
      params["ttl_time"] = boost::lexical_cast<String>(mModelTTL.seconds());
      // Translate subfile map to JSON
      namespace json = json_spirit;
      json::Value subfiles = json::Object();
      for (std::tr1::unordered_map<String, String>::iterator it = textureSet.begin(); it != textureSet.end() && !atlas_buffer; it++) {

          String subfile_name = it->first;
          {
              // The upload expects only the basename, i.e. the actual
              // filename. So if we referred to a URL
              // (meerkat://foo/bar/texture.png) or filename with
              // directories (/foo/bar/texture.png) we'd need to
              // extract just the filename.

              // Use URL to strip of any URL parts to get a normal looking path
              Transfer::URL subfile_name_as_url(subfile_name);
              if (!subfile_name_as_url.empty())
                  subfile_name = subfile_name_as_url.fullpath();

              // Then extract just the last element, i.e. the filename
              std::size_t filename_pos = subfile_name.rfind("/");
              if (filename_pos != String::npos)
                  subfile_name = subfile_name.substr(filename_pos+1);
          }

          String tex_path = Transfer::URL(it->second).fullpath();

          // Explicitly override the separator to ensure we don't
          // use parts of the filename to generate a hierarchy,
          // i.e. so that foo.png remains foo.png instead of
          // becoming foo { png : value }.
          subfiles.put(subfile_name, tex_path, '\0');
      }
      params["subfiles"] = json::write(subfiles);

      std::tr1::shared_ptr<Transfer::DenseData> uploaded_mesh(new Transfer::DenseData(files[localMeshName]));
      VisualPtr v;
      {
        boost::mutex::scoped_lock modelSystemLock(mModelsSystemMutex);

        v = mModelsSystem->load(uploaded_mesh);
      }
      // FIXME handle non-Meshdata formats
      MeshdataPtr m = std::tr1::dynamic_pointer_cast<Meshdata>(v);

      AGG_LOG(insane, localMeshName << " : upload started");
      Transfer::TransferRequestPtr req(
          new Transfer::UploadRequest(
              mOAuth,
              files, upload_path, params, 1.0f,
              std::tr1::bind(
                  &MeshAggregateManager::handleUploadFinished, this,
                  std::tr1::placeholders::_1, std::tr1::placeholders::_2,
		  m, atlas_name, atlas_buffer, atlas_length, aggObject, textureSet, retryAttempt,
                  uploadStartTime
              )
          )
      );
      mTransferPool->addRequest(req);

      //the remainder of the upload process is handled in handleUploadFinished.
  }
  else {
      String modelFilename;
      if (!mLocalPath.empty()) {
        modelFilename = (boost::filesystem::path(mLocalPath) / localMeshName).string();
        if (!mLocalURLPrefix.empty())
          cdnMeshName = mLocalURLPrefix + localMeshName;
        else
          cdnMeshName = "file://" + modelFilename;
      }
      else {
        modelFilename = "/disk/local/tazim/aggregate_meshes/" + localMeshName;
        cdnMeshName = "http://sns31.cs.princeton.edu:9080/" + localMeshName;
      }
      agg_mesh->uri = cdnMeshName;
      std::ofstream model_fs_stream(modelFilename.c_str(), std::ofstream::out | std::ofstream::binary);
      if (model_fs_stream) {
        model_fs_stream.write(&serialized[0], serialized.size());
        model_fs_stream.close();
      }
      else {
        mAggregatesFailedToUpload++;
        AGG_LOG(error, "Failed to save aggregate mesh " << localMeshName << ", it won't be displayed.");
        // Here the return value isn't success, it's "should I remove this
        // aggregate object from the queue for processing." Failure to save is
        // effectively fatal for the aggregate, so tell it to get removed.
        return;
      }

      if (atlas_buffer) {
        String atlasFilename = (boost::filesystem::path(modelFilename).branch_path() / atlas_name).string();
        std::ofstream atlas_fs_stream(atlasFilename.c_str(), std::ofstream::out | std::ofstream::binary);
        if (atlas_fs_stream) {
          atlas_fs_stream.write(atlas_buffer.get(), atlas_length);
          atlas_fs_stream.close();
        }
        else {
          mAggregatesFailedToUpload++;
          AGG_LOG(error, "Failed to save aggregate mesh atlas " << localMeshName << " : "
	                 << atlas_name << ", it won't be displayed.");
          return;
        }
      }

      //Upload to web server ("upload" for local doesn't require doing anything
      if (mLocalPath.empty()) {
        std::string cmdline = std::string("./upload_to_cdn.sh ") +  modelFilename;
        //FIXME: following line shouldnt be commented
        //system( cmdline.c_str()  );
      }

      AGG_LOG(info, "2. Uploaded successfully: " << localMeshName << "\n");

      //Update loc
      mLoc->context()->mainStrand->post(
        std::tr1::bind(
            &MeshAggregateManager::updateAggregateLocMesh, this,
            uuid, cdnMeshName
        ),
        "MeshAggregateManager::updateAggregateLocMesh"
      );

      mAggregatesUploaded++;


      std::tr1::shared_ptr<Transfer::DenseData> uploaded_mesh1(new Transfer::DenseData(serialized));
      {
        boost::mutex::scoped_lock modelSystemLock(mModelsSystemMutex);

        VisualPtr v = mModelsSystem->load(uploaded_mesh1);
        MeshdataPtr md1 = std::tr1::dynamic_pointer_cast<Meshdata>(v);
        addToInMemoryCache(cdnMeshName, md1);
      }

      //aggObject->mLeaves.clear();

      Duration upload_time = Timer::now() - uploadStartTime;
      {
        boost::mutex::scoped_lock modelSystemLock(mStatsMutex);
        mAggregateCumulativeUploadTime += upload_time;
      }
  }

  boost::filesystem::remove_all("/tmp/sirikata/"+uuid.toString()+".dir");
}

void MeshAggregateManager::handleUploadFinished(Transfer::UploadRequestPtr request, const Transfer::URI& path, Mesh::MeshdataPtr agg_mesh,
                                            String atlas_name,
                                            std::tr1::shared_ptr<char> atlas_buffer,
                                            uint32 atlas_length, AggregateObjectPtr aggObject,
                                            std::tr1::unordered_map<String, String> textureSet, uint32 retryAttempt,
                                            const Time& uploadStartTime)
{
    Transfer::URI generated_uri = path;
    const UUID& uuid = aggObject->mUUID;
    String localMeshName = boost::lexical_cast<String>(aggObject->mTreeLevel) +
                         "_aggregate_mesh_" +
                         uuid.toString() + ".dae";

    if (generated_uri.empty()) {
      //There was a problem during the upload. Try again!
      AGG_LOG(error, "Failed to upload aggregate mesh " << localMeshName << ", composed of these children meshes:");

      boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
      const std::vector<AggregateObjectPtr> children = aggObject->getChildrenCopy();
      for (uint32 i= 0; i < children.size(); i++) {
        UUID child_uuid = children[i]->mUUID;
        if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end() )
          continue;

        std::tr1::shared_ptr<LocationInfo> locInfo = getCachedLocInfo(child_uuid);

        String meshName = (locInfo) ? locInfo->mesh() :

                                      "LOC does not have meshname for " + child_uuid.toString();
        AGG_LOG(error, "   " << meshName);
      }

      AGG_LOG(error, "Failure was retry attempt # " << retryAttempt);
      //Retry uploading up to 5 times.
      if (retryAttempt < 5) {
	mUploadStrands[rand() % mNumUploadThreads]->post(
		  std::tr1::bind(&MeshAggregateManager::uploadAggregateMesh, this, agg_mesh, atlas_name, atlas_buffer, atlas_length, aggObject, textureSet, retryAttempt + 1, uploadStartTime),
		  "MeshAggregateManager::uploadAggregateMesh"
		);
      }
      else if (retryAttempt < 10) {
	//Could not upload -- CDN might be overloaded, try again in 30 seconds.
	mUploadStrands[rand() % mNumUploadThreads]->post(Duration::seconds(15),
                  std::tr1::bind(&MeshAggregateManager::uploadAggregateMesh, this, agg_mesh, atlas_name, atlas_buffer, atlas_length, aggObject, textureSet, retryAttempt + 1, uploadStartTime),
                  "MeshAggregateManager::uploadAggregateMesh"
                );
      }
      else if (retryAttempt < 15) {
        //Still cannot upload - just upload an empty mesh so remaining meshes higher up in the tree can be generated.
	MeshdataPtr m = MeshdataPtr(new Meshdata);
        mUploadStrands[rand() % mNumUploadThreads]->post(Duration::seconds(15),
          std::tr1::bind(&MeshAggregateManager::uploadAggregateMesh, this, m, atlas_name, atlas_buffer, atlas_length, aggObject, textureSet, retryAttempt + 1, uploadStartTime),
                  "MeshAggregateManager::uploadAggregateMesh"
                );
      }
      else {
        // It really failed for the last time
        mAggregatesFailedToUpload++;
      }

      return;
    }

    // The current CDN URL layout is kind of a pain. We'll get back something
    // like:
    // meerkat://localhost/echeslack/apiupload/multimtl.dae/13
    // and the target model will look something like:
    // meerkat://localhost/echeslack/apiupload/multimtl.dae/original/13/multimtl.dae
    // so we need to extract the number at the end so we can insert it between
    // the format and the filename.
    String cdnMeshName = generated_uri.toString();
    // Store this info and mark it for TTL refresh 3/4 through the TTL. We
    // only want the path part (i.e. /username/foo/model.dae/0) and not the
    // protocol + host part (i.e. meerkat://foo.com:8000).
    aggObject->cdnBaseName = Transfer::URL(cdnMeshName).fullpath();
    aggObject->refreshTTL = mLoc->context()->recentSimTime() + (mModelTTL*.75);
    // And extract the URL to hand out to users
    std::size_t upload_num_pos = cdnMeshName.rfind("/");
    assert(upload_num_pos != String::npos);
    String mesh_num_part = cdnMeshName.substr(upload_num_pos+1);
    cdnMeshName = cdnMeshName.substr(0, upload_num_pos);
    String cdnMeshNamePrefix = cdnMeshName;
    cdnMeshName = cdnMeshName + "/original/" + mesh_num_part + "/" + localMeshName;
    agg_mesh->uri = cdnMeshName;

    //Update loc
    mLoc->context()->mainStrand->post(
        std::tr1::bind(
            &MeshAggregateManager::updateAggregateLocMesh, this,
            uuid, cdnMeshName
        ),
        "MeshAggregateManager::updateAggregateLocMesh"
    );

    mAggregatesUploaded++;
    AGG_LOG(info, "Uploaded successfully: " << localMeshName);
    AGG_LOG(insane,  "CDN mesh name is " << cdnMeshName);
    //FIXME: This assumes that atlasing always happens! Fix this!
    for (uint32 i = 0 ; i < agg_mesh->textures.size() && atlas_buffer; i++) {
      String texturename = agg_mesh->textures[i];
      agg_mesh->textures[i] = cdnMeshNamePrefix + "/original/" + texturename.substr(texturename.find_last_of("/")+1)  + "/" + mesh_num_part;
      AGG_LOG(insane, texturename << " to " << agg_mesh->textures[i] << "\n");
      aggObject->mAtlasPath = agg_mesh->textures[i];
    }
    for(MaterialEffectInfoList::iterator mat_it = agg_mesh->materials.begin(); mat_it != agg_mesh->materials.end(); mat_it++) {
      for(MaterialEffectInfo::TextureList::iterator tex_it = mat_it->textures.begin(); tex_it != mat_it->textures.end(); tex_it++) {
        if (!tex_it->uri.empty()) {
	  String texturename = tex_it->uri;
          tex_it->uri = cdnMeshNamePrefix + "/original/" + texturename.substr(texturename.find_last_of("/")+1)  + "/" + mesh_num_part;
        }
      }
    }


    addToInMemoryCache(cdnMeshName, agg_mesh);
    aggObject->mMeshdata = agg_mesh;
    //aggObject->mLeaves.clear();

    Duration upload_time = Timer::now() - uploadStartTime;
    {
      boost::mutex::scoped_lock modelSystemLock(mStatsMutex);
      mAggregateCumulativeUploadTime += upload_time;
    }
}

void MeshAggregateManager::metadataFinished(Time t, const UUID uuid, const UUID child_uuid, std::string meshName, uint8 attemptNo,
                                          std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                          std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response)
{
  if (response != NULL) {
    AGG_LOG(detailed, ( Timer::now() - t )  << " : metadataFinished SUCCESS\n");

    const Transfer::RemoteFileMetadata metadata = *response;

    Transfer::TransferRequestPtr req(new Transfer::ChunkRequest(response->getURI(), metadata,
                                         response->getChunkList().front(), 1.0,
                                         std::tr1::bind(&MeshAggregateManager::chunkFinished, this, t,uuid, child_uuid, meshName,
                                                        std::tr1::placeholders::_1,
                                                        std::tr1::placeholders::_2) ) );

    mTransferPool->addRequest(req);

    const Sirikata::Transfer::FileHeaders& headers = response->getHeaders();
    String zernike = "";
    if (headers.find("Zernike") != headers.end()) {
      zernike = (headers.find("Zernike"))->second;
      Prox::ZernikeDescriptor zd(zernike) ;
      if (zd.size() == 121) {
        boost::mutex::scoped_lock lock(mMeshStoreMutex);
        mMeshDescriptors[meshName] = zd;
        //std::cout << meshName << " " << zernike << " :  zernike stored\n";
      }
    }

    boost::mutex::scoped_lock lock(mMeshStoreMutex);
    mMeshSizeMap[meshName] = response->getSize();
  }
  else if (attemptNo < 5) {
    AGG_LOG(warn, "Failed metadata download: Retrying...: Response time: "   << ( Timer::now() - t ) << "\n");
    Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &MeshAggregateManager::metadataFinished, this, t, uuid, child_uuid, meshName,
                                       attemptNo+1, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);
  }
  else {
    // Metadata for the file was not found, or the file was not found.
    // Use an empty Meshdata in that case.
    MeshdataPtr m = std::tr1::shared_ptr<Meshdata>(new Meshdata);
    m->uri = meshName;

    boost::mutex::scoped_lock aggregateObjectsLock(mAggregateObjectsMutex);
    mAggregateObjects[child_uuid]->mMeshdata = m;
    aggregateObjectsLock.unlock();

    addToInMemoryCache(request->getURI().toString(), m);
    AGG_LOG(warn, "Adding a fake mesh for " << meshName << " because it wasnt available on the CDN\n");
  }
}

void MeshAggregateManager::chunkFinished(Time t, const UUID uuid, const UUID child_uuid, std::string meshName,
                                       std::tr1::shared_ptr<Transfer::ChunkRequest> request,
                                       std::tr1::shared_ptr<const Transfer::DenseData> response)
{
    if (response != NULL) {
      //AGG_LOG(detailed, "Time spent downloading: " << (Timer::now() - t) << "\n");

      boost::mutex::scoped_lock aggregateObjectsLock(mAggregateObjectsMutex);
      if (mAggregateObjects[child_uuid]->mMeshdata == MeshdataPtr() ) {

        VisualPtr v;
        {
	  boost::mutex::scoped_lock modelSystemLock(mModelsSystemMutex);
          v = mModelsSystem->load(request->getMetadata(), request->getMetadata().getFingerprint(), response);
        }
        // FIXME handle non-Meshdata formats
        MeshdataPtr m = std::tr1::dynamic_pointer_cast<Meshdata>(v);
        //Center the mesh, as its done on the client side for display.
        {
          boost::mutex::scoped_lock filterlock(mCenteringFilterMutex);
          Mesh::MutableFilterDataPtr input_data(new Mesh::FilterData);
          input_data->push_back(m);
          Mesh::FilterDataPtr output_data = mCenteringFilter->apply(input_data);
          m = std::tr1::dynamic_pointer_cast<Mesh::Meshdata> (output_data->get());
        }


        mAggregateObjects[child_uuid]->mMeshdata = m;

        aggregateObjectsLock.unlock();

	addToInMemoryCache(request->getURI().toString(), m);

	AGG_LOG(detailed, "Stored mesh in mesh store for: " <<  request->getURI().toString() << "\n");
      }
    }
    else {
      AGG_LOG(warn, "ChunkFinished fail... retrying\n");
      Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &MeshAggregateManager::metadataFinished, this, t, uuid, child_uuid, meshName,
                                       1, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

      mTransferPool->addRequest(req);
    }
}

void MeshAggregateManager::addToInMemoryCache(const String& meshName, const MeshdataPtr mdptr) {
  boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);

  int MESHSTORESIZE=10000;
  //Store the mesh but keep the meshstore's size under control.
  if (   mMeshStore.size() > MESHSTORESIZE
      && mMeshStore.find(meshName) == mMeshStore.end())
  {
    int countMeshes = 0;
    std::tr1::unordered_map<String, Mesh::MeshdataPtr>::iterator it = mMeshStore.begin();
    for (; it != mMeshStore.end() && countMeshes <= MESHSTORESIZE; it++) {
      if (it->second) {
        countMeshes++;
      }
    }

    if (countMeshes > MESHSTORESIZE) {
      mMeshStore.erase(mMeshStoreOrdering.begin()->second);
      AGG_LOG(insane, "Erasing from meshstore: " <<mMeshStoreOrdering.begin()->second
                << " " << mMeshStoreOrdering.begin()->first
                << mCurrentInsertionNumber   );

      mMeshDescriptors.erase(mMeshStoreOrdering.begin()->second);
      mMeshStoreOrdering.erase(mMeshStoreOrdering.begin());
    }
  }

  AGG_LOG(insane, "Inserting to meshstore: " << meshName);
  mCurrentInsertionNumber++;
  mMeshStore[meshName] = mdptr;
  mMeshStoreOrdering[mCurrentInsertionNumber]=meshName;
}

void MeshAggregateManager::addLeavesUpTree(UUID leaf_uuid, UUID uuid) {
  if (uuid == UUID::null()) return;
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) return;

  std::tr1::shared_ptr<AggregateObject> obj = mAggregateObjects[uuid];
  if (!obj) return;
  std::tr1::shared_ptr<LocationInfo> locInfo = getCachedLocInfo(uuid);
  if (!locInfo) return;
  /*if (mDirtyAggregateObjects.find(uuid) != mDirtyAggregateObjects.end()) {
    float radius = locInfo->bounds.fullRadius();
    float solid_angle = TWO_PI * (1-sqrt(1- pow(radius/obj->mDistance,2)));

    if (solid_angle > ONE_PIXEL_SOLID_ANGLE) {
      obj->mLeaves.insert(leaf_uuid);
    }
  }*/
  //obj->mLeaves.insert(mAggregateObjects[leaf_uuid]);

  for (std::set<UUID>::iterator it = obj->mParentUUIDs.begin(); it != obj->mParentUUIDs.end(); it++) {
    addLeavesUpTree(leaf_uuid, *it);
  }
}

void MeshAggregateManager::getLeaves(const std::vector<UUID>& individualObjects) {
  for (uint32 i=0; i<individualObjects.size(); i++) {
    UUID indl_uuid = individualObjects[i];
    addLeavesUpTree(indl_uuid, indl_uuid);
  }
}

void MeshAggregateManager::queueDirtyAggregates(Time postTime) {
    if (postTime < mAggregateGenerationStartTime) {
      return;
    }

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    //Get the leaves that belong to each node.
    std::vector<UUID> individualObjects;

    if ( mDirtyAggregateObjects.size() > 0 ) {
      for (std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher >::iterator it = mAggregateObjects.begin();
           it != mAggregateObjects.end() ; it++)
      {
          std::tr1::shared_ptr<AggregateObject> aggObject = it->second;

          if (aggObject->childrenSize() == 0) {
            individualObjects.push_back(aggObject->mUUID);
          }

          if (mDirtyAggregateObjects.find(it->first) == mDirtyAggregateObjects.end()) {
            continue;
          }

          float radius  = INT_MAX;
          const std::vector<AggregateObjectPtr> children = aggObject->getChildrenCopy();
          for (uint32 i=0; i < children.size(); i++) {
            UUID& child_uuid = children[i]->mUUID;

            std::tr1::shared_ptr<LocationInfo> locInfo = getCachedLocInfo(child_uuid);
            if (!locInfo) continue;

            float32 bnds_rad = locInfo->bounds().fullRadius();
            if (bnds_rad < radius) {
              radius = bnds_rad;
            }
          }

          if (radius == INT_MAX) radius = 0;

          aggObject->mDistance = 0.001 + minimumDistanceForCut(aggObject);
      }

      //getLeaves(individualObjects);
    }

    lock.unlock();

    //Grab locks for all of the generation threads before proceeding.
    uint8 i = 0;
    boost::mutex::scoped_lock queueLocks[MAX_NUM_GENERATION_THREADS];
    for (uint8 i = 0; i < mNumGenerationThreads; i++) {
      queueLocks[i] = boost::mutex::scoped_lock(mObjectsByPriorityLocks[i]);
    }

    boost::mutex::scoped_lock queuedObjectsLock(mQueuedObjectsMutex);
    i=0;
    //Add objects to generation queue, ordered by priority.
    for (std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher>::iterator it = mDirtyAggregateObjects.begin();
         it != mDirtyAggregateObjects.end(); it++)
    {
      std::tr1::shared_ptr<AggregateObject> aggObject = it->second;
      UUID uuid = aggObject->mUUID;

      //Erase if this uuid is already queued.
      if (mQueuedObjects.find(uuid) != mQueuedObjects.end()) {
        std::pair<uint32, uint32>& threadtreeLevelPair = mQueuedObjects[uuid];

        AGG_LOG(insane, std::cout << "Found existing uuid: " << uuid
                  << " -- " << threadtreeLevelPair.first
                  << " -- " << threadtreeLevelPair.second
                  << "\n");

        std::deque<AggregateObjectPtr>& theDeque = mObjectsByPriority[threadtreeLevelPair.first]
                                                                     [threadtreeLevelPair.second];
        for (std::deque<AggregateObjectPtr>::iterator deq_it = theDeque.begin();
             deq_it != theDeque.end(); deq_it++)
        {
          if ( (*deq_it)->mUUID == uuid) {
            AGG_LOG(insane,  "Erased existing uuid: " << uuid << "\n");
            theDeque.erase(deq_it);
            break;
          }
        }
      }

      //Now queue it up!
      if (aggObject->mTreeLevel >= 0) {
        mAggregatesQueued++;
        mQueuedObjects[uuid] = std::pair<uint32,uint32>(i, aggObject->mTreeLevel);

        mObjectsByPriority[i][  (aggObject->mTreeLevel) ].push_back(aggObject);

        AGG_LOG(insane,  uuid << " : " << aggObject->mTreeLevel << " -- " <<
                     (aggObject->mTreeLevel)  << " -- enqueued in " <<  ((uint32)i)  <<  " \n");

        i = (i+1) % mNumGenerationThreads;
      }
      else {
        AGG_LOG(insane,  uuid << " not enqueued\n");
      }
    }

    queuedObjectsLock.unlock();
    mDirtyAggregateObjects.clear();

    for (i = 0; i < mNumGenerationThreads; i++) {
      //setAggregatesTriangleCount();

      mAggregationStrands[i]->post(
          std::tr1::bind(&MeshAggregateManager::generateMeshesFromQueue, this, i),
          "MeshAggregateManager::generateMeshesFromQueue"
      );
    }
}

void MeshAggregateManager::generateMeshesFromQueue(uint8 threadNumber) {
    boost::mutex::scoped_lock lock(mObjectsByPriorityLocks[threadNumber]);
    if (noMoreGeneration) return;

    //Generate the aggregates from the priority queue.
    Time curTime = (mObjectsByPriority[threadNumber].size() > 0) ? Timer::now() : Time::null();
    uint32 returner = GEN_SUCCESS;
    uint32 numFailedAttempts = 1;
    bool noObjectsToGenerate = true;
    for (std::map<float, std::deque<AggregateObjectPtr> >::reverse_iterator it =  mObjectsByPriority[threadNumber].rbegin();
         it != mObjectsByPriority[threadNumber].rend(); it++)
    {

      if (it->second.size() > 0) {
        noObjectsToGenerate = false;
        std::tr1::shared_ptr<AggregateObject> aggObject = it->second.front();

        AGG_LOG(insane,  "Top " << ((uint32)threadNumber) << " : " << aggObject->mTreeLevel << "_" << aggObject->mUUID << "\n");

        if (aggObject->generatedLastRound) {
          continue;
        }

        returner=generateAggregateMeshAsync(aggObject->mUUID, curTime, false);
        AGG_LOG(insane, "returner: " << returner << " for " << aggObject->mUUID << "\n");

        if (returner==GEN_SUCCESS || aggObject->mNumFailedGenerationAttempts > 16) {
          boost::mutex::scoped_lock queuedObjectsLock(mQueuedObjectsMutex);
          mQueuedObjects.erase(aggObject->mUUID);
          queuedObjectsLock.unlock();

          it->second.pop_front();
          if (returner != GEN_SUCCESS) {
            mAggregatesFailedToGenerate++;
              mLoc->context()->mainStrand->post(
                std::tr1::bind(
                  &MeshAggregateManager::updateAggregateLocMesh, this,
                  aggObject->mUUID, "meerkat:///tahirazim/apiupload/test_project_3_scene_shape_60270.dae/optimized/0/test_project_3_scene_shape_60270.dae"
                ),
                "MeshAggregateManager::updateAggregateLocMesh"
            );

            AGG_LOG(error, "Could not generate aggregate mesh for " <<
                           aggObject->mTreeLevel << "_" << aggObject->mUUID.toString() << ". Setting it to a fake flat mesh.\n");
          }

          aggObject->mNumFailedGenerationAttempts = 0;
        }
        else if (returner != CHILDREN_NOT_YET_GEN) {
          aggObject->mNumFailedGenerationAttempts++;
          numFailedAttempts = aggObject->mNumFailedGenerationAttempts;
        }

        break;
      }
    }

    if (noObjectsToGenerate) {
      mObjectsByPriority[threadNumber].clear();
    }

    if (mObjectsByPriority[threadNumber].size() > 0) {
      Duration dur = Duration::milliseconds(1.0);
      if (returner == GEN_SUCCESS) {
        // 1 ms is fine
      }
      else if (returner == CHILDREN_NOT_YET_GEN) {
        dur = Duration::milliseconds(500.0);
      }
      else { // need to back off for all other causes
        dur = Duration::milliseconds(10.0*pow(2.f,(float)numFailedAttempts));
      }

      mAggregationStrands[threadNumber]->post(
          dur,
          std::tr1::bind(&MeshAggregateManager::generateMeshesFromQueue, this, threadNumber),
          "MeshAggregateManager::generateMeshesFromQueue"
      );
    }
}


void MeshAggregateManager::updateChildrenTreeLevel(const UUID& uuid, uint16 treeLevel) {
    //mAggregateObjectsMutex MUST be locked BEFORE calling this function.

    /*Check for the rare case where an aggregate may be removed (through removeAggregate)
      before it is cleared from its parents' list of children (through removeChild). */
    if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) {
      return;
    }

    mAggregateObjects[uuid]->mTreeLevel = treeLevel;

    const std::vector<AggregateObjectPtr> children = mAggregateObjects[uuid]->getChildrenCopy();
    for (uint32 i = 0; i < children.size(); i++) {
      updateChildrenTreeLevel(children[i]->mUUID, treeLevel+1);
    }
}

//Recursively add uuid and all nodes upto the root to the dirty aggregates map.
void MeshAggregateManager::addDirtyAggregates(UUID uuid) {
  //mAggregateObjectsMutex MUST be locked BEFORE calling this function.
  if (uuid != UUID::null() && mAggregateObjects.find(uuid) != mAggregateObjects.end() ) {
    std::tr1::shared_ptr<AggregateObject> aggObj = mAggregateObjects[uuid];

    if (aggObj) {
      // We used to check if there were any children here, but we need to mark
      // this as dirty (if it's really an aggregate...) no matter what. It may
      // have gone from having children to not having children. We also must
      // always mark parents since removal of objects is important to them as
      // well.
      if (!aggObj->leaf) {
        mDirtyAggregateObjects[uuid] = aggObj;
        aggObj->generatedLastRound = false;
      }
      for (std::set<UUID>::iterator it = aggObj->mParentUUIDs.begin(); it != aggObj->mParentUUIDs.end(); it++) {
        addDirtyAggregates(*it);
      }
    }
  }
}

bool MeshAggregateManager::findChild(std::vector<MeshAggregateManager::AggregateObjectPtr>& v,
                                 const UUID& uuid)
{
  for (uint32 i=0; i < v.size(); i++) {
    if (v[i]->mUUID == uuid) {
      return true;
    }
  }

  return false;
}

//Checks if the given UUID is the name of an aggregate object.
//Note: This function locks mAggregateObjectsMutex.
bool MeshAggregateManager::isAggregate(const UUID& child_uuid) {
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  return (mAggregateObjects.find(child_uuid) != mAggregateObjects.end()
          && mAggregateObjects[child_uuid]->childrenSize() > 0);
}


void MeshAggregateManager::removeStaleLeaves() {
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

  std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher >::iterator it =
                                                           mAggregateObjects.begin();
  std::vector<UUID> markedForRemoval;

  while (it != mAggregateObjects.end()) {
    if (it->second->leaf && it->second.use_count() == 1) {
      markedForRemoval.push_back(it->first);
    }

    it++;
  }

  for (uint32 i = 0; i < markedForRemoval.size(); i++) {
    mAggregateObjects.erase(markedForRemoval[i]);
  }

  if (mAggregationStrands[0]) {
    mAggregationStrands[0]->post(
      Duration::seconds(60),
      std::tr1::bind(&MeshAggregateManager::removeStaleLeaves, this),
      "MeshAggregateManager::removeStaleLeaves"
    );
  }

}

void MeshAggregateManager::updateAggregateLocMesh(UUID uuid, String mesh) {
  if (mLoc->contains(uuid)) {
    mLoc->updateLocalAggregateMesh(uuid, mesh);
  }
}

void MeshAggregateManager::localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc,
                                                const TimedMotionQuaternion& orient,
                                                const AggregateBoundingInfo& bounds, const String& mesh, const String& physics,
                                                const String& zernike)
{
  boost::mutex::scoped_lock lock(mLocCacheMutex);
  mLocationServiceCache.insertLocationInfo(uuid, std::tr1::shared_ptr<LocationInfo>(
                                           new LocationInfo(loc.position(), bounds, orient.position(), mesh)));
}

void MeshAggregateManager::localObjectRemoved(const UUID& uuid, bool agg) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);
  mLocationServiceCache.removeLocationInfo(uuid);
}

void MeshAggregateManager::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);

  std::tr1::shared_ptr<LocationInfo> locinfo = mLocationServiceCache.getLocationInfo(uuid);
  if (!locinfo) return;

  locinfo->currentPosition(newval.position());
}

void MeshAggregateManager::localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);

  std::tr1::shared_ptr<LocationInfo> locinfo = mLocationServiceCache.getLocationInfo(uuid);
  if (!locinfo) return;

  locinfo->currentOrientation(newval.position());
}

void MeshAggregateManager::localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);

  std::tr1::shared_ptr<LocationInfo> locinfo = mLocationServiceCache.getLocationInfo(uuid);
  if (!locinfo) return;

  locinfo->bounds(newval);

}

void MeshAggregateManager::localMeshUpdated(const UUID& uuid, bool agg, const String& newval) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);

  std::tr1::shared_ptr<LocationInfo> locinfo = mLocationServiceCache.getLocationInfo(uuid);
  if (!locinfo) return;

  locinfo->mesh(newval);
}

void MeshAggregateManager::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc,
                                                const TimedMotionQuaternion& orient,
                                                const AggregateBoundingInfo& bounds, const String& mesh, const String& physics,
                                                const String& zernike)
{
  boost::mutex::scoped_lock lock(mLocCacheMutex);
  mLocationServiceCache.insertLocationInfo(uuid, std::tr1::shared_ptr<LocationInfo>(
                                           new LocationInfo(loc.position(), bounds, orient.position(), mesh)));
}

void MeshAggregateManager::replicaObjectRemoved(const UUID& uuid) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);
  mLocationServiceCache.removeLocationInfo(uuid);
}

void MeshAggregateManager::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);

  std::tr1::shared_ptr<LocationInfo> locinfo = mLocationServiceCache.getLocationInfo(uuid);
  if (!locinfo) return;

  locinfo->currentPosition(newval.position());
}

void MeshAggregateManager::replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);

  std::tr1::shared_ptr<LocationInfo> locinfo = mLocationServiceCache.getLocationInfo(uuid);
  if (!locinfo) return;

  locinfo->currentOrientation(newval.position());
}

void MeshAggregateManager::replicaBoundsUpdated(const UUID& uuid, const AggregateBoundingInfo& newval) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);

  std::tr1::shared_ptr<LocationInfo> locinfo = mLocationServiceCache.getLocationInfo(uuid);
  if (!locinfo) return;

  locinfo->bounds(newval);
}

void MeshAggregateManager::replicaMeshUpdated(const UUID& uuid, const String& newval) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);

  std::tr1::shared_ptr<LocationInfo> locinfo = mLocationServiceCache.getLocationInfo(uuid);
  if (!locinfo) return;

  locinfo->mesh(newval);
}

std::tr1::shared_ptr<MeshAggregateManager::LocationInfo> MeshAggregateManager::getCachedLocInfo(const UUID& uuid) {
  boost::mutex::scoped_lock lock(mLocCacheMutex);

  std::tr1::shared_ptr<LocationInfo> locinfo = mLocationServiceCache.getLocationInfo(uuid);

  return locinfo;
}


namespace {
  // Return true if the service is a default value
  bool ServiceIsDefault(const String& s) {
    if (!s.empty() && s != "http" && s != "80")
      return false;
    return true;
  }
  // Return empty if this is already empty or the default for this
  // service. Otherwise, return the old value
  String ServiceIfNotDefault(const String& s) {
    if (!ServiceIsDefault(s))
      return s;
    return "80";
  }
}


void MeshAggregateManager::sendKeepAlives() {
  // Don't bother unless we're uploading to the real CDN
  if (!mOAuth || mCDNUsername.empty())
    return;

  Time tnow = mLoc->context()->recentSimTime();

  // This could definitely be more efficient...
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  for(AggregateObjectsMap::iterator it = mAggregateObjects.begin(); it != mAggregateObjects.end(); it++) {
    if (it->second->refreshTTL != Time::null() &&
        it->second->refreshTTL < tnow &&
        !it->second->cdnBaseName.empty())
      {
        String keep_alive_path = "/api/keepalive" + it->second->cdnBaseName;
        // Currently there's no keepalive support in the Transfer
        // libraries, so we construct and send the http request ourselves.

        Network::Address cdn_addr(mOAuth->hostname, ServiceIfNotDefault(mOAuth->service));
        String full_oauth_hostinfo = mOAuth->hostname;
        if (!ServiceIsDefault(mOAuth->service))
          full_oauth_hostinfo += ":" + mOAuth->service;

        Transfer::HttpManager::Headers headers;
        headers["Host"] = full_oauth_hostinfo;

        Transfer::HttpManager::QueryParameters query_params;
        query_params["username"] = mCDNUsername;
        query_params["ttl"] = boost::lexical_cast<String>(mModelTTL.seconds());

        AGG_LOG(detailed, "Requesting TTL refresh for " << it->first);
        Transfer::OAuthHttpManager oauth_http(mOAuth);
        oauth_http.get(
                       cdn_addr, keep_alive_path,
                       std::tr1::bind(&MeshAggregateManager::handleKeepAliveResponse, this, it->first, _1, _2, _3),
                       headers, query_params
                       );

        /*if (it->second->mAtlasPath != "") {
	  keep_alive_path = "/api/keepalive"+it->second->mAtlasPath;
          Transfer::OAuthHttpManager oauth_http(mOAuth);
          oauth_http.get(
                       cdn_addr, keep_alive_path,
                       std::tr1::bind(&MeshAggregateManager::handleKeepAliveResponse, this, it->first, _1, _2, _3),
                       headers, query_params
                       );
        }  */
      }
  }
}



void MeshAggregateManager::handleKeepAliveResponse(const UUID& objid,
                                               std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> response,
                                               Transfer::HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error)
{
  // Check a bunch of error conditions, leaving the refresh TTL setting for
  // the next iteration is something went wrong.

  if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
    AGG_LOG(error, "Request parsing failed during aggregate TTL refresh (" << objid << ")");
    return;
  } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
    AGG_LOG(error, "Response parsing failed during aggregate TTL refresh (" << objid << ")");
    return;
  } else if (error == Transfer::HttpManager::BOOST_ERROR) {
    AGG_LOG(error, "A boost error happened during aggregate TTL refresh (" << objid << "). Boost error = " << boost_error.message());
    return;
  } else if (error != Transfer::HttpManager::SUCCESS) {
    AGG_LOG(error, "An unknown error happened during aggregate TTL refresh. (" << objid << ")");
    return;
  }

  if (response->getStatusCode() != 200) {
    AGG_LOG(error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during aggregate TTL refresh (" << objid << ")");
    return;
  }

  Transfer::DenseDataPtr response_data = response->getData();
  if (response_data && response_data->size() != 0) {
    AGG_LOG(error, "Got non-empty response durring aggregate TTL refresh: " << response_data->asString());
    return;
  }

  // If all these passed, then we were successful. Setup next refresh time
  AGG_LOG(detailed, "Successfully refreshed TTL for " << objid);
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  AggregateObjectsMap::iterator it = mAggregateObjects.find(objid);
  if (it == mAggregateObjects.end()) return;
  it->second->refreshTTL = mLoc->context()->recentSimTime() + (mModelTTL*.75);
}

float32 MeshAggregateManager::minimumDistanceForCut(AggregateObjectPtr aggObj) {
  const SolidAngle maxSolidAngle(1.f);

  std::tr1::shared_ptr<LocationInfo> uuidLocInfo = getCachedLocInfo(aggObj->mUUID);

  if (!uuidLocInfo) {
    //assume its at the minimal distance
    return 0;
  }
  return maxSolidAngle.maxDistance(uuidLocInfo->bounds().fullRadius());
}

void MeshAggregateManager::setAggregatesTriangleCount() {
  //assumes only one parent in mParentUUIDs

  //first, find the root.
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

  for (std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher >::iterator it = mAggregateObjects.begin();
           it != mAggregateObjects.end() ; it++)
  {
    AggregateObjectPtr aggObj = it->second;
    if (aggObj->mParentUUIDs.size() == 1 && *(aggObj->mParentUUIDs.begin()) == UUID::null()) {
      aggObj->mTriangleCount = 5000000;

      //setChildrensTriangleCount(aggObj);

      break;
    }
  }

}
/*
void MeshAggregateManager::setChildrensTriangleCount(AggregateObjectPtr aggObj) {
  std::tr1::shared_ptr<LocationInfo> uuidLocInfo = getCachedLocInfo(aggObj->mUUID);

  float32 sumSize = 0;
  for (uint32 i = 0; i < aggObj->childrenSize(); i++) {
    std::tr1::shared_ptr<LocationInfo> childLocInfo = getCachedLocInfo(aggObj->mChildren[i]->mUUID);
    sumSize += 4.0/3.0*3.14159*pow(childLocInfo->bounds().fullRadius(),3);
  }

  for (uint32 i = 0; i < aggObj->mChildren.size(); i++) {
    std::tr1::shared_ptr<LocationInfo> childLocInfo = getCachedLocInfo(aggObj->mChildren[i]->mUUID);

    aggObj->mChildren[i]->mTriangleCount =  aggObj->mTriangleCount *
                                            (4.0/3.0*3.14159*pow(childLocInfo->bounds().fullRadius(),3)) / sumSize;
    std::cout << aggObj->mChildren[i]->mTriangleCount << " : " << aggObj->mChildren[i]->mTreeLevel << "\n";

    setChildrensTriangleCount(aggObj->mChildren[i]);
  }

}
*/
void MeshAggregateManager::commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
  Command::Result result = Command::EmptyResult();

  result.put("stats.raw_updates", mRawAggregateUpdates.read());
  result.put("stats.queued", mAggregatesQueued.read());
  result.put("stats.generated", mAggregatesGenerated.read());
  result.put("stats.generation_failed", mAggregatesFailedToGenerate.read());
  result.put("stats.uploaded", mAggregatesUploaded.read());
  result.put("stats.upload_failed", mAggregatesFailedToUpload.read());

  {
    boost::mutex::scoped_lock modelSystemLock(mStatsMutex);
    result.put("stats.cumulative_generation_time", mAggregateCumulativeGenerationTime.toString());
    result.put("stats.cumulative_generation_time_seconds", mAggregateCumulativeGenerationTime.toSeconds());
    result.put("stats.cumulative_upload_time", mAggregateCumulativeUploadTime.toString());
    result.put("stats.cumulative_upload_time_seconds", mAggregateCumulativeUploadTime.toSeconds());
    result.put("stats.cumulative_size", mAggregateCumulativeDataSize);
  }

  {
    // Waiting count -- queued can count more (its how many requests went in,
    // not necessarily how many are generated), so things won't necessarily
    // match up with the counts coming out. Waiting counts how many items we
    // have sitting in a queue for processing, and it'll drop to 0 when things
    // settle down.
    uint32 waiting_count = 0;
    for (uint8 i = 0; i < mNumGenerationThreads; i++) {
      boost::mutex::scoped_lock queueLock(mObjectsByPriorityLocks[i]);
      for (std::map<float, std::deque<AggregateObjectPtr> >::iterator it =  mObjectsByPriority[i].begin();
           it != mObjectsByPriority[i].end(); it++) {
        waiting_count += it->second.size();
      }
    }
    // We might be in the wrong thread (mDirtyAggregateObjects should only be
    // used in the  main strand), but worst case we'll just get incorrect
    // stats.
    waiting_count += mDirtyAggregateObjects.size();
    result.put("stats.waiting", waiting_count);
  }

  cmdr->result(cmdid, result);
}


typedef struct Triangle{
  Vector3f pos1;
  Vector3f pos2;
  Vector3f pos3;
  float64 weight;

  float64 A;
  float64 B;
  float64 C;
  float64 D;

  Triangle(Vector3f p1, Vector3f p2, Vector3f p3) :
    pos1(p1), pos2(p2), pos3(p3)
  {
      Vector3f normal = (pos2 - pos1).cross(pos3-pos1);
      weight = normal.length();

      if (weight > 0) {
        normal = normal.normal();
        A = normal[0];
        B = normal[1];
        C = normal[2];
        D = -(normal.dot(pos1));
      }

  }

  bool operator<(const Triangle& t1) const {
    return weight < t1.weight;
  }

} Triangle;


float MeshAggregateManager::meshDiff(const UUID& uuid, std::tr1::shared_ptr<Meshdata> md_1, std::tr1::shared_ptr<Meshdata> md_2) {
  MeshdataPtr md1, md2;
  {
      boost::mutex::scoped_lock modelSystemLock(mModelsSystemMutex);

      std::stringstream model_ostream1(std::ofstream::out | std::ofstream::binary);
      {
        bool converted = mModelsSystem->convertVisual( md_1, "colladamodels", model_ostream1);
      }
      std::string serialized1 = model_ostream1.str();

      std::stringstream model_ostream2(std::ofstream::out | std::ofstream::binary);
      {
        bool converted = mModelsSystem->convertVisual( md_2, "colladamodels", model_ostream2);
      }
      std::string serialized2 = model_ostream2.str();

      std::tr1::shared_ptr<Transfer::DenseData> uploaded_mesh1(new Transfer::DenseData(serialized1));
      VisualPtr v = mModelsSystem->load(uploaded_mesh1);
      md1 = std::tr1::dynamic_pointer_cast<Meshdata>(v);

      std::tr1::shared_ptr<Transfer::DenseData> uploaded_mesh2(new Transfer::DenseData(serialized2));
      v = mModelsSystem->load(uploaded_mesh2);

      md2 = std::tr1::dynamic_pointer_cast<Meshdata>(v);

  }

  std::vector<Triangle> md1Triangles;
  float64 smallestMd1Weight = FLT_MAX;

  std::vector<Triangle> md2Triangles;
  float64 smallestMd2Weight = FLT_MAX;
  std::cout << uuid <<  " : entering meshdiff\n";

  //get all the triangles and their areas for md1
  Meshdata::GeometryInstanceIterator geoinst_it = md1->getGeometryInstanceIterator();
  Matrix4x4f orig_geo_inst_xform;
  uint32 geoinst_idx;
  while( geoinst_it.next(&geoinst_idx, &orig_geo_inst_xform) ) {
    const GeometryInstance& geomInstance = md1->instances[geoinst_idx];
    int geomIdx = geomInstance.geometryIndex;
    SubMeshGeometry& curGeometry = md1->geometry[geomIdx];

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      SubMeshGeometry::Primitive& primitive = curGeometry.primitives[j];

      for (uint32 k = 0; k+2 < primitive.indices.size(); k+=3) {
        unsigned short idx = primitive.indices[k];
        unsigned short idx2 = primitive.indices[k+1];
        unsigned short idx3 = primitive.indices[k+2];

        if (idx == USHRT_MAX && idx2 == USHRT_MAX && idx3 == USHRT_MAX) {
          continue;
        }

        if (idx == idx2 || idx == idx3 || idx2 == idx3) continue;

        Vector3f& orig_pos1 = curGeometry.positions[idx];
        Vector3f& orig_pos2 = curGeometry.positions[idx2];
        Vector3f& orig_pos3 = curGeometry.positions[idx3];


        Vector3f pos1 = orig_geo_inst_xform * orig_pos1;
        Vector3f pos2 = orig_geo_inst_xform * orig_pos2;
        Vector3f pos3 = orig_geo_inst_xform * orig_pos3;

        Triangle tri(pos1,pos2,pos3);
        float64 triWeight = tri.weight;

        if (triWeight == 0) continue;

        if (triWeight < smallestMd1Weight)
          smallestMd1Weight = triWeight;

        md1Triangles.push_back(tri);
      }
    }
  }

  std::cout << smallestMd1Weight  << " , "  << uuid << " : smallestMd1Weight\n";

  //get all the triangles and their areas for md2
  geoinst_it = md2->getGeometryInstanceIterator();
  while( geoinst_it.next(&geoinst_idx, &orig_geo_inst_xform) ) {
    const GeometryInstance& geomInstance = md2->instances[geoinst_idx];
    int geomIdx = geomInstance.geometryIndex;
    SubMeshGeometry& curGeometry = md2->geometry[geomIdx];

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      SubMeshGeometry::Primitive& primitive = curGeometry.primitives[j];

      for (uint32 k = 0; k+2 < primitive.indices.size(); k+=3) {
        unsigned short idx = primitive.indices[k];
        unsigned short idx2 = primitive.indices[k+1];
        unsigned short idx3 = primitive.indices[k+2];

        if (idx == USHRT_MAX && idx2 == USHRT_MAX && idx3 == USHRT_MAX) {
          continue;
        }

        if (idx == idx2 || idx == idx3 || idx2 == idx3) continue;

        Vector3f& orig_pos1 = curGeometry.positions[idx];
        Vector3f& orig_pos2 = curGeometry.positions[idx2];
        Vector3f& orig_pos3 = curGeometry.positions[idx3];


        Vector3f pos1 = orig_geo_inst_xform * orig_pos1;
        Vector3f pos2 = orig_geo_inst_xform * orig_pos2;
        Vector3f pos3 = orig_geo_inst_xform * orig_pos3;

        Triangle tri(pos1,pos2,pos3);
        float64 triWeight = tri.weight;

        if (triWeight == 0) continue;

        if (triWeight < smallestMd2Weight)
          smallestMd2Weight = triWeight;

        md2Triangles.push_back(tri);
      }
    }
  }

  std::cout << smallestMd2Weight << " , "  << uuid << " : smallestMd2Weight\n";

  std::cout << ((int)md2Triangles.size()) << " , "  << uuid << " : md2Triangles\n";
  std::cout << ((int)md1Triangles.size()) << " , "  << uuid << " : md1Triangles\n";

  if (md1Triangles.size() == 0 || md2Triangles.size() == 0) {
    std::cout << "md1Triangles.size() == 0 || md2Triangles.size() == 0 ZERO!\n";
    return 1;
  }

  md1Triangles[0].weight /= smallestMd1Weight;
  for (uint32 i = 1; i < md1Triangles.size(); i++) {
    md1Triangles[i].weight = md1Triangles[i].weight/smallestMd1Weight + md1Triangles[i-1].weight;
  }
  md2Triangles[0].weight /= smallestMd2Weight;
  for (uint32 i = 1; i < md2Triangles.size(); i++) {
    md2Triangles[i].weight = md2Triangles[i].weight/smallestMd2Weight + md2Triangles[i-1].weight;
  }

  //get point samples.
  uint32 NUM_POINT_SAMPLES=100;
  unsigned int seedp;
  std::vector<Vector3f> md1PointSamples;
  std::vector<Vector3f> md2PointSamples;

  std::cout << md1Triangles[md1Triangles.size() - 1].weight << " , "  << uuid
            << " : md1Triangles[md1Triangles.size() - 1].weight\n";
  uint64 sumWeights = md1Triangles[md1Triangles.size() - 1].weight;
  if (sumWeights == 0) {
    for (uint32 i = 0; i < md1Triangles.size(); i++) {
      std::cout << md1Triangles[i].weight  << " , "  << uuid << " : md1Triangles[i].weight\n";
    }
  }
  assert(sumWeights > 0);
  for (uint32 i = 0; i < NUM_POINT_SAMPLES; i++) {
    uint64 randomNumber = rand_r(&seedp) % sumWeights;
    uint32 triangleIdx = 0;
    //do a binary search to find this random number in the array
    int32 lo = 0, hi = md1Triangles.size() -1;
    int32 mid = (hi+lo)/2;
    while (hi > lo) {
      //std::cout << lo << " " << hi << " " << mid << "\n";

      if (md1Triangles[mid].weight > randomNumber) lo=mid+1;
      else if (md1Triangles[mid].weight < randomNumber) hi = mid -1;
      else if (md1Triangles[mid].weight == randomNumber) break;

      mid = (lo+hi)/2;
      if (mid < 0) mid = 0;
    }
    triangleIdx = mid;

    float u =  ((double) rand_r(&seedp) / (RAND_MAX));
    float v =  ((double) rand_r(&seedp) / (RAND_MAX));
    if (u+v > 1) {
      //u+v > 1;   x+y=1-u+1-v = 2 -u -v = 2-(u+v). Since u+v > 1, 2-(u+v) < 1.
      u = 1.0 - u;
      v = 1.0 - v;
    }

    Triangle& triangle = md1Triangles[triangleIdx];
    Vector3f point = triangle.pos1*u + triangle.pos2*v + triangle.pos3*(1.0-u-v);
    md1PointSamples.push_back(point);
  }

  std::cout << md2Triangles[md2Triangles.size() - 1].weight << " , "  << uuid
            << " : md2Triangles[md2Triangles.size() - 1].weight\n";
  sumWeights = md2Triangles[md2Triangles.size() - 1].weight;
  assert(sumWeights > 0);
  for (uint32 i = 0; i < NUM_POINT_SAMPLES; i++) {
    uint64 randomNumber = rand_r(&seedp) % sumWeights;
    uint32 triangleIdx = 0;
    //do a binary search to find this random number in the array
    int32 lo = 0, hi = md2Triangles.size() -1;
    int32 mid = (hi+lo)/2;
    while (hi > lo) {
      //std::cout << lo << " " << hi << " " << mid << "\n";
      if (md2Triangles[mid].weight > randomNumber) lo=mid+1;
      else if (md2Triangles[mid].weight < randomNumber) hi = mid -1;
      else if (md2Triangles[mid].weight == randomNumber) break;

      mid = (lo+hi)/2;
      if (mid < 0) mid = 0;
    }
    triangleIdx = mid;

    float u =  ((double) rand_r(&seedp) / (RAND_MAX));
    float v =  ((double) rand_r(&seedp) / (RAND_MAX));
    if (u+v > 1) {
      //u+v > 1;   x+y=1-u+1-v = 2 -u -v = 2-(u+v). Since u+v > 1, 2-(u+v) < 1.
      u = 1.0 - u;
      v = 1.0 - v;
    }

    Triangle& triangle = md2Triangles[triangleIdx];
    Vector3f point = triangle.pos1*u + triangle.pos2*v + triangle.pos3*(1.0-u-v);
    md2PointSamples.push_back(point);
  }

  std::cout <<  "finding diffs , "  << uuid << "\n";

  //find and sum the squares of the shortest distances.
  float64 sum_of_squared_distances = 0;
  for (uint32 i = 0; i < md1PointSamples.size(); i++) {
    Vector3f& point = md1PointSamples[i];
    float64 minDistance = FLT_MAX;

    for (uint32 j = 0; j < md2Triangles.size(); j++) {
      Triangle& triangle = md2Triangles[j];

      float64 A = triangle.A, B = triangle.B, C = triangle.C, D=triangle.D;

      float64 distance = pow(A*point.x + B*point.y + C*point.z+D,2)/(A*A+B*B+C*C);
      if (distance < minDistance) minDistance = distance;
    }

    if (minDistance != FLT_MAX) {
      sum_of_squared_distances += minDistance;
    }
  }

  for (uint32 i = 0; i < md2PointSamples.size(); i++) {
    Vector3f& point = md2PointSamples[i];
    float64 minDistance = FLT_MAX;

    for (uint32 j = 0; j < md1Triangles.size(); j++) {
      Triangle& triangle = md1Triangles[j];

      float64 A = triangle.A, B = triangle.B, C = triangle.C, D=triangle.D;
      float64 distance = pow(A*point.x + B*point.y + C*point.z+D,2)/(A*A+B*B+C*C);
      if (distance < minDistance) minDistance = distance;
    }

    if (minDistance != FLT_MAX) {
      sum_of_squared_distances += minDistance;
    }
  }

  return sum_of_squared_distances/((float)(md1PointSamples.size() + md2PointSamples.size()));
}


}
