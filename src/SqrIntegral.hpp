#include "Utility.hpp"

namespace CBR {
class SqrIntegral {
    enum {MAX_CACHE=2};
    double cachedCutoff[MAX_CACHE];
    double cachedDimensions[MAX_CACHE][2];
    double cachedFlatness[MAX_CACHE];
    double infiniteIntegral[MAX_CACHE];
    bool normalize;
    void *gsl_rng_r;
    void *gsl_monte_plane_state_s;
    static bool approxEqual(double a, double b) {
        double eps=1.0e-6;
        if (a>b) {
            return a-b<eps;
        }
        return b-a<eps;
    }
public:
    SqrIntegral(bool normalize);
    typedef double result_type;
    double integrate(double cutoff, double flatness,const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax, double*error);
    double computeWithGivenCutoff(int whichCutoff,const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax);
    double computeInfiniteIntegral(int whichCutoff);
    double operator()(float cutoff, float flatness, const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax){
        if (!normalize) {
            double error=0;
            assert(xymin.x<=xymax.x);
            assert(xymin.y<=xymax.y);
            assert(uvmin.x<=uvmax.x);
            assert(uvmin.y<=uvmax.y);
            return integrate(cutoff,flatness,xymin,xymax,uvmin,uvmax,&error);
        }
        //assert (cutoff*flatness>1);
        for (int i=0;i<MAX_CACHE;++i) {
            if (cachedCutoff[i]==cutoff&&cachedFlatness[i]==flatness&&approxEqual(cachedDimensions[i][0],(xymax.x-xymin.x))&&approxEqual(cachedDimensions[i][1],(xymax.y-xymin.y))) {
                return computeWithGivenCutoff(i,xymin,xymax,uvmin,uvmax);
            }
        }
        for (int i=0;i<MAX_CACHE;++i) {
            if (infiniteIntegral[i]==0||i+1==MAX_CACHE) {
                cachedCutoff[i]=cutoff;
                cachedFlatness[i]=flatness;
                cachedDimensions[i][0]=xymax.x-xymin.x;
                cachedDimensions[i][1]=xymax.y-xymin.y;
                infiniteIntegral[i]=1.0;
                infiniteIntegral[i]=computeInfiniteIntegral(i);  
                return computeWithGivenCutoff(i,xymin,xymax,uvmin,uvmax);
            }
        }
        return 0;
    }

};
}
