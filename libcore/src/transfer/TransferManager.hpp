/*  Sirikata Transfer -- Content Transfer management system
 *  TransferManager.hpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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
/*  Created on: Jan 1, 2009 */

#ifndef SIRIKATA_TransferManager_HPP__
#define SIRIKATA_TransferManager_HPP__

#include "task/Event.hpp"
#include "URI.hpp"
#include "TransferData.hpp"
#include "task/EventManager.hpp" // for EventListener
#include "task/UniqueId.hpp"

namespace Sirikata {
namespace Transfer {

/// You can subscribe to this event to receive notifications of all completed downloads.
static const char *DownloadEventId = "DownloadFinished";
static const char *UploadEventId = "UploadFinished";
static const char *UploadDataEventId = "UploadDataFinished";

/** Manages requests going into the cache.
 *
 * @see DownloadManager
 * @see TransferLayer
 * @see CacheLayer
 */
class TransferManager {
public:
	typedef Task::GenEventManager::EventListener EventListener;
	typedef Task::SubscriptionId SubscriptionId;
        typedef Task::SubscriptionIdClass SubscriptionIdClass;

	/** Very basic status messages--more detailed (permanent/network/temporary)
	 * statuses should be added to allow for better error handling.
	 */
	enum Status {SUCCESS, FAIL_UNIMPLEMENTED, FAIL_NAMELOOKUP, FAIL_DOWNLOAD, FAIL_NAMEUPLOAD, FAIL_UPLOAD, FAIL_AUTH, FAIL_SHUTDOWN};

	/// An Event that will be fired to all subscribers of DownloadEventId.
	class DownloadEvent : public Task::Event {
		const RemoteFileId mFileId;
		const SparseData mData;
		const Status mStatus;

	public:
		/** Gets the event ID that this event will be subscribed to.
		 *
		 * @returns IdPair(DownloadEventId, fileid.fingerprint().convertToHexString())
		 */
		static Task::IdPair getIdPair(const RemoteFileId &fileId) {
			return Task::IdPair(DownloadEventId, fileId.fingerprint().convertToHexString());
		}
		/// Constructor: fileId may be RemoteFileId() if unknown, data may be NULL.
		DownloadEvent(Status stat, const RemoteFileId &fileId, const SparseData *data)
			: Task::Event(getIdPair(fileId)),
				mFileId(fileId), mData(data?(*data):SparseData()), mStatus(stat) {
		}

		/// @returns fingerprint of the downloaded file.
		inline const Fingerprint &fingerprint() const {
			return mFileId.fingerprint();
		}
		/// @returns the URI of the hashed file (mhash:///...), NOT the named file.
		inline const URI &uri() const {
			return mFileId.uri();
		}
                /// @returns The RemoteFileId object.
                inline const RemoteFileId &getRemoteFileId() const {
                        return mFileId;
                }

		/** Gets at least the retrieved SparseData.
		 *
		 * Note that commonly, more is available in the cache than what you asked for,
		 * so be prepared to handle extra data.
		 *
		 * In addition, this event may fire
		 * if someone else asked for a smaller chunk of the file than you (i.e. if you
		 * asked for the whole file, but someone else just the header).
		 * Be prepared to handle this case-- to check, you can call
		 * myRequestedRange.isContainedBy(data()). If you did not get what you want,
		 * you are guaranteed to either get another event call, or else an event with
		 * success() equal to false.
		 *
		 * data().empty() mey return true if the transfer failed, if the Range passed
		 * to download() was the empty range Range(false), or if the file is empty.
		 *
		 * To check if you downloaded the empty file, you can compare fingerprint()
		 * with Fingerprint::emptyDigest()
		 *
		 * @todo  Clean up exactly what data will be returned, and when it is safe to
		 * ignore the event and remain subscribed to a given file without leaking.
		 */
		inline const SparseData &data() const {
			return mData;
		}
		/// Checks for a successful transfer. For more detail, call getStatus().
		inline bool success() const {
			return mStatus == SUCCESS;
		}
		/// @returns SUCCESS, or any failure value in the enumeration.
		inline Status getStatus() const {
			return mStatus;
		}
		virtual ~DownloadEvent() {}
	};
	typedef std::tr1::shared_ptr<DownloadEvent> DownloadEventPtr;

	class UploadEvent: public Task::Event {
		Status mStatus;
		URI mURI;
	public:
		static Task::IdPair getIdPair(const URI &uploadURI, const char *id) {
			return Task::IdPair(id, uploadURI.toString());
		}
		UploadEvent(Status stat, const URI &uploadURI, const char *id) :
			Task::Event(getIdPair(uploadURI, id)),
			mStatus(stat),
			mURI(uploadURI) {
		}
		/// Checks for a successful transfer. For more detail, call getStatus().
		inline bool success() const {
			return mStatus == SUCCESS;
		}
		/// @returns SUCCESS, or any failure value in the enumeration.
		inline Status getStatus() const {
			return mStatus;
		}
		/// @returns the URI of the hashed file (mhash:///...), NOT the named file.
		inline const URI &uri() const {
			return mURI;
		}
		virtual ~UploadEvent() {}
	};
	typedef std::tr1::shared_ptr<UploadEvent> UploadEventPtr;

public:

	/**
	 * Virtual destructor required for cleanup.
	 *
	 * To cleanup: <ul>
	 * <li>First call cleanup(), then
	 * <li>Then, delete the NameLookupManager and all CacheLayer's. <br/>
	 *   (these will synchronously ensure that all pending requests
	 *    call their respective callbacks.)
	 * <li>Finally, delete this.
	 * </ul>
	 */
	virtual ~TransferManager() {
	}

	/// Do not accept any new requests.
	virtual void cleanup() {
	}

	/** For debugging use only. Takes a Fingerprint, not a URI,
	 * and the content of a hashed file will never change. */
	virtual void purgeFromCache(const Fingerprint &fprint) {
	}

	/** Performs a name lookup, and then downloads this file.
	 *
	 * Uses the event system in order to ensure that multiple copies of a duplicate file
	 * are not being downloaded at the same time.
	 *
	 * @param name      URI of the named file (e.g. meerkat:///somefile.texture)
	 * @param listener  An EventListener to receive a DownloadEventPtr with the retrieved data.
	 * @param range     What part of the file to retrieve, or Range(true) for the whole file.
	 */
	///
	virtual void download(const URI &name, const EventListener &listener, const Range &range) {
		listener(DownloadEventPtr(new DownloadEvent(FAIL_UNIMPLEMENTED, RemoteFileId(), NULL)));
	}

	/** Downloads a file by hash. Almost equivalent to CacheLayer::getData,
	 * but uses the event system in order to ensure that multiple copies of a duplicate file
	 * are not being downloaded at the same time.
	 *
	 * @param name      RemoteFileId of the hash and download URI (e.g. mhash:///1234567890abcdef...)
	 * @param listener  An EventListener to receive a DownloadEventPtr with the retrieved data.
	 * @param range     What part of the file to retrieve, or Range(true) for the whole file.
	 */
	virtual SubscriptionId downloadByHash(const RemoteFileId &name, const EventListener &listener, const Range &range) {
		listener(DownloadEventPtr(new DownloadEvent(FAIL_UNIMPLEMENTED, name, NULL)));
		return SubscriptionIdClass::null();
	}

        virtual void downloadName(const URI &nameURI,
                const std::tr1::function<void(const URI &nameURI,const RemoteFileId *fingerprint)> &listener) {
            listener(nameURI, NULL);
        }

        bool isNameURI(const URI &check) const {
                std::string::size_type length = check.proto().length();
                if (length < 4) {
                        return false;
                }
                return check.proto().substr(length-4,4) == "hash";
        }


	/// Like the other upload() function, but avoids recomputing the hash.
	virtual void upload(const URI &name,
			const RemoteFileId &hash,
			const DenseDataPtr &toUpload,
			const EventListener &listener,
			bool forceIfExists) {
		listener(UploadEventPtr(new UploadEvent(FAIL_UNIMPLEMENTED, name, UploadEventId)));
	}

	/** Uploads a filename, and if necessary, uploads the data as well.
	 *
	 * @param name         The named file to be uploaded (of the form "meerkat:///somefile.texture")
	 * @param hashContext  The location to upload the data to (usually "mhash:")
	 * @param toUpload     Data to be uploaded. Will only be uploaded if necessary.
	 * @param listener     An EventListener to receive a UploadEventPtr with the retrieved data.
	 * @param forceIfExists Upload the data even if the file already exists on all known CDN services.
	 */
	inline void upload(const URI &name,
			const URIContext &hashContext,
			const DenseDataPtr &toUpload,
			const EventListener &listener,
			bool forceIfExists) {
		RemoteFileId rfid(
			Fingerprint::computeDigest(toUpload->data(), toUpload->length()),
			hashContext);
		upload(name, rfid, toUpload, listener, forceIfExists);
	}

	/**
	 * Like upload(), but does not check if the referenced data exists.
	 *
	 * Similar to calling:
	 * NameUploadHandler::uploadName(NULL, service.lookup(name.proto()), name, hash, listener)
	 *
	 * @param name     The named file to be uploaded (of the form "meerkat:///somefile.texture")
	 * @param hash     The precomputed hash of the referenced file.
	 * @param listener An EventListener to receive a UploadEventPtr with the retrieved data.
	 */
	virtual void uploadName(const URI &name,
			const RemoteFileId &hash,
			const EventListener &listener) {
		listener(UploadEventPtr(new UploadEvent(FAIL_UNIMPLEMENTED, name, UploadEventId)));
	}

	/**
	 * Like upload(), but does not allocate a name for this object.
	 *
	 * Similar to calling:
	 * UploadHandler::upload(NULL, service.lookup(hash.url().proto()), hash.url(), toUpload, listener)
	 *
	 * Note: uploadByHash checks for the existence of the file, and uploads only if it is
	 * missing from at least one service. The forceIfExists option will override this check.
	 *
	 * @param hash      The hash of the file upload.
	 * @param toUpload  SparseData to upload (can be cast from DenseData).
	 * @param listener  A listener to receive the UploadEvent.
	 * @param forceIfExists Upload the data even if the file already exists on all known CDN services.
	 */
	virtual void uploadByHash(const RemoteFileId &hash,
			const DenseDataPtr &toUpload,
			const EventListener &listener,
			bool forceIfExists) {
		listener(UploadEventPtr(new UploadEvent(FAIL_UNIMPLEMENTED, hash.uri(), UploadDataEventId)));
	}
	/** Like the other uploadByHash() function, but computes the hash.
	 *
	 * @param hashContext  The URIContext to upload the hash to (e.g. "mhash:")
	 */
	inline void uploadByHash(const URIContext &hashContext,
			const DenseDataPtr &toUpload,
			const EventListener &listener,
			bool forceIfExists) {
		RemoteFileId rfid(
			Fingerprint::computeDigest(toUpload->data(), toUpload->length()),
			hashContext);
		uploadByHash(rfid, toUpload, listener, forceIfExists);
	}
};

typedef TransferManager::DownloadEvent DownloadEvent;
typedef TransferManager::DownloadEventPtr DownloadEventPtr;

typedef TransferManager::UploadEvent UploadEvent;
typedef TransferManager::UploadEventPtr UploadEventPtr;

}
}

#endif /* SIRIKATA_TransferManager_HPP__ */
