#include "Utility.hpp"

namespace CBR {
class SqrIntegral {
    enum {MAX_CACHE=2};
    double cachedCutoff[MAX_CACHE];
    double cachedFlatness[MAX_CACHE];
    double infiniteIntegral[MAX_CACHE];
    void *gsl_rng_r;
    void *gsl_monte_plane_state_s;
public:
    SqrIntegral();
    typedef double result_type;
    double integrate(double cutoff, double flatness,const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax, double*error);
    double computeWithGivenCutoff(int whichCutoff,const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax);
    double computeInfiniteIntegral(int whichCutoff);
    double operator()(float cutoff, float flatness, const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax){
        for (int i=0;i<MAX_CACHE;++i) {
            if (cachedCutoff[i]==cutoff&&cachedFlatness[i]==flatness) {
                return computeWithGivenCutoff(i,xymin,xymax,uvmin,uvmax);
            }
        }
        for (int i=0;i<MAX_CACHE;++i) {
            if (infiniteIntegral[i]==0||i+1==MAX_CACHE) {
                cachedCutoff[i]=cutoff;
                cachedFlatness[i]=flatness;
                infiniteIntegral[i]=1.0;
                infiniteIntegral[i]=computeInfiniteIntegral(i);                
                return computeWithGivenCutoff(i,xymin,xymax,uvmin,uvmax);
            }
        }
        return 0;
    }

};
}
