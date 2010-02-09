/*  Sirikata Kernel -- Task scheduling system
 *  UniqueId.hpp
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

#ifndef SIRIKATA_UniqueId_HPP__
#define SIRIKATA_UniqueId_HPP__

#include <typeinfo>


namespace Sirikata {

/**
 * UniqueId.hpp -- Defines a SubscriptionId, as well as macros
 * to ease the creation of a FunctionId to go along with the
 * std::tr1::function objects passed into EventManager and TimerQueue.
 */
namespace Task {

/**
 * An ID to allow comparing two callback functions.  It is up
 * to the creator to ensure uniqueness of FunctionId objects.
 *
 * @see TimerQueue
 * @see EventManager
 */
class FunctionId {
private:
	void *mThisPtr; ///< do not dereference
	std::string mClassId; ///< A compile-time constant, usually a class name or file or module name.
	std::string mUniqueId; ///< usually contains class or function name, and other arguments.

public:

	/// Equality comparison
	inline bool operator== (const FunctionId &other) const {
		if (mThisPtr != other.mThisPtr)
			return false;
		if (mClassId == other.mClassId)
			return false;
		return (mUniqueId == other.mUniqueId);
	}
	/// Ordering comparison
	inline bool operator< (const FunctionId &other) const {
		if (mThisPtr == other.mThisPtr) {
			if (mClassId == other.mClassId) {
				return (mUniqueId < other.mUniqueId);
			} else {
				return (mClassId < other.mClassId);
			}
		} else {
			return (mThisPtr < other.mThisPtr);
		}
	}

	/**
	 * SubscriptionId is an ID that should be unique.  This ID is specified by
	 * the class, the class instance, and any other extra information needed
	 * in order to make this ID unique.
	 *
	 * @param thisPtr   Some identifying pointer for this class, or NULL.
	 * @param classId   A compile-time string (FULLY QUALIFIED class or filename)
	 * @param uniqueId  A specific string representing the request, usually
	 *                  the same as the corresponding SecondaryId.
	 */
	FunctionId(void *thisPtr,
				const std::string &classId,
				const std::string &uniqueId)
		: mThisPtr(thisPtr), mClassId(classId), mUniqueId(uniqueId) {
	}

	/**
	 * Create a null subscription ID.  A null subscription ID cannot be
	 * explicitly unsubscribed (and has greater efficiency when adding
	 * or removing event listeners)
	 */
	static inline FunctionId null() {
		return FunctionId(NULL, std::string(), std::string());
	}

	/// Hasher functor to be used in a hash_map.
	struct Hasher {
		std::size_t operator() (const FunctionId &sid) const{
			return std::tr1::hash<intptr_t>() ((intptr_t)sid.mThisPtr) * 43 +
				std::tr1::hash<std::string>() (sid.mClassId) * 41 +
				std::tr1::hash<std::string>() (sid.mUniqueId.c_str());
		}
	};
};

// TODO: provide a better (fool-proof) interface for these.

/// An ID for this instance of a class
#define CLASS_ID(cls, arg) \
	SubscriptionId(this, #cls, arg)

/// A generic ID.
#define GEN_ID(ptr, constname, id) \
	SubscriptionId(ptr, "[" constname "]", id)



/** A default integer that reuses IDs to stay compact. */
class CompactSubId {
public:
	typedef int Type; ///< What primitive storage type this needs
private:
	static std::list<Type> freelist;
	static Type nextid;
public:
	/** allocate the next available id (may come from freelist) */
	static inline Type alloc() {
		// LOCK
		int front;
		if (freelist.empty()) {
			front = nextid++;
		} else {
			front = freelist.front();
			freelist.pop_front();
		}
		// UNLOCK
		return front;
	}
	/** Return this ID to the freelist (be careful of double-frees) */
	static inline void free(Type id) {
		// LOCK
		freelist.push_back(id);
		// UNLOCK
	}
	/** Return a 'null' id that is only equal to another null id. */
	static inline Type null() {
		return -1;
	}
};

/** A 64-bit number that is always increasing, and never reuses IDs. */
class IncreasingSubId {
public:
	typedef int64 Type; ///< What primitive storage type this needs
private:
	static Type nextid;
public:
	/** allocate the next available id */
	static inline Type alloc() {
		// LOCK
		return nextid++;
		// UNLOCK
	}
	/** Called when an ID is no longer needed (for debug) */
	static inline void free(Type id) {
	}

	/** Return a 'null' id that is only equal to another null id. */
	static inline Type null() {
		return -1;
	}
};

//#ifdef _DEBUG

/** EventManager allows double-frees, so it is not safe to reuse IDs. */
typedef IncreasingSubId SubscriptionIdClass;
//#else
//typedef CompactSubId SubscriptionIdClass;
//#endif

/// The primitive type associated with SubscriptionIdClass.
typedef SubscriptionIdClass::Type SubscriptionId;
struct SubscriptionIdHasher {
	std::size_t operator() (SubscriptionId sid) const{
#ifdef __APPLE__
		return std::tr1::hash<unsigned int>()(((unsigned int)(sid>>32))^(unsigned int)sid);
#else
		return std::tr1::hash<SubscriptionId>()(sid);
#endif
	}
};

}
}

#endif
