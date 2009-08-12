/*  Sirikata Ogre Plugin
 *  CameraPath.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of Sirikata nor the names of its contributors may
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

#ifndef _SIRIKATA_OGRE_CAMERA_PATH_HPP_
#define _SIRIKATA_OGRE_CAMERA_PATH_HPP_

#include <util/Platform.hpp>
#include <task/Time.hpp>

namespace Sirikata {
namespace Graphics {

struct CameraPoint {
    Vector3d position;
    Quaternion orientation;
    Task::DeltaTime dt;
    Task::DeltaTime time;

    CameraPoint(const Vector3d& pos, const Quaternion& orient, const Task::DeltaTime& _dt)
     : position(pos),
       orientation(orient),
       dt(_dt),
       time(Task::DeltaTime::zero())
    {
    }
}; // struct CameraPoint


class CameraPath {
public:
    CameraPath();

    CameraPoint& operator[](int idx);
    const CameraPoint& operator[](int idx) const;

    uint32 numPoints() const;
    bool empty() const;

    Task::DeltaTime startTime() const;
    Task::DeltaTime endTime() const;

    int32 clampKeyIndex(int32 idx) const;

    Task::DeltaTime keyFrameTime(int32 idx) const;
    void changeTimeDelta(int32 idx, const Task::DeltaTime& d_dt);

    int32 insert(int32 idx, const Vector3d& pos, const Quaternion& orient, const Task::DeltaTime& dt);
    int32 remove(int32 idx);

    void load(const String& filename);
    void save(const String& filename);

    void normalizePath();
    void computeDensities();
    void computeTimes();

    bool evaluate(const Task::DeltaTime& t, Vector3d* pos_out, Quaternion* orient_out);
private:
    std::vector<CameraPoint> mPathPoints;
    std::vector<double> mDensities;
    bool mDirty;
};

} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_CAMERA_PATH_HPP_
