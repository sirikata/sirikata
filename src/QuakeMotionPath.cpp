/*  cbr
 *  QuakeMotionPath.cpp
 *
 *  Copyright (c) 2009, Tahir Azim
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of cbr nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "QuakeMotionPath.hpp"
#include <algorithm>
#include <cmath>
#include <boost/tokenizer.hpp>

#include "Random.hpp"

#ifndef PI
#define PI 3.14159f
#endif

#define FLT_MIN -1e30f
#define FLT_MAX 1e30f

namespace CBR {


QuakeMotionPath::QuakeMotionPath( const char* quakeDataTraceFile, float scaleDownFactor,
				  const BoundingBox3f& region )
{
    std::ifstream inputFile(quakeDataTraceFile);

    String str1="", str2="";

    float minX=FLT_MAX, maxX=FLT_MIN;
    float minY=FLT_MAX, maxY=FLT_MIN;
    float minZ=FLT_MAX, maxZ=FLT_MIN;

    if ( getline(inputFile, str1) ) {

        while ( getline(inputFile, str2)) {	    
  	    TimedMotionVector3f motionVector = parseTraceLines(str1, str2, scaleDownFactor);
	    mUpdates.push_back(motionVector);	    

	    if (motionVector.value().position().x < minX) minX = motionVector.value().position().x;
	    if (motionVector.value().position().y < minY) minY = motionVector.value().position().y;
	    if (motionVector.value().position().z < minZ) minZ = motionVector.value().position().z;

	    if (motionVector.value().position().x > maxX) maxX = motionVector.value().position().x;
	    if (motionVector.value().position().y > maxY) maxY = motionVector.value().position().y;
	    if (motionVector.value().position().z > maxZ) maxZ = motionVector.value().position().z;

	    str1 = str2;
        }

	float maxRatio = (maxX - minX) / (region.max().x - region.min().x);

	if ( (maxY - minY)/(region.max().y - region.min().y) > maxRatio)
	    maxRatio = (maxY - minY) / (region.max().y - region.min().y);

	if ((maxZ - minZ)/(region.max().z - region.min().z) > maxRatio)
	    maxRatio = (maxZ - minZ) / (region.max().z - region.min().z);

	if (maxRatio > 1.f) {
	    for (uint32_t i = 0; i < mUpdates.size(); i++) {	      
	      mUpdates[i] = TimedMotionVector3f(mUpdates[i].time(),
                                                MotionVector3f(
                                                region.clamp(mUpdates[i].value().position()/maxRatio),
                                                mUpdates[i].value().velocity()/maxRatio
                                                ));
	    }
	}
    }
}

const TimedMotionVector3f QuakeMotionPath::initial() const {
    assert( !mUpdates.empty() );
    return mUpdates[0];
}

const TimedMotionVector3f* QuakeMotionPath::nextUpdate(const Time& curtime) const {
    for(uint32 i = 0; i < mUpdates.size(); i++) {
        if (mUpdates[i].time() > curtime) {	 
	    return &mUpdates[i];
        }
    }
    return NULL;
}

TimedMotionVector3f QuakeMotionPath::parseTraceLines(String firstLine, String secondLine, float scaleDownFactor) {
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(",: ");
    boost::tokenizer<boost::char_separator<char> > tokens(firstLine, sep);

    tokenizer::iterator tok_iter = tokens.begin();

    int id = atoi((*tok_iter).c_str());

    ++tok_iter;
    float x = atof((*tok_iter).c_str());

    ++tok_iter;
    float y = atof((*tok_iter).c_str());

    ++tok_iter;
    float z = atof((*tok_iter).c_str());

    ++tok_iter;
    int t1 = atoi((*tok_iter).c_str());

    static int start_time = t1;

    Vector3f cur_pos = Vector3f(x, y, z) / scaleDownFactor;

    boost::tokenizer<boost::char_separator<char> > tokens2(secondLine, sep);

    tok_iter = tokens2.begin();

    id = atoi((*tok_iter).c_str());

    ++tok_iter;
    x = atof((*tok_iter).c_str());

    ++tok_iter;
    y = atof((*tok_iter).c_str());

    ++tok_iter;
    z = atof((*tok_iter).c_str());

    ++tok_iter;
    int t2 = atoi((*tok_iter).c_str());

    Vector3f next_pos = Vector3f(x, y, z) / scaleDownFactor;

    Vector3f vel = (next_pos - cur_pos) * 1.f / (t2 - t1);

    return TimedMotionVector3f(Time( (t1 - start_time)*1000.0f), MotionVector3f(cur_pos, vel) );
}


} // namespace CBR
