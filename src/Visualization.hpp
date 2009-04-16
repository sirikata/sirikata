#ifndef _VISUALIZATION_HPP_
#define _VISUALIZATION_HPP_

namespace CBR {
class LocationVisualization :public LocationErrorAnalysis {
public:
    LocationVisualization(const char *opt_name, const uint32 nservers);
    void displayError(const UUID&observer, const Duration& sampling_rate, ObjectFactory* obj_factory);
    void displayRandomViewerError(int seed, const Duration& sampling_rate, ObjectFactory* obj_factory);
    
};
}

#endif
