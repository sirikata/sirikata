#include "SqrIntegral.hpp"

extern "C" {
#include <gsl/gsl_math.h>
#include <gsl/gsl_monte.h>
#include <gsl/gsl_monte_plain.h>
#include <gsl/gsl_rng.h>

struct sqrParams{
    double constant_extent_rho;
    double constant_inner_speed_k;
    double flatness;
};
double inverse_polynomial(double r, double r2) {
    double logr=log(r+1);
    return 1./(r2*logr*logr);
}
double bandwidth_bound_no_cutoff(double r, double r2, size_t dim, void *v_params) {
    struct sqrParams*params=(struct sqrParams*)v_params;    
    double flatness=params->flatness;
    r*=flatness;
    r2*=flatness*flatness;
    return inverse_polynomial(r,r2);
}
double bandwidth_bound(double *x, size_t dim, void *v_params) {
    struct sqrParams*params=(struct sqrParams*)v_params;
    
    double p =params->constant_extent_rho;
    double k = params->constant_inner_speed_k;
    double r2=(x[2]-x[0])*(x[2]-x[0])+(x[3]-x[1])*(x[3]-x[1]);
    double r=sqrt(r2);
    if (r<=p) return k;
    return bandwidth_bound_no_cutoff(r,r2,dim,params);
}
}
#define gsl_monte_plain_integratePRINT(F0, xl, xu, dim, NCALLS, r, s, res1, err1) (gsl_monte_plain_integrate(F0, xl, xu, dim, NCALLS, r, s, res1, err1)),printf ("Integrating (%.0f %.0f)->(%.0f %.0f) to (%.0f %.0f)->(%.0f %.0f)=%lf -+ %lf\n",xl[0],xl[1],xu[0],xu[1],xl[2],xl[3],xu[2],xu[3],*res1,*err1);                

namespace CBR {
SqrIntegral::SqrIntegral() {
	gsl_monte_plain_state *s = gsl_monte_plain_alloc(4);
	gsl_rng *r = gsl_rng_alloc(gsl_rng_default);
    gsl_rng_r=r;
    gsl_monte_plane_state_s=s;
    memset(cachedDimensions,0,sizeof(cachedDimensions));
    memset(cachedCutoff,0,sizeof(cachedCutoff));
    memset(cachedFlatness,0,sizeof(cachedFlatness));
    memset(infiniteIntegral,0,sizeof(infiniteIntegral));
}
double SqrIntegral::integrate(double cutoff, double flatness,const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax, double*error){
    int NCALLS=256;
    sqrParams params;
    params.constant_extent_rho=cutoff;
    params.constant_inner_speed_k=1.0;
    params.flatness=flatness;
    params.constant_inner_speed_k=bandwidth_bound_no_cutoff(cutoff,cutoff*cutoff,4,&params);
    double xl[4]={xymin.x,xymin.y,uvmin.x,uvmin.y};
    double xu[4]={xymax.x,xymax.y,uvmax.x,uvmax.y};
    double res1=0;
    gsl_monte_function F0 = { &bandwidth_bound, 4, &params};        
	gsl_monte_plain_integrate(&F0, xl, xu, 4, NCALLS, (gsl_rng*)gsl_rng_r, (gsl_monte_plain_state*)gsl_monte_plane_state_s, &res1, error);    
    //printf ("%lf, %lf\n",res1,*error);
    return res1;
}

double SqrIntegral::computeWithGivenCutoff(int whichCutoff,const Vector3d&xymin, const Vector3d&xymax, const Vector3d &uvmin, const Vector3d &uvmax){
    double error=0.0;
    double renormalization = infiniteIntegral[whichCutoff]*(xymax.x-xymin.x)*(xymax.y-xymin.y);
    return integrate(cachedCutoff[whichCutoff],cachedFlatness[whichCutoff],xymin,xymax,uvmin,uvmax,&error)/renormalization;
}

double SqrIntegral::computeInfiniteIntegral(int whichCutoff) {
    double sum=0;
    double err_sum=0;
    int i;

    double xmul=cachedDimensions[whichCutoff][0];
    double ymul=cachedDimensions[whichCutoff][1];
    for (i=1;i<10000;i*=2) {
        int j;
        for ( j=1;j<10000;j*=2) {
            double error=0.0;
            sum+=integrate(cachedCutoff[whichCutoff],cachedFlatness[whichCutoff],Vector3d(0,0,0),Vector3d(xmul,ymul,1),Vector3d(i*xmul,j*ymul,0),Vector3d(i*2*xmul,j*2*ymul,1),&error);
            err_sum+=error;
        }
        double error =0.0;
        sum+=integrate(cachedCutoff[whichCutoff],cachedFlatness[whichCutoff],Vector3d(0,0,0),Vector3d(xmul,ymul,1),Vector3d(i*xmul,0,0),Vector3d(i*2*xmul,ymul,1),&error);
        err_sum+=error;       
   }
   if ((err_sum*err_sum)/(sum*sum)>.01) {
       fprintf(stderr,"Error is %f on absolute of %f\n",err_sum,sum);
   }
   return sum;
}

}

