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

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>

#include "CacheMap.hpp"
#include "util/ThreadSafeQueue.hpp"

namespace Iridium {
namespace Transfer {

// should really be a config option.
//#define NUM_WORKER_THREADS 10

/// Disk Cache keeps track of what files are on disk, and manages a helper thread to retrieve it.
class DiskCache : public CacheLayer {
public:
	struct CacheData : public CacheEntry {
		RangeList mRanges;
	};

private:
	struct DiskRequest;
	ThreadSafeQueue<boost::shared_ptr<DiskRequest> > mRequestQueue; // must be initialized before the thread.
	boost::thread mWorkerThread;

	CacheMap mFiles;


	std::string mPrefix; // directory or prefix name with trailing slash.

	struct DiskRequest {
		enum Operation {READ, WRITE, DELETE, EXIT} op;

		DiskRequest(Operation op, const URI &myURI, const Range &myRange)
			:op(op), fileURI(myURI), toRead(myRange) {}

		URI fileURI;
		Range toRead;
		TransferCallback finished;
		boost::shared_ptr<DenseData> data; // if NULL, read data.

	};

	boost::mutex destroyLock;
	boost::condition_variable destroyCV;
	bool mCleaningUp; // do not delete any files.

	static cache_usize_type getDiskUsage(const struct stat *st) {
#ifdef _WIN32
		return (cache_usize_type)st->st_size;
#else
		// http://lkml.indiana.edu/hypermail/linux/kernel/9812.2/0216.html
		return 512 * (cache_usize_type)st->st_blocks;
#endif
	}
public:
	void workerThread() {
		while (true) {
			boost::shared_ptr<DiskRequest> req;

            mRequestQueue.blockingPop(req);
			if (req->op == DiskRequest::EXIT) {
				break;
			} else if (req->op == DiskRequest::WRITE) {
				// Note: TransferLayer::populatePreviousCaches has already been called.
				std::string fileId = req->fileURI.fingerprint().convertToHexString();
				{
					CacheMap::write_iterator writer(mFiles);
					if (!mFiles.alloc(req->data->length(), writer)) {
						continue;
					}
					if (writer.find(req->fileURI.fingerprint())) {
						RangeList &rlist = static_cast<CacheData*>(*writer)->mRanges;
						if (req->data->isContainedBy(rlist)) {
							// this range is already written to disk.
							continue;
						}
					}
				}

				std::string filePath = mPrefix + fileId;
				/////////////// FIXME: Sanatize fingerprint before opening files.
				FILE *fp = fopen(filePath.c_str(), "wb");
				if (!fp) {
					std::cerr << "Failed to open " << fileId <<
						"for writing; reason: " << errno << std::endl;
					continue;
				}
				// FIXME: may not work with 64-bit files?
				fseek(fp, req->data->startbyte(), SEEK_SET);
				fwrite(req->data->writableData(), 1,
						req->data->length(), fp);
				fflush(fp);
				cache_usize_type diskUsage;
				{
					struct stat st;
					fstat(fileno(fp), &st);
					diskUsage = getDiskUsage(&st);
				}
				fclose(fp);

				{
					CacheMap::write_iterator writer(mFiles);

					if (writer.insert(req->fileURI.fingerprint(), diskUsage)) {
						*writer = new CacheData;
						writer.use();
					} else {
						writer.update(diskUsage);
					}
					RangeList &data = static_cast<CacheData*>(*writer)->mRanges;
					req->data->addToList(*(req->data), data);
				}
			} else if (req->op == DiskRequest::READ) {
				// check that we still have this file..........?
				std::string fileId = req->fileURI.fingerprint().convertToHexString();
				std::string filePath = mPrefix + fileId;
				/////////////// FIXME: Sanatize fingerprint before opening files.
				FILE *fp = fopen(filePath.c_str(), "rb");
				if (!fp) {
					std::cerr << "Failed to open " << fileId <<
						"for reading; reason: " << errno << std::endl;
					CacheLayer::getData(req->fileURI, req->toRead, req->finished);
					continue;
				}
				if (req->toRead.goesToEndOfFile()) {
					struct stat st;
					fstat(fileno(fp), &st);
					req->toRead.setLength(st.st_size, true);
				}
				// FIXME: may not work with 64-bit files?
				fseek(fp, req->toRead.startbyte(), SEEK_SET);
				DenseDataPtr datum(new DenseData(req->toRead));
				fread(datum->writableData(), 1,
						req->toRead.length(), fp);
				fclose(fp);

				CacheLayer::populateParentCaches(req->fileURI.fingerprint(), datum);
				SparseData data;
				data.addValidData(datum);
				req->finished(&data);
			} else if (req->op == DiskRequest::DELETE) {
				std::string fileId = req->fileURI.fingerprint().convertToHexString();
				std::string filePath = mPrefix + fileId;
				/////////////// FIXME: Sanatize fingerprint before opening files.
				unlink(filePath.c_str());
			}
		}
		{
			boost::unique_lock<boost::mutex> wake_cv(destroyLock);
			destroyCV.notify_one();
		}
	}

	void readDataFromDisk(const URI &fileURI,
			const Range &requestedRange,
			const TransferCallback&callback) {
		boost::shared_ptr<DiskRequest> req (
				new DiskRequest(DiskRequest::READ, fileURI, requestedRange));
		req->finished = callback;

		mRequestQueue.push(req);
	}

	void serialize() {
		std::string toOpen = mPrefix + "index.txt";
		std::fstream fp (toOpen.c_str(), std::ios_base::out);

		CacheMap::read_iterator iter (mFiles);
		while (iter.iterate()) {
			std::string id = iter.getId().convertToHexString();
			fp << id << " " << iter.getSize() << ": ";
			const RangeList &list = static_cast<const CacheData*>(*iter)->mRanges;
			for (RangeList::const_iterator liter = list.begin(); liter != list.end(); ++liter) {
				cache_ssize_type len = (cache_ssize_type)((*liter).length());
				if ((*liter).goesToEndOfFile()) {
					len = -len;
				}
				fp << (*liter).startbyte() << " " << len << "; ";
			}
			fp << std::endl;
		}
		fp.close();
	}

	void unserialize() {
		std::string toOpen = mPrefix + "index.txt";
		std::fstream fp (toOpen.c_str(), std::ios_base::in);

		CacheMap::write_iterator writer (mFiles);
		while (fp.good()) {
			std::string theline;
			std::getline (fp, theline);
			std::istringstream iranges(theline);

			std::string fingerprint;
			cache_usize_type totalLength;

			char c;
			iranges >> fingerprint >> totalLength >> c;
			if (fingerprint.empty()) {
				continue;
			}

			std::cout << "Cached fingerprint" << c << " " << fingerprint <<
				"(" << totalLength << ")" << std::endl;

			if (!writer.insert(SHA256::convertFromHex(fingerprint), totalLength)) {
				continue;
			}
			CacheData *cdata = new CacheData();
			RangeList *rlist = &cdata->mRanges;
			while (iranges.good()) {
				Range::base_type start = 0;
				cache_ssize_type length = 0;
				bool toEndOfFile = false;
				iranges >> start >> length;

				if (length == 0) {
					continue;
				}
				if (length < 0) {
					length = -length;
					toEndOfFile = true;
				}

				Range toAdd (start, length, LENGTH, toEndOfFile);

				toAdd.addToList(toAdd, *rlist);

				char c;
				iranges >> c;
			}
			*writer = cdata;
		}
		fp.close();
	}

protected:
	virtual void populateCache(const Fingerprint& fileId, const DenseDataPtr &data) {
		boost::shared_ptr<DiskRequest> req (
				new DiskRequest(DiskRequest::WRITE, URI(fileId, ""), *data));
		req->data = data;

		mRequestQueue.push(req);

		CacheLayer::populateParentCaches(req->fileURI.fingerprint(), data);
	}

	virtual void destroyCacheEntry(const Fingerprint &fileId, CacheEntry *cacheLayerData, cache_usize_type releaseSize) {
		if (!mCleaningUp) {
			// don't want to erase the disk cache when exiting the program.
			std::string fileName = fileId.convertToHexString();
			boost::shared_ptr<DiskRequest> req
				(new DiskRequest(DiskRequest::DELETE, URI(fileId, ""), Range(true)));
		}
		CacheData *toDelete = static_cast<CacheData*>(cacheLayerData);
		delete toDelete;
	}

public:

	DiskCache(CachePolicy *policy, const std::string &prefix, CacheLayer *tryNext)
			: CacheLayer(tryNext),
			mWorkerThread(boost::bind(&DiskCache::workerThread, this)),
			mFiles(this, policy),
			mPrefix(prefix),
			mCleaningUp(false) {

		std::string::size_type slash=0;
		while (true) {
			std::string::size_type fwdslash = mPrefix.find('/', slash);
			std::string::size_type backslash = mPrefix.find('\\', slash);
			slash = fwdslash<backslash ? fwdslash : backslash;
			if (slash == std::string::npos) {
				break;
			}
			std::string thisDir = mPrefix.substr(0, slash);
			mkdir(thisDir.c_str(),
#ifndef _WIN32
					0755
#endif
				);

			++slash;
		}
		// FIXME: read the list of files from disk?
		try {
			unserialize();
		} catch (...) {
			std::cerr << "ERROR loading cache index!" << std::endl;
			/// do nothing
		}
	}

	virtual ~DiskCache() {
		boost::shared_ptr<DiskRequest> req
			(new DiskRequest(DiskRequest::EXIT, URI(Fingerprint(),""), Range(true)));
		boost::unique_lock<boost::mutex> sleep_cv(destroyLock);
		mRequestQueue.push(req);
		destroyCV.wait(sleep_cv); // we know the thread has terminated.

		mCleaningUp = true; // don't allow destroyCacheEntry to delete files.

		serialize();
		// FIXME: serialize the list of files?
	}

	virtual void purgeFromCache(const Fingerprint &fileId) {
		CacheMap::write_iterator iter(mFiles);
		if (iter.find(fileId)) {
			iter.erase();
		}
		CacheLayer::purgeFromCache(fileId);
	}

	virtual bool getData(const URI &fileId,
			const Range &requestedRange,
			const TransferCallback&callback) {
		bool haveRange = false;
		{
			CacheMap::read_iterator iter(mFiles);

			if (iter.find(fileId.fingerprint())) {
				const RangeList &list = static_cast<const CacheData*>(*iter)->mRanges;
				haveRange = requestedRange.isContainedBy(list);
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
