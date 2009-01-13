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
#include "URI.hpp" // defines cache_usize_type, cache_ssize_type

namespace Iridium {
namespace Transfer {

enum Initializer { LENGTH, BOUNDS };

/** Range identifier -- specifies two segments of a file. */
class Range {
public:
	typedef cache_usize_type base_type;
	typedef cache_usize_type length_type;

	//static const length_type npos = (cache_usize_type)-1;
private:
	/// A 64-bit starting byte.
	base_type mStart;
	/// Length should be 64-bit as ranges can apply to files on disk.
	length_type mLength;

	bool mWholeFile;

public:

	Range(bool wholeFile)
		: mStart(0), mLength(0), mWholeFile(wholeFile) {
	}

	Range(base_type start, bool wholeFile)
		: mStart(start), mLength(0), mWholeFile(wholeFile) {
	}

	Range(base_type start, base_type length,
			Initializer type, bool wholeFile=false)
		: mStart(start), mLength(type==LENGTH?length:length-start), mWholeFile(wholeFile) {
	}

	inline bool goesToEndOfFile() const {
		return mWholeFile;
	}

	inline base_type startbyte() const {
		return mStart;
	}
	inline length_type length() const {
		return mLength;
	}
	inline base_type endbyte() const {
		return mStart + mLength;
	}

	inline void setLength(length_type l, bool wholeFile) {
		mLength = l;
		mWholeFile = wholeFile;
	}
	inline void setBase(base_type s) {
		mStart = s;
	}

	inline bool operator< (const Range &other) const {
		if (mLength == other.mLength) {
			if (mStart == other.mStart && (mWholeFile || other.mWholeFile)) {
				return other.mWholeFile && !mWholeFile;
			}
			return mStart < other.mStart;
		}
		return mLength < other.mLength;
	}

	template <class ListType>
	bool isContainedBy(const ListType &list) const {

		typename ListType::const_iterator iter = list.begin(),
			enditer = list.end();

		bool found = false;
		base_type lastEnd = 0;

		while (iter != enditer) {
			base_type start = (*iter).startbyte();
			if (mStart >= start && (*iter).goesToEndOfFile()) {
				return true;
			}
			base_type end = (*iter).endbyte();

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
			base_type start = (*iter).startbyte();
			if (start > lastEnd) {
				found = false; // gap in range.
				break;
			}
			base_type end = (*iter).endbyte();

			// only an infinite range can include an infinite range.
			if ((*iter).goesToEndOfFile()) {
				return true;
			}
			if (!goesToEndOfFile() && endbyte() <= end) {
				found = true;
				break;
			}
			lastEnd = end;
			++iter;
		}
		return found;
	}

	/// Removes overlapping ranges if possible (assumes a list ordered by starting byte).
	template <class ListType>
	void addToList(const typename ListType::value_type &data, ListType &list) const {
		if (startbyte()==0 && goesToEndOfFile()) {
			// favor a single chunk covering the whole file.
			list.clear();
			list.insert(list.end(), data);
			return;
		}

		typename ListType::iterator endIter=list.end(), iter=list.begin();
		Range::base_type startdata = startbyte();
		Range::base_type maxend = startdata;
		bool includeseof = false;
		while (iter != endIter) {
			if ((*iter).endbyte() > maxend) {
				maxend = (*iter).endbyte();
			}
			if ((*iter).goesToEndOfFile()) {
				includeseof = true;
			}
			// we do not want to allow for more than one
			// range starting at the same start byte--
			// If this is the case, one is guaranteed to overlap.
			if ((*iter).startbyte() >= startdata) {
				break;
			}
			++iter;
		}
		if (includeseof || (maxend > endbyte() && !goesToEndOfFile())) {
			return; // already included by another range.
		}
		iter = list.insert(iter, data);
		++iter;
		while (iter != endIter) {
			typename ListType::iterator nextIter = iter;
			++nextIter;

			if (goesToEndOfFile() ||
					(!(*iter).goesToEndOfFile() && (*iter).endbyte() <= endbyte())) {
				list.erase(iter);
			}

			iter = nextIter;
		}
	}

	/// equality comparision
	inline bool operator== (const Range &other) const {
		return mLength == other.mLength &&
			mStart == other.mStart &&
			mWholeFile == other.mWholeFile;
	}
};

typedef std::list<Range> RangeList;


/// Represents a single block of data, and also knows the range of the file it came from.
class DenseData : public Range {
public:
	std::vector<unsigned char> mData;

	DenseData(bool wholeFile)
			:Range(wholeFile) {
	}

	DenseData(const Range &range)
			:Range(range) {
		if (range.length()) {
			mData.resize(range.length());
		}
	}

	inline const unsigned char *data() const {
		return &(mData[0]);
	}

	inline void setLength(size_t len, bool is_npos) {
		Range::setLength(len, is_npos);
		mData.resize(len);
		//message1.reserve(size);
		//std::copy(data, data+len, std::back_inserter(mData));
	}
};

typedef boost::shared_ptr<DenseData> DenseDataPtr;

/// Represents a series of DenseData.  Often you may have adjacent DenseData.
class SparseData {
	typedef std::list<DenseDataPtr> ListType;

	///sorted vector of Range/vector pairs
	ListType mSparseData;
public:
	typedef DenseDataPtr value_type;

	/// Simple stub iterator class for use by Range::isContainedBy()
	class iterator : public ListType::iterator {
	public:
		iterator(const ListType::iterator &e) :
			ListType::iterator(e) {
		}
		inline DenseData &operator* () {
			return *(this->ListType::iterator::operator*());
		}

		inline const DenseDataPtr &getPtr() {
			return this->ListType::iterator::operator*();
		}
	};

	inline iterator begin() {
		return mSparseData.begin();
	}
	inline iterator end() {
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

	inline iterator insert(const iterator &iter, const value_type &dd) {
		return mSparseData.insert(iter, dd);
	}

	inline void erase(const iterator &iter) {
		mSparseData.erase(iter);
	}

	inline void clear() {
		return mSparseData.clear();
	}

	///adds a range of valid data to the SparseData set.
	void addValidData(const DenseDataPtr &data) {
		data->addToList(data, *this);
	}

	///gets the space used by the sparse file
	inline cache_usize_type getSpaceUsed() const {
		cache_usize_type length;
		const_iterator myend = end();
		for (const_iterator iter = begin(); iter != myend; ++iter) {
			length += (*iter).length();
		}
		return length;
	}

	void debugPrint(std::ostream &os) const {
		Range::base_type position = 0, len;
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
	const unsigned char *dataAt(Range::base_type offset, Range::length_type &length) const {
		const_iterator enditer = end();
		for (const_iterator iter = begin(); iter != enditer; ++iter) {
			const Range &range = (*iter);
			if (offset >= range.startbyte() &&
					(range.goesToEndOfFile() || offset < range.endbyte())) {
				// We're within some valid data... return the DenseData.
				length = range.length() + (Range::length_type)(range.startbyte() - offset);
				return &((*iter).mData[offset - range.startbyte()]);
			} else if (offset < range.startbyte()){
				// we missed it.
				length = (size_t)(range.startbyte() - offset);
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
