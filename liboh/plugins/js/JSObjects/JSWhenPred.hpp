
#ifndef __SIRIKATA_JS_WHENPRED_HPP__
#define __SIRIKATA_JS_WHENPRED_HPP__

#include <v8.h>
#include <sirikata/core/util/Platform.hpp>
#include <tr1/functional>

namespace Sirikata{
namespace JS{

class JSWhenPred
{
public:


    bool whenToEval(JSVisibleStruct* a, JSVisibleStruct* b, float g, float& timeToEval);
    
    //Finds roots for
    //k1*sqrt( (a1 + a2*t)^2 + (b1 + b2*t)^2 + (c1+c2*t)^2) + g
    bool computeDistPredFirstRoot(float a1, float a2, float b1, float b2, float c1, float c2, float k1, flaot g, float& root1, float& root2);

private:
    
};





} //end namespace js
} //end namespace sirikata

#endif
