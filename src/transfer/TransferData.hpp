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

#include <boost/shared_ptr.hpp>
#include <vector>
#include <list>
#include <iostream>

namespace Iridium {
namespace Transfer {

/** Range identifier -- specifies two segments of a file. */
struct Range {
	typedef int64_t offset_type;

	/// A 64-bit starting byte.
	offset_type mStart;
	/// Length is a size_t (may only be 32 bits).
	size_t mLength;

	Range() :mStart(0), mLength(0) {}

	Range(offset_type start, size_t length)
		: mStart(start), mLength(length) {
	}

	inline offset_type startbyte() const {
		return mStart;
	}
	inline size_t length() const {
		return mLength;
	}
	inline Range::offset_type endbyte() const {
		return mStart + mLength;
	}

	inline bool operator< (const Range &other) const {
		if (mLength == other.mLength) {
			return mStart < other.mStart;
		}
		return mLength < other.mLength;
	}

	template <class ListType>
	bool isContainedBy(const ListType &list) const {
		typename ListType::const_iterator iter = list.begin(),
			enditer = list.end();
		offset_type mEnd = mStart + mLength;
		bool found = false;
		offset_type lastEnd = 0;

		while (iter != enditer) {
			offset_type start = (*iter).startbyte();
			offset_type end = start + (*iter).length();

			if (mStart >= start && mStart < end) {
				found = true;
				lastEnd = end;
				break;
			}
			++iter;
		}
		if (!found) {
			return false;
		}
		found = false;
		while (iter != enditer) {
			offset_type start = (*iter).startbyte();
			if (start > lastEnd) {
				found = false; // gap in range.
				break;
			}
			offset_type end = start + (*iter).length();

			if (mEnd <= end) {
				found = true;
				break;
			}
			lastEnd = end;
			++iter;
		}
		return found;
	}

	inline bool operator== (const Range &other) const {
		return mLength == other.mLength && mStart == other.mStart;
	}
};

typedef std::list<Range> RangeList;


/// Represents a single block of data, and also knows the range of the file it came from.
class DenseData {
public:
	Range mRange;
	std::vector<unsigned char> mData;

	DenseData() {
	}

	DenseData(const Range &range)
			:mRange(range) {
		mData.resize(range.mLength);
	}

	inline Range::offset_type startbyte() const {
		return mRange.mStart;
	}
	inline size_t length() const {
		return mRange.mLength;
	}
	inline Range::offset_type endbyte() const {
		return mRange.mStart + mRange.mLength;
	}

	inline const unsigned char *data() const {
		return &(mData[0]);
	}

	inline void setLength(size_t len) {
		mRange.mLength = len;
		mData.resize(mRange.mLength);
		//message1.reserve(size);
		//std::copy(data, data+len, std::back_inserter(mData));
	}

	inline bool operator <(const DenseData &other) const {
		return mRange < other.mRange;
	}
};

typedef boost::shared_ptr<DenseData> DenseDataPtr;

/// Represents a series of DenseData.  Often you may have adjacent DenseData.
class SparseData {
	typedef std::list<DenseDataPtr> ListType;
	///sorted vector of Range/vector pairs
	ListType mSparseData;
	/*
	///The timestamp of last file update, in case the file changes itself midstream
	int64_t timestamp;
	///the length of the fully filled file, were it all there
	size_t mFileLength;
	*/
public:
	typedef ListType::iterator iterator;
	inline iterator ptrbegin() {
		return mSparseData.begin();
	}
	inline iterator ptrend() {
		return mSparseData.end();
	}

	/// Simple stub iterator class for use by Range::isContainedBy()
	class const_iterator : public ListType::const_iterator {
	public:
		const_iterator(const ListType::const_iterator &e) :
			ListType::const_iterator(e) {
		}
		inline const DenseData &operator* () const {
			return *(this->ListType::const_iterator::operator*());
		}
	};
	inline const_iterator begin() const {
		return mSparseData.begin();
	}
	inline const_iterator end() const {
		return mSparseData.end();
	}
	///adds a range of valid data to the SparseData set.
	void addValidData(const DenseDataPtr &data) {
		iterator endIter=ptrend(), iter=ptrbegin();
		Range::offset_type startdata = data->startbyte();
		while (iter != endIter) {
			if ((*iter)->startbyte() > startdata) {
				break;
			}

			++iter;
		}
		mSparseData.insert(iter, data);
	}

	///gets the space used by the sparse file
	inline size_t getSpaceUsed() {
		size_t length;
		for (const_iterator iter = begin(); iter != end(); ++iter) {
			length += (*iter).length();
		}
		return length;
	}

	void debugPrint(std::ostream &os) const {
		size_t position = 0, len;
		do {
			const unsigned char *data = dataAt(position, len);
			if (data) {
				std::cout << "{GOT DATA "<<len<<"}";
				std::cout << std::string(data, data+len);
			} else if (len) {
				std::cout << "[INVALID:" <<len<< "]";
			}
			position += len;
		} while (len);
		std::cout << std::endl;
	}

	/**
	 * gets a pointer to data starting at offset
	 * @param offset specifies where in the sparse file the data should be gotten from
	 * @param length returns for how much the data is valid
	 * @returns data at that point unless offset is not yet valid in which case dataAt returns NULL with
	 *          length being the range of INVALID data, or 0 if there is not valid data past that point
	 */
	const unsigned char *dataAt(Range::offset_type offset, size_t &length) const {
		const_iterator enditer = end();
		for (const_iterator iter = begin(); iter != enditer; ++iter) {
			const Range &range = (*iter).mRange;
			if (offset >= range.mStart && offset < range.mStart + (Range::offset_type)range.mLength) {
				// We're within some valid data... return the DenseData.
				length = range.mLength + (size_t)(range.mStart - offset);
				return &((*iter).mData[offset - range.mStart]);
			} else if (offset < range.mStart){
				// we missed it.
				length = (size_t)(range.mStart - offset);
				return NULL;
			}
		}
		length = 0;
		return NULL;
	}

};
//typedef boost::shared_ptr<SparseData> SparseDataPtr;

}
}

#endif /* IRIDIUM_TransferData_HPP__ */
