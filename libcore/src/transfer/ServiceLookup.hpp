/*  Sirikata Transfer -- Content Distribution Network
 *  ServiceLookup.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
/*  Created on: Feb 5, 2009 */

#ifndef SIRIKATA_ServiceLookup_HPP__
#define SIRIKATA_ServiceLookup_HPP__

#include "URI.hpp"
#include "options/Options.hpp"

namespace Sirikata {
namespace Transfer {

/** A vector of URIContexts to be tried.
 * FIXME: It is not smart to try them in order every time.
 * What if one of these services goes down (and times out each time)?
 *
 * Perhaps it would make sense to have a service get shifted to the
 * end of the list if it is "slow" or experiences a timeout.
 *
 * This could possibly be solved by returning a pointer to a ListOfServicesPtr,
 * and simply changing the pointer but keeping the old ListOfServices around
 * until it is dereferenced.
 *
 * We still need an interface to invalidate a service--perhaps another class.
 */
typedef std::map<std::string, std::string> ServiceParams;
typedef std::vector<std::pair<URIContext, ServiceParams> > ListOfServices;

/// A shared reference to the URIContext, so we don't have to copy it.
typedef std::tr1::shared_ptr<ListOfServices> ListOfServicesPtr;


/** A really confusing system that maps a SURI (service URI) to a regular
 * Internet URI that will have a handler in the ProtocolRegistry.
 *
 * Any ideas on how to make this whole system less confusing?
 */
class ServiceLookup {
public:
	typedef std::tr1::function<void(ListOfServicesPtr)> Callback;
private:

	ServiceLookup* mRespondTo;
	ServiceLookup* mNext;

	inline void setResponder(ServiceLookup* other) {
		mRespondTo = other;
	}

public:

	/** Public function to inject entries into the cache.
	 * This will be made private once the OptionManagerServiceLookup
	 * works well.
	 *
	 * Does nothing except in CachedServiceLookup.
	 */
	virtual void addToCache(const URIContext &origService, const ListOfServicesPtr &toCache) {
		if (mRespondTo) {
			mRespondTo->addToCache(origService, toCache);
		}
	}

	/// Virtual destructor, for subclasses.
	virtual ~ServiceLookup() {
		if (mNext) {
			mNext->setResponder(NULL);
		}
	}

	/** A chain of service lookup methods--the order probably will go something like
	 * CachedServiceLookup -> OptionManagerServiceLookup -> query space server -> NULL
	 *
	 * @param next  The next service lookup to try, or NULL at the end of the chain.
	 */
	ServiceLookup(ServiceLookup* next=NULL)
			: mRespondTo(NULL), mNext(next) {
		if (next) {
			next->setResponder(this);
		}
	}

	/** Looks up a (service)URIContext and returns a corresponding URIContext.
	 * FIXME: because a URIContext includes a path, this is a bit tricky to
	 * do without some sort of pattern matching. In most cases, it is acceptable
	 * to ignore the URIContext::basepath().
	 *
	 * Also, note that usually the username field in the SURI might map to
	 * the path of the resulting URI.
	 *
	 * Example: meerkat://tux@/ => http://namelookup.com/~tux/dns/
	 * In addition, meerkat://tux@someserver/ => http://someserver/~tux/dns/
	 *
	 * There could be a wide variety of returned URIs--basically it depends
	 * on the format of the underlying protocol...
	 *
	 * @param context  The SURI Context to lookup--The basepath MAY be ignored.
	 * @param cb       A callback to be called. Most lookups will be synchronous.
	 *
	 * Default lookup function does nothing except try the next lookup method.
	 */
	virtual void lookupService(const URIContext &context, const Callback &cb) {
		if (mNext) {
			mNext->lookupService(context, cb);
		} else {
			SILOG(transfer,error,"ServiceLookup could find no handlers for " <<
				context);
			cb(ListOfServicesPtr());
		}
	}
};

/** A ServiceLookup that queries the configuration to lookup a URIContext.
 * The format of this is not designed very well yet--ideally it will be
 * possible to use wildcards.
 */
class OptionManagerServiceLookup : public ServiceLookup {
public:
	virtual void lookupService(const URIContext &context, const Callback &cb) {
//		std::vector<UriContext> *vec = GET_OPTION("protocol", context);
		OptionSet *services = OptionSet::getOptions("service");
		OptionValue *serviceOptionValue = NULL;
		services->referenceOption(context.toString(), &serviceOptionValue);
		if (!serviceOptionValue) {
			ServiceLookup::lookupService(context, cb);
			return;
		}
		const ListOfServices &vec = serviceOptionValue->get()->as<const ListOfServices>();
		ListOfServicesPtr vecptr(new ListOfServices(vec));
		addToCache(context, vecptr);
		cb(vecptr);
	}
};

}
}

#endif /* SIRIKATA_ServiceLookup_HPP__ */
