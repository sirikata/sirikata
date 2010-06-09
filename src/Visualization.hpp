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
