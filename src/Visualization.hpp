#ifndef _VISUALIZATION_HPP_
#define _VISUALIZATION_HPP_

#include "AnalysisEvents.hpp"
#include "CoordinateSegmentation.hpp"


namespace CBR {
class LocationService;
class CoordinateSegmentation;
class LocationVisualization :public LocationErrorAnalysis {
    CoordinateSegmentation*mSeg;
    ObjectFactory*mFactory;
    UUID mObserver;
    EventList* mObservedEvents;
    EventList::iterator mCurEvent;
    Time mCurTime;
    Duration mSamplingRate;
    typedef std::tr1::unordered_map<UUID,TimedMotionVector3f,UUID::Hasher> VisibilityMap;
    VisibilityMap mVisible;
    VisibilityMap mInvisible;

    BoundingBoxList mDynamicBoxes;

    std::vector<SegmentationChangeEvent*> mSegmentationChangeEvents;
    std::vector<SegmentationChangeEvent*>::iterator mSegmentationChangeIterator;

    void handleObjectEvent(const UUID& obj, bool add, const TimedMotionVector3f& loc);
    void handleLocEvent(const UUID& obj, const TimedMotionVector3f& loc);

    void displayError(const Duration& sampling_rate);
public:
    void mainLoop();
    LocationVisualization(const char *opt_name, const uint32 nservers, ObjectFactory*obj_factory, CoordinateSegmentation*cseg);
    void displayError(const UUID&observer, const Duration& sampling_rate);
    void displayError(const ServerID&observer, const Duration& sampling_rate);

    void displayRandomViewerError(int seed, const Duration& sampling_rate);
    void displayRandomServerError(int seed, const Duration& sampling_rate);

};
}

#endif
