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

//typedef std::tr1::shared_ptr<class ServiceLookup> ServiceLookupPtr;
typedef std::vector<URIContext> ListOfServices;
typedef std::tr1::shared_ptr<ListOfServices> ListOfServicesPtr;

class ServiceLookup {
public:
	typedef std::tr1::function<void(ListOfServicesPtr)> Callback;
private:
	ListOfServicesPtr mEmptyListOfServices;

	ServiceLookup* mRespondTo;
	ServiceLookup* mNext;

	inline void setResponder(ServiceLookup* other) {
		mRespondTo = other;
	}

public:

	virtual void addToCache(const URIContext &origService, const ListOfServicesPtr &toCache) {
		if (mRespondTo) {
			mRespondTo->addToCache(origService, toCache);
		}
	}

	virtual ~ServiceLookup() {
		if (mNext) {
			mNext->setResponder(NULL);
		}
	}

	ServiceLookup(ServiceLookup* next=NULL)
			: mEmptyListOfServices(new ListOfServices()), mRespondTo(NULL), mNext(next) {
		if (next) {
			next->setResponder(this);
		}
	}

	virtual void lookupService(const URIContext &context, const Callback &cb) {
		if (mNext) {
			mNext->lookupService(context, cb);
		} else {
			std::cerr << "ServiceLookup could find no handlers for " <<
				context << std::endl;
			cb(mEmptyListOfServices);
		}
	}
};

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
