/*  Sirikata
 *  Visualization.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#ifndef _VISUALIZATION_HPP_
#define _VISUALIZATION_HPP_

#include "AnalysisEvents.hpp"
#include "CoordinateSegmentation.hpp"

namespace Sirikata {
class MotionPath;
class CoordinateSegmentation;

class LocationVisualization :public LocationErrorAnalysis {
    CoordinateSegmentation*mSeg;
    UUID mObserver;
    static EventList* NullObservedEvents; // if we don't have an event list to use
    EventList* mObservedEvents;
    EventList::iterator mCurEvent;

    Time mDisplayTime; // The current simulated time, i.e. the time currently
                       // being displayed
    Duration mSamplingRate;

    typedef std::tr1::unordered_map<UUID,TimedMotionVector3f,UUID::Hasher> VisibilityMap;
    VisibilityMap mVisible;
    VisibilityMap mInvisible;

    BoundingBoxList mDynamicBoxes;

    std::vector<SegmentationChangeEvent*> mSegmentationChangeEvents;
    std::vector<SegmentationChangeEvent*>::iterator mSegmentationChangeIterator;

    typedef std::tr1::unordered_map<UUID, MotionPath*, UUID::Hasher> MotionPathMap;
    MotionPathMap mObjectMotions;

    void handleObjectEvent(const UUID& obj, bool add, const TimedMotionVector3f& loc);
    void handleLocEvent(const UUID& obj, const TimedMotionVector3f& loc);

    void displayError(const Duration& sampling_rate);
public:
    void mainLoop();
    LocationVisualization(const char *opt_name, const uint32 nservers, CoordinateSegmentation*cseg);
    void displayError(const UUID&observer, const Duration& sampling_rate);
    void displayError(const ServerID&observer, const Duration& sampling_rate);

    void displayRandomViewerError(int seed, const Duration& sampling_rate);
    void displayRandomServerError(int seed, const Duration& sampling_rate);

};
}

#endif
