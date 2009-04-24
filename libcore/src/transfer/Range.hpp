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

	typedef length_type size_type;
        typedef cache_ssize_type difference_type;

	//static const length_type npos = (cache_usize_type)-1;
private:
	/// A 64-bit starting byte.
	base_type mStart;
	/// Length should be 64-bit as ranges can apply to files on disk.
	length_type mLength;

	bool mWholeFile;

	/// Cause an error for accidentally calling the (int,bool) constructor.
	Range(base_type start, base_type end);
public:

	/// Pass in true for the whole file, and false for an empty range.
	explicit Range(bool wholeFile)
		: mStart(0), mLength(0), mWholeFile(wholeFile) {
	}

	/// Goes from a start byte for the rest of the file--false means an empty range.
	Range(base_type start, bool wholeFile)
		: mStart(start), mLength(0), mWholeFile(wholeFile) {
	}

	/** Generic constructor for a partial range.
	 * If type==LENGTH, specifies the start byte and the length.
	 * If type==BOUNDS, specifies the starting and the ending byte.
	 * Generally, the wholeFile flag will be false here.
	 */
	Range(base_type start, base_type length,
			Initializer type, bool wholeFile=false)
		: mStart(start), mLength(type==LENGTH?length:length-start), mWholeFile(wholeFile) {
	}

	/// Copy constructor
	Range(const Range &other)
		: mStart(other.mStart), mLength(other.mLength), mWholeFile(other.mWholeFile) {
	}

	/** Ranges contain an additional boolean whether they extend
	 * throughout the file.  This is not related to endbyte(), but
	 * endbyte() is often set to 0 if the length is unknown.
	 */
	inline bool goesToEndOfFile() const {
		return mWholeFile;
	}

	/// Returns the initial byte--will be 0 if this references the whole file.
	inline base_type startbyte() const {
		return mStart;
	}
	/** The length -- may not be accurate in the case that goesToEndOfFile() returns
	 * true. In the case of DenseData, this should be equal to the number of bytes
	 * in the DenseData.
	 */
	inline length_type length() const {
		return mLength;
	}
	inline length_type size() const {
	       return length();
	}

	/** The ending byte--again, when goesToEndOfFile() is true, this may be undefined. */
	inline base_type endbyte() const {
		return mStart + mLength;
	}

	/// Sets the length of the range.
	inline void setLength(length_type l, bool wholeFile) {
		mLength = l;
		mWholeFile = wholeFile;
	}
	/// setter for startbyte().
	inline void setBase(base_type s) {
		mStart = s;
	}

	/// Ordering comparator.
	inline bool operator< (const Range &other) const {
		if (mLength == other.mLength) {
			if (mStart == other.mStart && (mWholeFile || other.mWholeFile)) {
				return other.mWholeFile && !mWholeFile;
			}
			return mStart < other.mStart;
		}
		return mLength < other.mLength;
	}

	/** Printing a range. The format is:
	 * "[1)" for empty,
	 * "[8,10)" for a two-byte-long range, and
	 * "[8,10 => eof)" for data with two bytes but will extend for the whole file.
	 * "[0 => eof)" for the whole file.
	 */
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

	/** Checks if one range is inside a list of ranges (or any class with
	 * begin() and end() whose elements are ranges.--such as SparseData) */
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

	inline bool contains(const Range &other) const {
		if (this->goesToEndOfFile()) {
			// endbyte infinitely long -- can contain anything.
			return (other.startbyte() >= this->startbyte());
		} else if (!this->goesToEndOfFile() && other.goesToEndOfFile()) {
			// endbyte is not infinite.
			return false;
		} else {
			return (other.startbyte() >= this->startbyte()) &&
				(other.endbyte() <= this->endbyte());
		}
	}

	inline bool isContainedBy(const Range &other) const {
		return other.contains(*this);
	}

	/// Removes overlapping ranges if possible (assumes a list ordered by starting byte).
	template <class ListType>
	void addToList(const typename ListType::value_type &data, ListType &list) const {
		if (length()<=0) {
			// Ensure that no dummy data will get added. Shouldn't get here.
			return;
		}
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

	template <class ListType>
	static std::ostream &printRangeList(
				std::ostream &os,
				const ListType &cdata,
				Range checkFor=Range(false),
				bool exact=false) {
		bool start = true;
		typename ListType::const_iterator iter = cdata.begin(),
			enditer = cdata.end();
		for (;
				iter != enditer;
				++iter) {
			os << (start?'{':',') << (*iter);
			start = false;
			if ((Range)(*iter) == checkFor) {
				os << "**";
			}
			else if (!exact) {
				if (((Range)(*iter)).isContainedBy(checkFor)) {
					os << "++";
				}
			}
		}
		os << '}';
		return os;
	}

	/// equality comparision
	inline bool operator== (const Range &other) const {
		return mLength == other.mLength &&
			mStart == other.mStart &&
			mWholeFile == other.mWholeFile;
	}
};

/// Simple list of ranges (to be used by Range::isContainedBy and Range::addToList)
typedef std::list<Range> RangeList;

}
}

#endif
