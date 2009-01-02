/*     Iridium Transfer -- Content Transfer management system
 *  TransferData.hpp
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

#ifndef IRIDIUM_TransferData_HPP__
#define IRIDIUM_TransferData_HPP__

#include <vector>

namespace Iridium {
namespace Transfer {

/** Range identifier -- specifies two segments of a file. */
struct Range {
	/// A 64-bit starting byte.
	uint64_t mStart;
	/// Length is a size_t (may only be 32 bits).
	size_t mLength;

	inline bool operator< (const Range &other) const {
		if (mLength == other.mLength) {
			return mStart < other.mStart;
		}
		return mLength < other.mLength;
	}

	inline bool operator== (const Range &other) const {
		return mLength == other.mLength && mStart == other.mStart;
	}
};


class DenseData {
public:
	Range mRange;
	std::vector<unsigned char> mData;

	inline int length() const {
		return mRange.mLength;
	}

	inline bool operator <(const DenseData &other) const {
		return mRange < other.mRange;
	}
	void merge(const DenseData&other);
};

class SparseData {
	///sorted vector of Range/vector pairs
	std::list<DenseData> mSparseData;
	///The timestamp of last file update, in case the file changes itself midstream
	int64_t timestamp;
	///the length of the fully filled file, were it all there
	size_t mFileLength;
public:
	///Serializes the sparse data into a dense stream
	void serialize(std::ostream&);
	///unserializes the sparse data from a dense stream
	void unserialize(std::istream&);
	///adds a range of valid data to the SparseData set.
	void addValidData(const Range&, unsigned char *data);
	///gets the length of the sparse file, were it fully populated
	size_t getFileLength();
	/**
	 * gets a pointer to data starting at offset
	 * @param offset specifies where in the sparse file the data should be gotten from
	 * @param length returns for how much the data is valid
	 * @returns data at that point unless offset is not yet valid in which case dataAt returns NULL with
	 *          length being the range of INVALID data, or 0 if there is not valid data past that point
	 */
	unsigned char *dataAt(size_t offset, size_t &length);

};
typedef boost::shared_ptr<SparseData> SparseDataPtr;

}
}

#endif /* IRIDIUM_TransferData_HPP__ */
