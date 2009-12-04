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
class ServiceParams : public std::map<std::string,  std::string> {
	std::string mEmpty;

	std::string &operator[](const std::string &name);
public:
	inline void set(const std::string &name, const std::string &value) {
		iterator iter (insert(value_type(name, std::string())).first);
		(*iter).second = value;
	}

	inline void unset(const std::string &name) {
		iterator iter (find(name));
		if (iter != end()) {
			erase(iter);
		}
	}

	inline const std::string &get(const std::string &name) const {
		const_iterator iter (find(name));
		if (iter != end()) {
			return (*iter).second;
		} else {
			return mEmpty;
		}
	}
	inline const std::string &operator[](const std::string &name) const {
		return get(name);
	}
};
typedef std::vector<std::pair<URIContext, ServiceParams> > ListOfServices;

/// A shared reference to the URIContext, so we don't have to copy it.
typedef std::tr1::shared_ptr<ListOfServices> ListOfServicesPtr;


class ServiceIterator {
public:
	// SUCCESS means that the download was successful (e.g. you are retrying due to a format error)
	enum ErrorType {SUCCESS, UNSUPPORTED, GENERAL_ERROR, NETWORK_ERROR, NOT_FOUND, FORBIDDEN};

	virtual bool tryNext(ErrorType reason, URI &uri, ServiceParams &outParams) {
		delete this;
		return false;
	}

	virtual ~ServiceIterator() {
	}

	/** Notification that the download was successful.
	 * This may help ServiceLookup to pick a better service next time.
	 */
	virtual void finished(ErrorType reason=SUCCESS) {
		delete this;
	}

	inline void relocateURI(URI &uri, const URIContext &context, const std::string &merge) {
		uri.getContext() = URIContext(context, merge);
	}

	void relocate(URI &uri, ServiceParams &params, const URIContext &context, const std::string &merge) {
		if (!params.get("requesturi").empty()) {
			params.set("requesturi",uri.toString());
			uri = URI(context.toString());
		} else {
			relocateURI(uri, context, merge);
		}
	}
};

class SimpleServiceIterator : public ServiceIterator {
	ListOfServicesPtr mServices;
	std::string mMergePath;
	unsigned int mIndex;
public:
	// SUCCESS means that the download was successful (e.g. you are retrying due to a format error)

	SimpleServiceIterator(const ListOfServicesPtr &serv, const std::string &mergePath)
		: mServices(serv), mMergePath(mergePath), mIndex(0) {
	}

	virtual bool tryNext(ErrorType reason, URI &uri, ServiceParams &outParams) {
		if (mIndex >= mServices->size()) {
			delete this;
			return false;
		}
		outParams = (*mServices)[mIndex].second;
		relocate(uri, outParams, (*mServices)[mIndex].first, mMergePath);
		mIndex++;
		return true;
	}

};

/** A really confusing system that maps a SURI (service URI) to a regular
 * Internet URI that will have a handler in the ProtocolRegistry.
 *
 * Any ideas on how to make this whole system less confusing?
 */
class ServiceLookup {
public:
	typedef std::tr1::function<void(ServiceIterator*)> Callback;
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
	virtual void addToCache(const URIContext &origService, const ListOfServicesPtr &toCache, const std::string &mergePath=std::string(), const Callback &cb=Callback()) {
		if (mRespondTo) {
			mRespondTo->addToCache(origService, toCache, mergePath, cb);
		} else if (cb) {
			cb(new SimpleServiceIterator(toCache, mergePath));
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
	 * @param context  The SURI Context to lookup.
     * @param merge    Parts of the URI to append.
	 * @param cb       A callback to be called. Most lookups will be synchronous.
     * @param retry    Pointer to the original ServiceLookup if we need to retry.
	 *
	 * Default lookup function does nothing except try the next lookup method.
	 */
	void lookupService(const URIContext &context, const Callback &cb) {
		doLookupService(context, std::string(), cb, this);
	}
	virtual void doLookupService(const URIContext &context, const std::string &merge, const Callback &cb, ServiceLookup *retry) {
		if (mNext) {
			mNext->doLookupService(context, merge, cb, retry);
		} else {
			SILOG(transfer,error,"ServiceLookup could find no handlers for " <<
				  context);
			// Ideally, we would retry once per directory/hostname/protocol.
			// But until we find a use for this, only support a default URI.
			if (retry && context != URIContext()) {
				std::string mergeStr = merge;
				URIContext parentContext = context;
				parentContext.toParentContext(&mergeStr);
				if (parentContext == context) {
					SILOG(transfer,error,"Failure in URIContext::toParentContext. Old is "<<context<<" new is "<<parentContext);
					assert(parentContext == URIContext());
					cb(new ServiceIterator);
				} else {
					retry->doLookupService(parentContext, mergeStr, cb, retry);
				}
			} else {
				cb(new ServiceIterator);
			}
		}
	}
};

class NullServiceLookup : public ServiceLookup {
	virtual void doLookupService(const URIContext &context, const std::string &merge, const Callback &cb, ServiceLookup *retry) {
		ListOfServicesPtr services(new ListOfServices);
		services->push_back(ListOfServices::value_type(context, ServiceParams()));
		cb(new SimpleServiceIterator(services, merge));
	}
};

/** A ServiceLookup that queries the configuration to lookup a URIContext.
 * The format of this is not designed very well yet--ideally it will be
 * possible to use wildcards.
 */
class OptionManagerServiceLookup : public ServiceLookup {
public:
	virtual void doLookupService(const URIContext &context, const std::string &merge, const Callback &cb, ServiceLookup *retry) {
//		std::vector<UriContext> *vec = GET_OPTION("protocol", context);
		OptionSet *services = OptionSet::getOptions("service");
		OptionValue *serviceOptionValue = NULL;
		services->referenceOption(context.toString(), &serviceOptionValue);
		if (!serviceOptionValue) {
			ServiceLookup::doLookupService(context, merge, cb, retry);
			return;
		}
		const ListOfServices &vec = serviceOptionValue->get()->as<const ListOfServices>();
		ListOfServicesPtr vecptr(new ListOfServices(vec));
		addToCache(context, vecptr, merge, cb);
	}
};

}
}

#endif /* SIRIKATA_ServiceLookup_HPP__ */
