/*     Iridium Transfer -- Content Transfer management system
 *  DiskCache.hpp
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

#ifndef IRIDIUM_DiskCache_HPP__
#define IRIDIUM_DiskCache_HPP__

#include "CacheLayer.hpp"
#include "util/ThreadSafeQueue.hpp"

namespace Iridium {
namespace Transfer {

// should really be a config option.
//#define NUM_WORKER_THREADS 10

/// Disk Cache keeps track of what files are on disk, and manages a helper thread to retrieve it.
class DiskCache : CacheLayer {
public:
	typedef std::list<Range> CacheData;

private:
	typedef CacheMap<CacheData> DiskMap;
	DiskMap mFiles;

	struct DiskRequest;
	boost::thread mWorkerThread;
	ThreadSafeQueue<DiskReqeust> mRequestQueue;

	struct DiskRequest {
		URI fileURI;
		Range toRead;
		TransferCallback finished;
		boost::shared_ptr<DenseData> data; // if NULL, read data.
	};

	void workerThread() {
		boost::shared_ptr<DiskRequest> req;
		while (true) {
			req = mRequestQueue.blockingPop();
			if (req->data) {
				// Note: TransferLayer::populatePreviousCaches has already been called.
				FILE *fp;
				std::string fileId = fileURI.fingerprint();
				if (!mFiles->alloc(req->mRange.mLength)) {
					continue;
				}

				fp = fopen(fileId.c_str(), "wb");
				if (!fp) {
					std::cerr << "Failed to open " << fileId <<
						"for writing; reason: " << errno << std::endl;
					continue;
				}
				fseek(req->data->mRange.mStart);
				fwrite(req->data->mData.data(), 1,
						req->data->mRange.mLength, fp);
				fclose(fp);

				{
					DiskMap::write_iterator writer(mFiles);

					writer.insert(req->fileURI.fingerprint(), FileInfo());
					(*writer).push_back(mRange);
					writer->use();
				}
			} else {
				// check that we still have this file..........?
				std::string fileId = fileURI.fingerprint();
				FILE *fp = fopen(fileId.c_str(), "rb");
				if (!fp) {
					std::cerr << "Failed to open " << fileId <<
						"for reading; reason: " << errno << std::endl;
					TransferLayer::getData(req->fileURI, req->toRead, req->finished);
					continue;
				}
				fseek(req->toRead.mStart);
				DenseDataPtr datum(new DenseData(
						req->toRead.mStart,
						req->toRead.mLength));
				fread(&(*datum.mData.begin()), 1,
						req->toRead.mLength, fp);
				fclose(fp);

				TransferLayer::populatePreviousCaches(req->fileId, datum);
				req->finished(datum);
			}
		}
	}

	void readDataFromDisk(const URI &fileURI,
			const Range &requestedRange,
			const TransferCallback&callback) {
		DiskRequest *req = new DiskRequest();
		req->fileURI = fileURI;
		req->finished = callback;
		req->toRead = requestedRange;

		mRequestQueue.push(req);
	}

protected:
	virtual void populateCache(const Fingerprint& fileId, const DenseDataPtr &data) {
		DiskRequest *req = new DiskRequest();
		req->fileId = fileId;
		req->data = data;

		mRequestQueue.push(req);

		TransferLayer::populatePreviousCaches(req->fileId, datum);
	}

public:

	DiskCache(CachePolicy<CacheData> *policy, TransferManager *cacheMgr, CacheLayer *respondTo, TransferLayer *tryNext)
			: CacheLayer(cacheMgr, respondTo, tryNext),
			mWorkerThread(boost::bind(&DiskCache::workerThread, this)),
			mFiles(policy) {
	}

	virtual bool getData(const URI &fileId,
			const Range &requestedRange,
			const TransferCallback&callback) {
		bool haveRange = false;
		{
			DiskMap::read_iterator iter(mFiles);

			if (iter.find(fileId.fingerprint())) {
				haveRange = requestedRange.isContainedBy(*iter);
			}
			if (haveRange) {
				iter->use(); // or is it more proper to use() after reading from disk?
			}
		}
		if (haveRange) {
			readDataFromDisk(fileId, requestedRange, callback);
		} else {
			TransferLayer::getData(fileId, requestedRange, callback);
		}
	}
};

}
}

#endif /* IRIDIUM_DiskCache_HPP__ */
