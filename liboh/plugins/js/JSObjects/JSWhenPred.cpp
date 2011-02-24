#include <v8.h>

#include "JSObjectsUtils.hpp"
#include <cassert>
#include <sirikata/core/util/Platform.hpp>
#include "JSWhenPred.hpp"
#include <math.h>


namespace Sirikata{
namespace JS{



//dist(a,b) < g
//timeToEval is going to be when the above condition will first be true.
bool JSWhenPred::whenToEval(JSVisibleStruct* a, JSVisibleStruct* b, float g, float& timeToEval)
{
    Vector3d posA = a->returnProxyPositionCPP();
    Vector3d posB = b->returnProxyPositionCPP();

    Vector3d velA = a->returnProxyVelocityCPP();
    Vector3d velB = b->returnProxyVelocityCPP();

    float root1, root2;
    bool hasRoot = computeDistPredFirstRoot(
        posA.x - posB.x,velA.x - velB.x,
        posA.y - posB.y,velA.y - velB.y,
        posA.z - posB.z,velA.z - velB.z,
        root1, root2);

    if ( ! ((a->getStillVisibleCPP()) && (b->getStillVisibleCPP())))
        JSLOG(info,"Warning.  Evaluating when predicate for object that is not visible");


    if ((root1>=0) && (root2 <0))
    {
        timeToEval = root1;
        return true;
    }

    if ((root2>=0) && (root1 <0))
    {
        timeToEval = root2;
        return true;
    }

    if ((root1 >= 0) && (root1 <= root2))
    {
        timeToEval = root1;
        return true;
    }

    if ((root2 >=0) && (root2<=root1))
    {
        timeToEval = root2;
        return true;
    }
        
    
    return false;
}


//k1*sqrt( (a1 + a2*t)^2 + (b1 + b2*t)^2 + (c1+c2*t)^2) + g
bool JSWhenPred::computeDistPredFirstRoot(float a1, float a2, float b1, float b2, float c1, float c2, float k1, flaot g, float& root1, float& root2)
{
    //want to get it into form:
    // a*t^2 + b*t + c = 0;

    float k1Sq = k1*k1;
    float gSq  = g*g;
    float a = k1Sq* ( a2*a2 + b2*b2 + c2*c2);
    float b = 2*k1Sq* (a2*a1 + b2*b1 + c2*c1);
    float c = -gSq + (a1*a1 + b1*b1 + b2*b2);


    float toSqrt = b*b - 4*a*c;
    if (toSqrt < 0)
        return false;

    root1 = (-b -  sqrt(toSqrt))/ (2*a);
    root2 = (-b +  sqrt(toSqrt))/ (2*a);
    return true;
}





}//namespace js
}//namespace sirikata

