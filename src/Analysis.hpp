/*  cbr
 *  Analysis.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_ANALYSIS_HPP_
#define _CBR_ANALYSIS_HPP_

#include "Utility.hpp"
#include "Time.hpp"
#include "MotionVector.hpp"

namespace CBR {

struct Event;
struct ObjectEvent;
struct PacketEvent;
class ObjectFactory;

/** Error of observed vs. true object locations over simulation period. */
class LocationErrorAnalysis {
public:
    LocationErrorAnalysis(const char* opt_name, const uint32 nservers);
    ~LocationErrorAnalysis();

    // Return true if observer was watching seen at any point during the simulation
    bool observed(const UUID& observer, const UUID& seen) const;

    // Return the average error in the approximation of an object over its observed period, sampled at the given rate.
    double averageError(const UUID& observer, const UUID& seen, const Duration& sampling_rate, ObjectFactory* obj_factory) const;
    // Return the average error in object position approximation over all observers and observed objects, sampled at the given rate.
    double globalAverageError(const Duration& sampling_rate, ObjectFactory* obj_factory) const;
private:
    typedef std::vector<ObjectEvent*> EventList;
    typedef std::map<UUID, EventList*> ObjectEventListMap;

    EventList* getEventList(const UUID& observer) const;

    ObjectEventListMap mEventLists;
}; // class LocationErrorAnalysis


} // namespace CBR

#endif //_CBR_ANALYSIS_HPP_
