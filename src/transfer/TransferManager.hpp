/*     Iridium Transfer -- Content Transfer management system
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
 *  * Neither the name of Iridium nor the names of its contributors may
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

#ifndef IRIDIUM_TransferManager_HPP__
#define IRIDIUM_TransferManager_HPP__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

namespace Iridium {
namespace Transfer {

class TransferManager {
	TransferLayer *mFirstTransferLayer;

	struct RequestInfo {
		bool mPending; //pending overlapping requests.

		Range mReqRange;
		boost::function1<void, const FileId&, const SparseDataPtr&> mCallback;
	};
	typedef std::map<FileId, std::list<RequestInfo> > RequestMap;
	RequestMap mActiveRequests;

	boost::mutex mLock;
public:

	bool download(const FileId &name, const boost::function4<void, const URI&,
				const Fingerprint&, const SparseDataPtr&, STATUS>&callback,
				Range range) {
		mLock.acquire();
		// check for overlapping requests.
		// if overlapping, put in "pendingOn"
		// if not,
		bool async = mFirstTransferLayer->getData(name, range, boost::bind(finishedRequest, name, callback));
		if (async) {
			// not instantaneous--we must add it to our mActiveRequests
		}
		mLock.release();
		return async;
	}
};

}
}

#endif /* IRIDIUM_TransferManager_HPP__ */
