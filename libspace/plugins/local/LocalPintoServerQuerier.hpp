/*  Sirikata
 *  LocalPintoServerQuerier.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_LIBSPACE_LOCAL_PINTO_SERVER_QUERIER_HPP_
#define _SIRIKATA_LIBSPACE_LOCAL_PINTO_SERVER_QUERIER_HPP_

#include <sirikata/space/PintoServerQuerier.hpp>

namespace Sirikata {

/** LocalPintoServerQuerier is a dummy implementation of PintoServerQuerier for
 *  single server setups.  Since there are no other space servers, no results
 *  would ever be necessary.  This class just ignores all update requests.
 */
class LocalPintoServerQuerier : public PintoServerQuerier {
public:
    LocalPintoServerQuerier(SpaceContext* ctx) {}
    virtual ~LocalPintoServerQuerier() {}

    virtual void updateRegion(const BoundingBox3f& region) {
    }
    virtual void updateLargestObject(float max_radius) {
    }
    virtual void updateQuery(const SolidAngle& min_angle) {
    }
}; // PintoServerQuerier

} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_LOCAL_PINTO_SERVER_QUERIER_HPP_
