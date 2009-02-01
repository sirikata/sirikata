/*  Sirikata Transfer -- Content Transfer management system
 *  Range.hpp
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

#ifndef SIRIKATA_Range_HPP__
#define SIRIKATA_Range_HPP__

namespace Sirikata {
namespace Transfer {

typedef uint64 cache_usize_type;
typedef int64 cache_ssize_type;

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

	explicit Range(bool wholeFile)
		: mStart(0), mLength(0), mWholeFile(wholeFile) {
	}

	Range(base_type start, bool wholeFile)
		: mStart(start), mLength(0), mWholeFile(wholeFile) {
	}

	Range(base_type start, base_type length,
			Initializer type, bool wholeFile=false)
		: mStart(start), mLength(type==LENGTH?length:length-start), mWholeFile(wholeFile) {
	}

	Range(const Range &other)
		: mStart(other.mStart), mLength(other.mLength), mWholeFile(other.mWholeFile) {
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

	friend inline std::ostream &operator << (std::ostream &os, const Range &range) {
		os << "[" << range.startbyte();
		if (range.mLength) {
			os << ", " << range.endbyte();
		}
		if (range.mWholeFile) {
			os << " => eof";
		}
		os << ")";
		return os;
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
			// maxend is not relevant for ranges strictly above us.
			if ((*iter).startbyte() > startdata) {
				break;
			}
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

}
}

#endif
