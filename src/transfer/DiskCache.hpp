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

#include <sys/stat.h>
#include <sys/types.h>

#include "CacheMap.hpp"
#include "util/ThreadSafeQueue.hpp"

namespace Iridium {
namespace Transfer {

// should really be a config option.
//#define NUM_WORKER_THREADS 10

/// Disk Cache keeps track of what files are on disk, and manages a helper thread to retrieve it.
class DiskCache : public CacheLayer {
public:
	typedef RangeList CacheData;

private:
	boost::thread mWorkerThread;

	typedef CacheMap DiskMap; //<CacheData> DiskMap;
	DiskMap mFiles;

	struct DiskRequest;
	ThreadSafeQueue<DiskRequest*> mRequestQueue;

	struct DiskRequest {
		DiskRequest(URI myURI) :fileURI(myURI) {}

		URI fileURI;
		Range toRead;
		TransferCallback finished;
		boost::shared_ptr<DenseData> data; // if NULL, read data.
	};
public:
	void workerThread() {
		while (true) {
			boost::shared_ptr<DiskRequest> req (mRequestQueue.blockingPop());
			if (req->data) {
				// Note: TransferLayer::populatePreviousCaches has already been called.
				FILE *fp;
				std::string fileId = req->fileURI.fingerprint().convertToHexString();
				if (!mFiles.alloc(req->data->mRange.mLength)) {
					continue;
				}

				fp = fopen(fileId.c_str(), "wb");
				if (!fp) {
					std::cerr << "Failed to open " << fileId <<
						"for writing; reason: " << errno << std::endl;
					continue;
				}
				// FIXME: may not work with 64-bit files?
				fseek(fp, req->data->mRange.mStart, SEEK_SET);
				fwrite(req->data->mData.data(), 1,
						req->data->mRange.mLength, fp);
				fflush(fp);
				size_t diskUsage;
				{
					struct stat st;
					fstat(fileno(fp), &st);
#ifdef _WIN32
					diskUsage = st.st_size;
#else
					// http://lkml.indiana.edu/hypermail/linux/kernel/9812.2/0216.html
					diskUsage = 512 * st.st_blocks;
#endif
				}
				fclose(fp);

				{
					DiskMap::write_iterator writer(mFiles);

					if (writer.insert(req->fileURI.fingerprint(), new CacheData, diskUsage)) {
						writer.use();
					} else {
						writer.update(diskUsage);
					}
					RangeList *data = writer;
					data->push_back(req->data->mRange);
				}
			} else {
				// check that we still have this file..........?
				std::string fileId = req->fileURI.fingerprint().convertToHexString();
				FILE *fp = fopen(fileId.c_str(), "rb");
				if (!fp) {
					std::cerr << "Failed to open " << fileId <<
						"for reading; reason: " << errno << std::endl;
					CacheLayer::getData(req->fileURI, req->toRead, req->finished);
					continue;
				}
				// FIXME: may not work with 64-bit files?
				fseek(fp, req->toRead.mStart, SEEK_SET);
				DenseDataPtr datum(new DenseData(Range(
						req->toRead.mStart,
						req->toRead.mLength)));
				fread(&(*datum->mData.begin()), 1,
						req->toRead.mLength, fp);
				fclose(fp);

				CacheLayer::populateParentCaches(req->fileURI.fingerprint(), datum);
				SparseData data;
				data.addValidData(datum);
				req->finished(&data);
			}
		}
	}

	void readDataFromDisk(const URI &fileURI,
			const Range &requestedRange,
			const TransferCallback&callback) {
		DiskRequest *req = new DiskRequest(fileURI);
		req->finished = callback;
		req->toRead = requestedRange;

		mRequestQueue.push(req);
	}

protected:
	virtual void populateCache(const Fingerprint& fileId, const DenseDataPtr &data) {
		DiskRequest *req = new DiskRequest(URI(fileId, ""));
		req->data = data;

		mRequestQueue.push(req);

		CacheLayer::populateParentCaches(req->fileURI.fingerprint(), data);
	}

	virtual void destroyCacheEntry(const Fingerprint &fileId,  void *cacheLayerData) {
		unlink(fileId.convertToHexString().c_str());
		CacheData *toDelete = (CacheData*)cacheLayerData;
		delete toDelete;
	}

public:

	DiskCache(CachePolicy *policy, TransferManager *cacheMgr, CacheLayer *respondTo, CacheLayer *tryNext)
			: CacheLayer(cacheMgr, respondTo, tryNext),
			mWorkerThread(boost::bind(&DiskCache::workerThread, this)),
			mFiles(this, policy) {
	}

	virtual bool getData(const URI &fileId,
			const Range &requestedRange,
			const TransferCallback&callback) {
		bool haveRange = false;
		{
			DiskMap::read_iterator iter(mFiles);

			if (iter.find(fileId.fingerprint())) {
				const RangeList *list = iter;
				haveRange = requestedRange.isContainedBy(*list);
			}
			if (haveRange) {
				iter.use(); // or is it more proper to use() after reading from disk?
			}
		}
		if (haveRange) {
			readDataFromDisk(fileId, requestedRange, callback);
			return true;
		} else {
			return CacheLayer::getData(fileId, requestedRange, callback);
		}
	}
};

}
}

#endif /* IRIDIUM_DiskCache_HPP__ */
