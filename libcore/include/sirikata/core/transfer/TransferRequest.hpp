// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_TRANSFER_REQUEST_HPP_
#define _SIRIKATA_CORE_TRANSFER_REQUEST_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/transfer/Defs.hpp>
#include <sirikata/core/transfer/TransferData.hpp>
#include "RemoteFileMetadata.hpp"
#include "URI.hpp"
#include <sirikata/core/transfer/OAuthParams.hpp>

namespace Sirikata {
namespace Transfer {

class TransferRequest;
typedef std::tr1::shared_ptr<TransferRequest> TransferRequestPtr;

/*
 * Base class for a request going into the transfer pool to set
 * the priority of a request
 */
class SIRIKATA_EXPORT TransferRequest {

public:
    typedef std::tr1::function<void()> ExecuteFinished;

	//Return the priority of the request
	inline Priority getPriority() const {
		return mPriority;
	}

	//Returns true if this request is for deletion
	inline bool isDeletionRequest() const {
	    return mDeletionRequest;
	}

	/// Get an identifier for the data referred to by this
	/// TransferRequest. The identifier is not unique for each
	/// TransferRequest. Instead, it identifies the asset data: if two
	/// identifiers are equal, they refer to the same data. (Two different
	/// identifiers may *ultimately* refer to the same data because two
	/// names could point to the same underlying hash).
	virtual const std::string& getIdentifier() const = 0;

	inline const std::string& getClientID() const {
		return mClientID;
	}

	virtual void execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb) = 0;

    virtual void notifyCaller(TransferRequestPtr me, TransferRequestPtr from) = 0;

	virtual ~TransferRequest() {}

	friend class TransferPool;

protected:
	inline void setClientID(const std::string& clientID) {
		mClientID = clientID;
	}

    //Change the priority of the request
    inline void setPriority(Priority p) {
        mPriority = p;
    }

    //Change the priority of the request
    inline void setDeletion() {
        mDeletionRequest = true;
    }

	Priority mPriority;
	std::string mClientID;
	bool mDeletionRequest;

};

class MetadataRequest;
typedef std::tr1::shared_ptr<MetadataRequest> MetadataRequestPtr;
/*
 * Handles requests for metadata of a file when all you have is the URI
 */
class SIRIKATA_EXPORT MetadataRequest: public TransferRequest {

public:
    typedef std::tr1::function<void(
            std::tr1::shared_ptr<MetadataRequest> request,
            std::tr1::shared_ptr<RemoteFileMetadata> response)> MetadataCallback;

    MetadataRequest(const URI &uri, Priority priority, MetadataCallback cb) :
     mURI(uri), mCallback(cb) {
        mPriority = priority;
        mDeletionRequest = false;
        mID = uri.toString();
    }

    inline const std::string &getIdentifier() const {
        return mID;
    }

    inline const URI& getURI() {
        return mURI;
    }

    void execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb);

    inline void notifyCaller(TransferRequestPtr me, TransferRequestPtr from) {
        std::tr1::shared_ptr<MetadataRequest> meC =
            std::tr1::static_pointer_cast<MetadataRequest, TransferRequest>(me);
        std::tr1::shared_ptr<MetadataRequest> fromC =
            std::tr1::static_pointer_cast<MetadataRequest, TransferRequest>(from);
        mCallback(meC, fromC->mRemoteFileMetadata);
    }
    inline void notifyCaller(MetadataRequestPtr me, TransferRequestPtr from, RemoteFileMetadataPtr data) {
        std::tr1::shared_ptr<MetadataRequest> fromC =
                std::tr1::static_pointer_cast<MetadataRequest, TransferRequest>(from);
        mCallback(me, data);
    }

    inline bool operator==(const MetadataRequest& other) const {
        return mID == other.mID;
    }
    inline bool operator<(const MetadataRequest& other) const {
        return mID < other.mID;
    }

    virtual ~MetadataRequest() {}

protected:

    const URI mURI;
    std::string mID;
    MetadataCallback mCallback;
    std::tr1::shared_ptr<RemoteFileMetadata> mRemoteFileMetadata;

    MetadataRequest(const URI &uri, Priority priority) :
        mURI(uri) {
        mPriority = priority;
        mDeletionRequest = false;

        mID = uri.toString();
    }


    inline void execute_finished(std::tr1::shared_ptr<RemoteFileMetadata> response, ExecuteFinished cb) {
        mRemoteFileMetadata = response;
        cb();
        SILOG(transfer, detailed, "done MetadataRequest execute_finished");
    }

};

/*
 * Handles requests for the data associated with a file:chunk pair
 */
class SIRIKATA_EXPORT DirectChunkRequest : public TransferRequest {

public:
    typedef std::tr1::function<void(
            std::tr1::shared_ptr<DirectChunkRequest> request,
            std::tr1::shared_ptr<const DenseData> response)> DirectChunkCallback;

    DirectChunkRequest(const Chunk &chunk, Priority priority, DirectChunkCallback cb)
            : mChunk(std::tr1::shared_ptr<Chunk>(new Chunk(chunk))),
            mCallback(cb)
            {
                mPriority = priority;
                mDeletionRequest = false;
                mID = chunk.toString();
            }

    inline const Chunk& getChunk() {
        return *mChunk;
    }

    inline const std::string &getIdentifier() const {
        return mID;
    }

    void execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb);

    void execute_finished(std::tr1::shared_ptr<const DenseData> response, ExecuteFinished cb);

    void notifyCaller(TransferRequestPtr me, TransferRequestPtr from);
    void notifyCaller(TransferRequestPtr me, TransferRequestPtr from, DenseDataPtr data);

protected:
    std::string mID;
    std::tr1::shared_ptr<Chunk> mChunk;
    DirectChunkCallback mCallback;
    std::tr1::shared_ptr<const DenseData> mDenseData;
};

typedef std::tr1::shared_ptr<DirectChunkRequest> DirectChunkRequestPtr;

/*
 * Handles requests for the data associated with a file:chunk pair
 */
class SIRIKATA_EXPORT ChunkRequest : public MetadataRequest {

public:
    typedef std::tr1::function<void(
            std::tr1::shared_ptr<ChunkRequest> request,
            std::tr1::shared_ptr<const DenseData> response)> ChunkCallback;

	ChunkRequest(const URI &uri, const RemoteFileMetadata &metadata, const Chunk &chunk,
	        Priority priority, ChunkCallback cb)
            : MetadataRequest(uri, priority),
            mMetadata(std::tr1::shared_ptr<RemoteFileMetadata>(new RemoteFileMetadata(metadata))),
            mChunk(std::tr1::shared_ptr<Chunk>(new Chunk(chunk))),
            mCallback(cb)
            {
                mDeletionRequest = false;
                mID = chunk.getHash().toString();
            }

	inline const RemoteFileMetadata& getMetadata() {
		return *mMetadata;
	}

	inline const Chunk& getChunk() {
		return *mChunk;
	}

    void execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb);

    void execute_finished(std::tr1::shared_ptr<const DenseData> response, ExecuteFinished cb);

    void notifyCaller(TransferRequestPtr me, TransferRequestPtr from);
    void notifyCaller(TransferRequestPtr me, TransferRequestPtr from, DenseDataPtr data);

protected:
    std::tr1::shared_ptr<RemoteFileMetadata> mMetadata;
    std::tr1::shared_ptr<Chunk> mChunk;
    std::tr1::shared_ptr<const DenseData> mDenseData;
    ChunkCallback mCallback;
};

typedef std::tr1::shared_ptr<ChunkRequest> ChunkRequestPtr;





class UploadRequest;
typedef std::tr1::shared_ptr<UploadRequest> UploadRequestPtr;
/** Upload requests allow you to upload content to the CDN and
 *  retrieve their URL when the upload is complete.
 */
class SIRIKATA_EXPORT UploadRequest : public TransferRequest {
  public:
    typedef std::map<String, String> StringMap;

    typedef std::tr1::function<void(
        UploadRequestPtr request,
        const Transfer::URI& path)> UploadCallback;

    UploadRequest(OAuthParamsPtr oauth,
        const StringMap& files, const String& path, const StringMap& params,
        Priority priority,
        UploadCallback cb)
        : mOAuth(oauth),
          mFiles(files),
          mPath(path),
          mParams(params),
          mCB(cb)
    {
    }
    virtual ~UploadRequest() {}

    // TransferRequest Interface
    virtual const std::string& getIdentifier() const;
    virtual void execute(TransferRequestPtr req, ExecuteFinished cb);
    virtual void notifyCaller(TransferRequestPtr me, TransferRequestPtr from);

    OAuthParamsPtr oauth() { return mOAuth; }
    const StringMap& files() { return mFiles; }
    const String& path() { return mPath; }
    const StringMap& params() { return mParams; }

  private:

    void execute_finished(Transfer::URI uploaded_path, ExecuteFinished cb);

    const OAuthParamsPtr mOAuth;
    const StringMap mFiles; // Filename -> data
    const String mPath; // Requested location of asset,
                        // e.g. apiupload/filename results in
                        // /username/apiupload/filename
    const StringMap mParams; // Optional params, e.g. title = 'Building'

    // The URI the file was actually uploaded to, or empty if it failed
    Transfer::URI mUploadedPath;

    UploadCallback mCB;
};

} // namespace Transfer
} // namespace Sirikata

#endif //_SIRIKATA_CORE_TRANSFER_REQUEST_HPP_
