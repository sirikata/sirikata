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

namespace Sirikata {
namespace Transfer {

static const char *DownloadEventId = "DownloadFinished";

/** Manages requests going into the cache.
 *
 * @see DownloadManager
 * @see TransferLayer
 * @see CacheLayer
 */
class TransferManager {
public:
	typedef Task::GenEventManager::EventListener EventListener;

	enum Status {SUCCESS, FAIL_UNIMPLEMENTED, FAIL_NAMELOOKUP, FAIL_DOWNLOAD};

	struct DownloadEvent : public Task::Event {

		const RemoteFileId mFileId;
		const SparseData mData;
		const Status mStatus;

		static Task::IdPair getIdPair(const RemoteFileId &fileId) {
			return Task::IdPair(DownloadEventId, fileId.fingerprint().convertToHexString());
		}
		DownloadEvent(Status stat, const RemoteFileId &fileId, const SparseData *data)
			: Task::Event(getIdPair(fileId)),
				mFileId(fileId), mData(data?(*data):SparseData()), mStatus(stat) {
		}

		const Fingerprint &fingerprint() const {
			return mFileId.fingerprint();
		}
		const URI &uri() const {
			return mFileId.uri();
		}
		const SparseData &data() const {
			return mData;
		}
		bool success() const {
			return mStatus == SUCCESS;
		}
	};
	typedef boost::shared_ptr<DownloadEvent> DownloadEventPtr;

public:

	// TODO: Handle multiple name lookups at the same time to the same filename. Is this possible? worth doing?

	virtual void purgeFromCache(const Fingerprint &fprint) {
	}

	virtual void download(const URI &name, const EventListener &listener, const Range &range) {
		listener(DownloadEventPtr(new DownloadEvent(FAIL_UNIMPLEMENTED, RemoteFileId(), NULL)));
	}

	virtual void downloadByHash(const RemoteFileId &name, const EventListener &listener, const Range &range) {
		listener(DownloadEventPtr(new DownloadEvent(FAIL_UNIMPLEMENTED, RemoteFileId(), NULL)));
	}

	/// Not yet implemented -- may be moved outside of TransferManager.
	virtual bool upload(const URI &name, const SparseData &toUpload) {
		return false;
	}
};

typedef TransferManager::DownloadEvent DownloadEvent;
typedef TransferManager::DownloadEventPtr DownloadEventPtr;

}
}

#endif /* SIRIKATA_TransferManager_HPP__ */
