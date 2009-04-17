#ifndef _VISUALIZATION_HPP_
#define _VISUALIZATION_HPP_


namespace CBR {
class LocationService;
class CoordinateSegmentation;
class LocationVisualization :public LocationErrorAnalysis {
    LocationService*mLoc;
    CoordinateSegmentation*mSeg;
    ObjectFactory*mFactory;
    UUID mObserver;
    EventList* mObservedEvents;
    EventList::iterator mCurEvent;
    Time mCurTime;
public:
    void mainLoop();
    LocationVisualization(const char *opt_name, const uint32 nservers, ObjectFactory*obj_factory, LocationService*loc_serv, CoordinateSegmentation*cseg);
    void displayError(const UUID&observer, const Duration& sampling_rate);
    void displayRandomViewerError(int seed, const Duration& sampling_rate);
    
};
}

#endif
