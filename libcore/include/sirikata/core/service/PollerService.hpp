// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_POLLER_SERVICE_HPP_
#define _SIRIKATA_CORE_POLLER_SERVICE_HPP_

#include <sirikata/core/service/Poller.hpp>
#include <sirikata/core/service/Service.hpp>

namespace Sirikata {

/** PollerService is a Poller which also inherits from Service. This doesn't
 *  change Poller's interface at all, but makes it usable as a Service without
 *  forcing Poller to implement Service (which causes other problems requiring
 *  virtual inhertitance).
 */
class SIRIKATA_EXPORT PollerService : public Poller, public Service {
public:
    PollerService(Network::IOStrand* str, const Network::IOCallback& cb, const char* cb_tag, const Duration& max_rate = Duration::microseconds(0), bool accurate = false)
        : Poller(str, cb, cb_tag, max_rate, accurate)
    {}
    virtual ~PollerService() {}

    virtual void start() {
        Poller::start();
    }
    virtual void stop() {
        Poller::stop();
    }
}; // class Poller

} // namespace Sirikata

#endif //_SIRIKATA_CORE_POLLER_SERVICE_HPP_
