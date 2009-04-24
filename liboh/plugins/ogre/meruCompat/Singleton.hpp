/*  Meru
 *  Singleton.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _SINGLETON_HPP_
#define _SINGLETON_HPP_

#include <cassert>
#include <cstdlib>

namespace Meru {

/* Templated singleton classes.  */

/** Use one of the following macros to create the storage for
 *  for the singleton pointer for a singleton class.  E.g. if
 *  you declare class MySingleton : public AutoSingleton<MySingleton>
 *  then you should have AUTO_SINGLETON_STORAGE(MySingleton) in your
 *  corresponding cpp file.
 */
#define MANUAL_SINGLETON_STORAGE(klass) \
    template<> klass* Meru::ManualSingleton<klass>::sSingleton = NULL
#define AUTO_SINGLETON_STORAGE(klass) \
    template<> klass* Meru::AutoSingleton<klass>::sSingleton = NULL

/** ManualSingleton - Templated singleton class for types
 *  which should be created manually, i.e. their constructor
 *  should be called explicitly instead of being created
 *  automatically when getSingleton() or getSingletonPtr()
 *  are called for the first time.  Generally this type of
 *  singleton should be used when you can choose a specific
 *  implementation subclass.
 */
template<class T>
class ManualSingleton {
public:
	/** Construct the manual singleton.  No instance of this class
	 *  may exist yet and this instance will be set as the singleton.
	 */
	ManualSingleton() {
		assert(sSingleton == NULL);
		sSingleton = static_cast<T*>(this);
	}

    static bool isInstantiated() {
        return sSingleton != NULL;
    }

	/** Destroy this singleton.  The global singleton is set to NULL. */
	virtual ~ManualSingleton() {
		assert(sSingleton != NULL);
		sSingleton = NULL;
	}

	/** Get a reference to the singleton for this class.  An instance
	 *  of this class must already have been created.
	 *  \returns a reference to the singleton for this class
	 */
	static T& getSingleton() {
		assert(sSingleton != NULL);
		return *sSingleton;
	}

	/** Get a pointer to the singleton for this class.  An instance
	 *  of this class must already have been created.
	 *  \returns a pointer to the singleton for this class
	 */
	static T* getSingletonPtr() {
		assert(sSingleton != NULL);
		return sSingleton;
	}

	/** Destroy this class's singleton by calling delete on the singleton
	 *  if one exists.
	 */
	static void destroy() {
		if (sSingleton != NULL)
			delete sSingleton;
	}
protected:
	static T* sSingleton;
};


/** AutoSingleton - Templated singleton class for types
 *  which should be created automatically, i.e. if
 *  getSingleton() or getSingletonPtr() are called and
 *  the singleton does not exist yet, one will be created
 *  automatically.  This requires a default constructor
 *  which is accessible to this base class.  Use this type
 *  of singleton for classes that only have one implementation
 *  and which can be initialized at any time (i.e. ordering with
 *  other classes' initialization is unimportant).
 */
template<class T>
class AutoSingleton {
public:
	/** Create a singleton.  One must not currently exist
	 *  and this one will be set as the singleton for this class.
	 */
	AutoSingleton() {
		assert(sSingleton == NULL);
		sSingleton = static_cast<T*>(this);
	}

	virtual ~AutoSingleton() {
		assert(sSingleton != NULL);
		sSingleton = NULL;
	}

	/** Get a reference to the singleton for this class.  If one does
	 *  not exist yet then one will be created automatically.
	 *  \returns a reference to the singleton for this class
	 */
	static T& getSingleton() {
		if (sSingleton == NULL)
			sSingleton = new T();
		assert(sSingleton != NULL);
		return *sSingleton;
	}

	/** Get a pointer to the singleton for this class.  If one does
	 *  not exist yet then one will be created automatically.
	 *  \returns a pointer to the singleton for this class
	 */
	static T* getSingletonPtr() {
		if (sSingleton == NULL)
			sSingleton = new T();
		assert(sSingleton != NULL);
		return sSingleton;
	}

	/** Destroy the singleton for this class by calling delete on
	 *  the singleton, if it exists.
	 */
	static void destroy() {
		if (sSingleton != NULL)
			delete sSingleton;
	}

protected:
	static T* sSingleton;
};

} // namespace Meru

#endif //_SINGLETON_HPP_
