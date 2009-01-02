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

namespace Iridium {
namespace Transfer {


class DiskCache : CacheLayer {
	typedef std::map<FileId, std::pair<std::list<Range>, CacheInfoPtr > > FileMap;
	FileMap mFiles;
protected:
	virtual void populateCache(const FileId& file, const DenseData &data) {
		//
	}
private:
	void finishedReading(const FileId &fileId, const TransferCallback&callback,
			const DenseData &data) {
		mCacheManager.mLock.acquire();
		TransferLayer::populatePreviousCaches(fileId, data);
		mCacheManager.mLock.release();
		callback(data);
	}

public:
	virtual bool getData(const FileId &fileId, const Range &requestedRange,
			const TransferCallback&callback) {
		DataMap::const_iterator iter = mFiles.find(fileID);
		if (requestedRange.isContainedBy((*iter).first)) {
			readDataFromDisk(fileId, requestedRange,
					boost::bind(&finishedReading, this, fileId, callback));
		} else {
			TransferLayer::getData(fileId, requestedRange, callback);
		}
	}
};

}
}

#endif /* IRIDIUM_DiskCache_HPP__ */
