
#ifndef _CBR_OSEG_TEST_MOTION_PATH_HPP_
#define _CBR_OSEG_TEST_MOTION_PATH_HPP_


#include "Utility.hpp"
#include "MotionPath.hpp"


namespace CBR
{

/** Static motion path, i.e. not a motion path at all.  Just has a single initial
 *  update which has 0 velocityy.
 */
  class OSegTestMotionPath : public MotionPath
  {
  public:

    OSegTestMotionPath(const Time& start, const Time& end, const Vector3f& startpos, float32 speed, const Duration& update_period, const BoundingBox3f& region, float zfactor);

  
    virtual const TimedMotionVector3f initial() const;
    virtual const TimedMotionVector3f* nextUpdate(const Time& curtime) const;
    virtual const TimedMotionVector3f at(const Time& t) const;
  private:
    std::vector<TimedMotionVector3f> mUpdates;
    TimedMotionVector3f mMotion;

  }; // class StaticMotionPath

} // namespace CBR

#endif //_CBR_STATIC_MOTION_PATH_HPP_




