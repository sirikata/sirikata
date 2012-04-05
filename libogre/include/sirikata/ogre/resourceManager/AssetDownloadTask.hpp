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

#ifndef _SIRIKATA_OGRE_ASSET_DOWNLOAD_TASK_HPP_
#define _SIRIKATA_OGRE_ASSET_DOWNLOAD_TASK_HPP_

#include <sirikata/core/transfer/ResourceDownloadTask.hpp>
#include <sirikata/mesh/Visual.hpp>

namespace Sirikata {
namespace Graphics {
class OgreRenderer;

class ParseMeshTaskInfo;
typedef std::tr1::shared_ptr<ParseMeshTaskInfo> ParseMeshTaskHandle;
}

/** An AssetDownloadTask manages the full download of an asset, including
 *  dependencies. It builds upon the basic functionality of
 *  ResourceDownloadTask.
 *
 *  Eventually this class should handle progressive downloads, letting you know
 *  when some minimum loadable unit is available, and then providing additional
 *  callbacks as more data becomes available. Currently, it just manages
 *  downloading all dependencies and notifying you when the are complete.
 */
class AssetDownloadTask : public SelfWeakPtr<AssetDownloadTask>{
public:
    typedef std::tr1::function<void()> FinishedCallback;

    struct ResourceData {
        Transfer::TransferRequestPtr request;
        Transfer::DenseDataPtr response;
    };
    typedef std::map<Transfer::URI, ResourceData> Dependencies;
private:
    AssetDownloadTask(const Transfer::URI& uri, Graphics::OgreRenderer* const scene, double priority, bool isAgg, FinishedCallback cb);
public:
    static std::tr1::shared_ptr<AssetDownloadTask> construct(const Transfer::URI& uri, Graphics::OgreRenderer* const scene, double priority, FinishedCallback cb);
    static std::tr1::shared_ptr<AssetDownloadTask> construct(const Transfer::URI& uri, Graphics::OgreRenderer* const scene, double priority, bool isAgg, FinishedCallback cb);

    ~AssetDownloadTask();

    Mesh::VisualPtr asset() const { return mAsset; }
    const Dependencies& dependencies() const { return mDependencies; }
    float64 priority() const { return mPriority; }

    void getDownloadTasks(
        std::vector<String>& finishedDownloads, std::vector<String>& activeDownloads);


    void updatePriority(float64 priority);
    void cancel();

    typedef std::map<const String, Transfer::ResourceDownloadTaskPtr> ActiveDownloadMap;

private:

    void downloadAssetFile();
    static void weakAssetFileDownloaded(std::tr1::weak_ptr<AssetDownloadTask> thus, Transfer::ResourceDownloadTaskPtr taskptr,
            Transfer::TransferRequestPtr request, Transfer::DenseDataPtr response);
    void assetFileDownloaded(Transfer::ResourceDownloadTaskPtr taskptr, Transfer::ChunkRequestPtr request, Transfer::DenseDataPtr response);
    static void weakHandleAssetParsed(std::tr1::weak_ptr<AssetDownloadTask> thus, Mesh::VisualPtr md);
    void handleAssetParsed(Mesh::VisualPtr md);

    void addDependentDownload(Transfer::ResourceDownloadTaskPtr resPtr);
    void addDependentDownload(const Transfer::URI& depUrl);
    void addDependentDownload(const Transfer::URI& depUrl, const Transfer::Chunk& depChunk);

    // Start is separate from add so we can be sure mActiveDownloads isn't being
    // modified after we start the downloads, except by the completion handler
    void startDependentDownloads();

    static void weakTextureDownloaded(const std::tr1::weak_ptr<AssetDownloadTask>&, Transfer::URI uri, Transfer::ResourceDownloadTaskPtr taskptr,
            Transfer::TransferRequestPtr request, Transfer::DenseDataPtr response);

    void textureDownloaded(Transfer::URI uri, Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr request,
            Transfer::DenseDataPtr response);


    // Fails the entire process as a result of one dependency (or the original
    // asset) failing to download.
    void failDownload();

    void cancelNoLock();

    // Get the URL for an asset, deciding automatically whether it
    // needs to be relative or absolute
    Transfer::URI getURL(const Transfer::URI& orig, const String& given_url);

    Graphics::OgreRenderer *const mScene;
    Transfer::URI mAssetURI;
    double mPriority;
    FinishedCallback mCB;

    Graphics::ParseMeshTaskHandle mParseMeshHandle;
    Mesh::VisualPtr mAsset;
    Dependencies mDependencies;
    bool mIsAggregate;

    // Active downloads, for making sure shared_ptrs stick around and for cancelling
    ActiveDownloadMap mActiveDownloads;
    std::vector<String> mFinishedDownloads;

    boost::mutex mDependentDownloadMutex;

public:

    ActiveDownloadMap::size_type getOutstandingDependentDownloads();


};
typedef std::tr1::shared_ptr<AssetDownloadTask> AssetDownloadTaskPtr;

} // namespace Sirikata

#endif //_SIRIKATA_OGRE_ASSET_DOWNLOAD_TASK_HPP_
