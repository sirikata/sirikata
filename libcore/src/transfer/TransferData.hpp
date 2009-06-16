/*  Sirikata Transfer -- Content Transfer management system
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

#ifndef SIRIKATA_TransferData_HPP__
#define SIRIKATA_TransferData_HPP__

#include "util/Sha256.hpp"
#include "Range.hpp"

namespace Sirikata {
namespace Transfer {


/// Represents a single block of data, and also knows the range of the file it came from.
class DenseData : Noncopyable, public Range {
	std::vector<unsigned char> mData;

    // All too easy to mix up string constructors (binarydata,length) with (string,startbyte)
	DenseData(const char *str, size_t len) : Range(false) {}
	DenseData(const unsigned char *str, size_t len) : Range(false) {}

public:
	/// The only constructor--the length can be changed later with setLength().
	DenseData(const Range &range)
			:Range(range) {
		if (range.length()) {
			mData.resize((std::vector<unsigned char>::size_type)range.length());
		}
	}

	DenseData(const std::string &str, Range::base_type start=0, bool wholeFile=true)
			:Range(start, str.length(), LENGTH, wholeFile) {
		setLength(str.length(), wholeFile);
		std::copy(str.begin(), str.end(), writableData());
	}

	/// equals dataAt(startbyte()).
	inline const unsigned char *data() const {
		return &(mData[0]);
	}

	inline const unsigned char *begin() const {
		return data();
	}

	inline const unsigned char *end() const {
		return data()+length();
	}

	/// Returns a non-const data, starting at startbyte().
	inline unsigned char *writableData() {
		return &(mData[0]);
	}

	/** Returns a char* to data at a cetain offset. Does not return the corresponding
	 * Length, but it is your responsibility to compare to endbyte().
	 * Note that it will also return NULL if startbyte() > offset.
	 */
	inline const unsigned char *dataAt(base_type offset) const {
		if (offset >= endbyte() || offset < startbyte()) {
			return NULL;
		}
		return &(mData[(std::vector<unsigned char>::size_type)(offset-startbyte())]);
	}

	inline std::string asString() const {
		return std::string((const char *)data(),(size_t)length());
	}

	/// Sets the length of the range, as well as allocates more space in the data vector.
	inline void setLength(size_t len, bool is_npos) {
		Range::setLength(len, is_npos);
		mData.resize(len);
		//message1.reserve(size);
		//std::copy(data, data+len, std::back_inserter(mData));
	}
};

typedef std::tr1::shared_ptr<DenseData> MutableDenseDataPtr;
typedef std::tr1::shared_ptr<const DenseData> DenseDataPtr;

// Meant to act like an STL list.
class DenseDataList {
protected:
	typedef std::list<DenseDataPtr> ListType;

	///sorted vector of Range/vector pairs
	ListType mSparseData;

	DenseDataList() {
	}

public:
	/// Acts like a STL container.
	typedef DenseDataPtr value_type;

    /// Simple stub iterator class for use by Range::isContainedBy()
	class iterator : public ListType::iterator {
	public:
		iterator(const ListType::iterator &e) :
			ListType::iterator(e) {
		}
		inline const DenseData &operator* () const {
			return *(this->ListType::iterator::operator*());
		}

		inline const DenseData *operator-> () const {
			return &(*(this->ListType::iterator::operator*()));
		}

		inline const DenseDataPtr &getPtr() const {
			return this->ListType::iterator::operator*();
		}
    };

    /// Simple iteration functions, to keep compatibility with RangeList.
    inline iterator begin() {
            return mSparseData.begin();
    }
    /// Simple iteration functions, to keep compatibility with RangeList.
    inline iterator end() {
            return mSparseData.end();
    }

    /// Simple stub iterator class for use by Range::isContainedBy()
    class const_iterator : public ListType::const_iterator {
    public:
        const_iterator() {
        }
            const_iterator(const ListType::const_iterator &e) :
                    ListType::const_iterator(e) {
            }
            const_iterator(const ListType::iterator &e) :
                    ListType::const_iterator(e) {
            }
            inline const DenseData &operator* () const {
                    return *(this->ListType::const_iterator::operator*());
            }

            inline const DenseData *operator-> () const {
                    return &(*(this->ListType::const_iterator::operator*()));
            }
    };
    /// Simple iteration functions, to keep compatibility with RangeList.
    inline const_iterator begin() const {
            return mSparseData.begin();
    }
    /// Simple iteration functions, to keep compatibility with RangeList.
    inline const_iterator end() const {
            return mSparseData.end();
    }

    /// insert into the internal std::list.
    inline iterator insert(const iterator &iter, const value_type &dd) {
            return mSparseData.insert(iter, dd);
    }

    /// delete from the internal std::list.
    inline void erase(const iterator &iter) {
            mSparseData.erase(iter);
    }

    /// Clear all data.
    inline void clear() {
            return mSparseData.clear();
    }

    /// Is there any data.
    inline bool empty() const {
            return mSparseData.empty();
    }

    ///adds a range of valid data to the SparseData set.
    void addValidData(const DenseDataPtr &data) {
            data->addToList(data, *this);
    }

    ///gets the space used by the sparse file.
    inline cache_usize_type getSpaceUsed() const {
            cache_usize_type length = 0;
            const_iterator myend = end();
            for (const_iterator iter = begin(); iter != myend; ++iter) {
                    length += (*iter).length();
            }
            return length;
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
				return (*iter).dataAt(offset);
			} else if (offset < range.startbyte()){
				// we missed it.
				length = (size_t)(range.startbyte() - offset);
				return NULL;
			}
		}
		length = 0;
		return NULL;
	}

	inline cache_usize_type startbyte() const {
		if (mSparseData.empty()) {
			return 0;
		}
		return mSparseData.front()->startbyte();
	}

	inline cache_usize_type endbyte() const {
		if (mSparseData.empty()) {
			return 0;
		}
		return mSparseData.back()->endbyte();
	}

	inline bool contiguous() const {
		if (mSparseData.empty())
			return true;
		return Range(startbyte(), endbyte(), BOUNDS).isContainedBy(*this);
	}

	inline bool contains(const Range &other) const {
		return other.isContainedBy(*this);
	}
};

/// Represents a series of DenseData.  Often you may have adjacent DenseData.
class SparseData : public DenseDataList {
public:
	typedef cache_usize_type size_type;
	typedef cache_ssize_type difference_type;
	typedef const unsigned char value_type;

	class const_iterator {
	public:
		typedef SparseData::size_type size_type;
		typedef SparseData::difference_type difference_type;
		typedef SparseData::value_type value_type;
		typedef std::random_access_iterator_tag iterator_category;
		typedef value_type* pointer;
		typedef value_type& reference;

	private:
		DenseDataList::const_iterator iter;
		const SparseData *parent;
		value_type *data;
        size_type globalbyte;
        size_type datastart;
		size_type dataend;

		void setDataPtr() {
			if (globalbyte >= parent->endbyte() || iter == (parent->DenseDataList::end())) {
				data = NULL;
				dataend = globalbyte;
				datastart = globalbyte;
			} else {
				const DenseData&dd = *iter;
				data = dd.data();
				datastart = dd.startbyte();
				dataend = dd.endbyte();
			}
		}
		void fixData() {
			while (iter != (parent->DenseDataList::begin()) && datastart > globalbyte) {
				--iter;
				setDataPtr();
			}
			while (data && dataend <= globalbyte) {
				++iter;
				setDataPtr();
				if (datastart > globalbyte) {
					throw std::runtime_error("SparseData iterator skipped over some data");
				}
			}
			if (iter == (parent->DenseDataList::end())) {
				globalbyte = parent->endbyte();
			}
		}
	public:
		const_iterator() {
			parent = NULL;
			data = NULL;
			datastart = 0;
			dataend = 0;
			globalbyte = 0;
		}
		const_iterator (const DenseDataList::const_iterator &iter,
				const SparseData *parent, SparseData::size_type pos)
				: iter(iter), parent(parent) {
			/* // Caller's responsibility to check if contiguous.
			if (!parent->contiguous()) {
				throw std::domain_error("Cannot create iterator over noncontiguous SparseData");
			}
			*/
        	globalbyte = pos;
            setDataPtr();
		}

		unsigned char operator*() const{
			return data[globalbyte-datastart];
		}
		const_iterator &operator+=(difference_type diff) {
			globalbyte += diff;
			if (globalbyte < datastart || globalbyte >= dataend) {
				fixData();
			}
			return *this;
		}
		const_iterator &operator-=(difference_type diff) {
			operator+=(-diff);
			return *this;
		}
		const_iterator &operator++() {
			++globalbyte;
			if (globalbyte >= dataend) {
				fixData();
			}
			return *this;
		}
		const_iterator &operator--() {
			--globalbyte;
			if (globalbyte < datastart) {
				fixData();
			}
			return *this;
		}
		const_iterator operator+(difference_type diff) const {
			const_iterator other (*this);
			return (other += diff);
		}
		const_iterator operator-(difference_type diff) const {
			const_iterator other (*this);
			return (other -= diff);
		}

		difference_type operator-(const const_iterator&other) const {
			return (globalbyte - other.globalbyte);
		}

		bool operator<(const const_iterator&other) const {
			return (globalbyte < other.globalbyte);
		}
		bool operator==(const const_iterator&other) const {
			return (other.iter == iter && other.globalbyte == globalbyte);
		}
		bool operator!=(const const_iterator&other) const {
			return (other.iter != iter || other.globalbyte != globalbyte);
		}
	};

	const_iterator begin() const {
		return const_iterator(DenseDataList::begin(), this, startbyte());
	}
	const_iterator end() const {
		return const_iterator(DenseDataList::end(), this, endbyte());
	}

	/*// slow: better to use iterators.
	const char operator[] (cache_usize_type location) {
		Range::length_type length;
		const unsigned char *data = dataAt(location, length);
		if (data == NULL) {
			return '\0';
		}
		if (length == 0) {
			throw new std::domain_error("byte outside range passed to SparseData::operator[]");
		}
		return *data;
	}
	*/

	typedef const_iterator iterator; // Only supports read-only operations

	SparseData() {
	}

	SparseData(DenseDataPtr contents) {
		addValidData(contents);
	}

	inline cache_usize_type length() const {
		return endbyte() - startbyte();
	}

	inline cache_usize_type size() const {
		return length();
	}

	/** Would be a << operator, but it's inefficient, and should only be
	 * used for debugging/testing purposes, so it's safer as a different function.
	 */
	std::ostream & debugPrint(std::ostream &os) const {
		Range::base_type position = 0, len;
		do {
			const unsigned char *data = dataAt(position, len);
			if (data) {
				os<<"{GOT DATA "<<len<<"}";
				os<<std::string(data, data+len);
			} else if (len) {
				os<<"[INVALID:" <<len<< "]";
			}
			position += len;
		} while (len);
		return os;
	}

	SHA256 computeFingerprint() const {
		SHA256Context context;
		const unsigned char *data;
		Range::length_type length;
		Range::base_type start = 0;

		while (true) {
			data = dataAt(start, length);
			start += length;
			if (data == NULL && length == 0) {
				break;
			} else if (data == NULL) {
				context.updateZeros((size_t)length);
			} else {
				context.update(data, (size_t)length);
			}
		}
		return context.get();
	}

        DenseDataPtr flatten() const {
                if (mSparseData.size() == 0) {
                        return DenseDataPtr(new DenseData(Range(false)));
                }
                if (mSparseData.size() == 1) {
                        return mSparseData.front();
                }
                MutableDenseDataPtr denseData (new DenseData(Range(startbyte(),endbyte(),BOUNDS)));
                unsigned char *outdata = denseData->writableData();
                std::copy(begin(), end(), outdata);
                return denseData;
        }

};
//typedef std::tr1::shared_ptr<SparseData> SparseDataPtr;

}
}

#endif /* SIRIKATA_TransferData_HPP__ */
