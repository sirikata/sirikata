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
        double deltaLocation=(((xymin+xymax)-(uvmin+uvmax))*.5).length();
        deltaLocation/=flatness;
        deltaLocation+=cutoff;
        if (deltaLocation<2.7) deltaLocation=2.7;
        double logLocDelta=deltaLocation*log(deltaLocation);
        double r1=(xymax-xymin).length();
        double r2=(uvmax-uvmin).length();
        return 2.*r1*r2/(logLocDelta*logLocDelta);
    }

};
}
