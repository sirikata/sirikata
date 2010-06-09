

#include "OSegTestMotionPath.hpp"
#include "Random.hpp"

#include <algorithm>
#include <cmath>


namespace Sirikata {


OSegTestMotionPath::OSegTestMotionPath(const Time& start, const Time& end, const Vector3f& startpos, float32 speed, const Duration& update_period, const BoundingBox3f& region, float zfactor, Vector3f driftDir)
  //OSegTestMotionPath::OSegTestMotionPath(const Time& start, const Time& end, const Vector3f& startpos, float32 speed, const Duration& update_period, const BoundingBox3f& region, float zfactor)
{
  TimedMotionVector3f last_motion(start, MotionVector3f(startpos, Vector3f(0,0,0)));
  mUpdates.push_back(last_motion);

  Time offset_start = start + update_period * randFloat();

  for(Time curtime = offset_start; curtime < end; curtime += update_period)
  {
    Vector3f curpos = last_motion.extrapolate(curtime).position();
    
    //    Vector3f dir = UniformSampleSphere( randFloat(), randFloat() ) * speed;
    //    dir.z = dir.z * zfactor;
    
    //    Vector3f dir = Vector3f(.3,.3,.3);
    //    dir = dir.normal() *  speed;
    //    last_motion = TimedMotionVector3f(curtime, MotionVector3f(curpos, dir));

    Vector3f dir = driftDir;
    //dir = dir.normal() *  speed;
    last_motion = TimedMotionVector3f(curtime, MotionVector3f(curpos, dir));
    
    // last_motion is what we would like, no we have to make sure we'll stay  in range
    Vector3f nextpos = last_motion.extrapolate(curtime + update_period).position();
    Vector3f nextpos_clamped = region.clamp(nextpos);
    Vector3f to_dest = nextpos_clamped - curpos;
    dir = to_dest * (1.f / update_period.toSeconds());

    last_motion = TimedMotionVector3f(curtime, MotionVector3f(curpos, dir));
    
    mUpdates.push_back(last_motion);
  }
}


const TimedMotionVector3f OSegTestMotionPath::initial() const {
    assert( !mUpdates.empty() );
    return mUpdates[0];
}
  

const TimedMotionVector3f* OSegTestMotionPath::nextUpdate(const Time& curtime) const {
    for(uint32 i = 0; i < mUpdates.size(); i++)
        if (mUpdates[i].time() > curtime) return &mUpdates[i];
    return NULL;
}

  
const TimedMotionVector3f OSegTestMotionPath::at(const Time& t) const {
    for(uint32 i = 1; i < mUpdates.size(); i++)
        if (mUpdates[i].time() > t) return mUpdates[i-1];
    return mUpdates[ mUpdates.size()-1 ];
}


} // namespace Sirikata
