/*  Sirikata
 *  AssetDownloadTask.hpp
 *
 *  Copyright (c) 2011, Stanford University
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

#include <sirikata/ogre/OgreRenderer.hpp>
#include <sirikata/ogre/resourceManager/AssetDownloadTask.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>

using namespace std::tr1::placeholders;
using namespace Sirikata::Transfer;
using namespace Sirikata::Mesh;

namespace Sirikata {
std::tr1::shared_ptr<AssetDownloadTask> AssetDownloadTask::construct(const Transfer::URI& uri, Graphics::OgreRenderer* const scene, double priority, FinishedCallback cb){
    std::tr1::shared_ptr<AssetDownloadTask> retval(SelfWeakPtr<AssetDownloadTask>::internalConstruct(new AssetDownloadTask(uri,scene,priority,cb)));
    retval->downloadAssetFile();
    return retval;
}
AssetDownloadTask::AssetDownloadTask(const Transfer::URI& uri, Graphics::OgreRenderer* const scene, double priority, FinishedCallback cb)
 : mScene(scene),
   mAssetURI(uri),
   mPriority(priority),
   mCB(cb),
   mAsset(),
   mRemainingDownloads(0)
{
}

AssetDownloadTask::~AssetDownloadTask() {
    cancel();
}

void AssetDownloadTask::cancel() {
    for(ActiveDownloadMap::iterator it = mActiveDownloads.begin(); it != mActiveDownloads.end(); it++)
        it->second->cancel();
    mActiveDownloads.clear();
}

void AssetDownloadTask::downloadAssetFile() {
    assert( !mAssetURI.empty() );

    ResourceDownloadTaskPtr dl = ResourceDownloadTask::construct(
        mAssetURI, mScene->transferPool(),
        mPriority,
        std::tr1::bind(&AssetDownloadTask::weakAssetFileDownloaded, getWeakPtr(), _1, _2)
    );
    mActiveDownloads[mAssetURI] = dl;
    dl->start();
}
void AssetDownloadTask::weakAssetFileDownloaded(std::tr1::weak_ptr<AssetDownloadTask> thus, std::tr1::shared_ptr<ChunkRequest> request, std::tr1::shared_ptr<const DenseData> response) {
    std::tr1::shared_ptr<AssetDownloadTask> locked(thus.lock());
    if (locked){
        locked->assetFileDownloaded(request,response);
    }
}
void AssetDownloadTask::assetFileDownloaded(std::tr1::shared_ptr<ChunkRequest> request, std::tr1::shared_ptr<const DenseData> response) {
    // Clear from the active download list
    assert(mActiveDownloads.size() == 1);
    mActiveDownloads.erase(mAssetURI);

    // Lack of response data means failure of some sort
    if (!response) {
        failDownload();
        return;
    }

    // FIXME here we could have another callback which lets them get
    // at the hash to try to use an existing copy. Even with the
    // eventual centralized loading we want, this may still be
    // beneficial since Ogre may have a copy even if we don't have a
    // copy of the raw data any more.

    mScene->parseMesh(
        mAssetURI, request->getMetadata().getFingerprint(), response,
        std::tr1::bind(&AssetDownloadTask::weakHandleAssetParsed, getWeakPtr(), _1)
    );
}
void AssetDownloadTask::weakHandleAssetParsed(std::tr1::weak_ptr<AssetDownloadTask> thus, Mesh::MeshdataPtr md){
    std::tr1::shared_ptr<AssetDownloadTask> locked(thus.lock());
    if (locked){
        locked->handleAssetParsed(md);
    }
}
void AssetDownloadTask::handleAssetParsed(Mesh::MeshdataPtr md) {
    mAsset = md;

    if (!md) {
        SILOG(ogre,error,"Failed to parse mesh " << mAssetURI.toString());
        mCB();
        return;
    }

    // This is a sanity check. There's no way Ogre can reasonably handle meshes
    // that require a ton of draw calls. Estimate them here and if its too high,
    // destroy the data and invoke the callback to make it look like failure.
    {
        // Draw calls =
        //   Number of instances * number of primitives in instance
        uint32 draw_calls = 0;
        Meshdata::GeometryInstanceIterator geoinst_it = md->getGeometryInstanceIterator();
        uint32 geoinst_idx;
        Matrix4x4f pos_xform;
        while( geoinst_it.next(&geoinst_idx, &pos_xform) )
            draw_calls += md->geometry[ md->instances[geoinst_idx].geometryIndex ].primitives.size();

        // Arbitrary number, but probably more than we should even allow given
        // that there are probably hundreds or thousands of other objects
        if (draw_calls > 500) {
            SILOG(ogre,error,"Excessively complicated mesh: " << mAssetURI.toString() << " has " << draw_calls << " draw calls. Ignoring this mesh.");
            mAsset = Mesh::MeshdataPtr();
            mCB();
            return;
        }
    }

    mRemainingDownloads = md->textures.size();

    // Special case for no dependent downloads
    if (mRemainingDownloads == 0) {
        mCB();
        return;
    }

    String assetURIString = mAssetURI.toString();
    for(TextureList::const_iterator it = md->textures.begin(); it != md->textures.end(); it++) {
        String texURIString = assetURIString.substr(0, assetURIString.rfind("/")+1) + (*it);
        Transfer::URI texURI(texURIString);
        ResourceDownloadTaskPtr dl = ResourceDownloadTask::construct(
            texURI, mScene->transferPool(),
            mPriority,
            std::tr1::bind(&AssetDownloadTask::weakTextureDownloaded, getWeakPtr(), _1, _2)
        );
        mActiveDownloads[texURI] = dl;
        dl->start();
    }
}
void AssetDownloadTask::weakTextureDownloaded(const std::tr1::weak_ptr<AssetDownloadTask>&thus, std::tr1::shared_ptr<ChunkRequest> request, std::tr1::shared_ptr<const DenseData> response) {
    std::tr1::shared_ptr<AssetDownloadTask>locked(thus.lock());
    if (locked) {
        locked->textureDownloaded(request,response);
    }
}
void AssetDownloadTask::textureDownloaded(std::tr1::shared_ptr<ChunkRequest> request, std::tr1::shared_ptr<const DenseData> response) {
    // Clear the download task
    mActiveDownloads.erase(request->getURI());

    // Lack of response data means failure of some sort
    if (!response) {
        failDownload();
        return;
    }

    // Store data for later use
    mDependencies[request->getURI()].request = request;
    mDependencies[request->getURI()].response = response;

    mRemainingDownloads--;
    if (mRemainingDownloads == 0)
        mCB();
}

void AssetDownloadTask::failDownload() {
    // Cancel will stop the current download process.
    cancel();

    // In this case, since it wasn't user requested, we also clear any parsed
    // data (e.g. if we failed on a texture download) and trigger a callback to
    // let them know about the failure.
    mAsset.reset();
    mCB();
}

} // namespace Sirikata
