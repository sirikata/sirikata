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


#include "CameraPath.hpp"
#include <boost/filesystem.hpp>
#include <math.h>
#include <stdlib.h>


namespace Sirikata {
namespace Graphics {

using namespace std;
using namespace Sirikata::Task;

CameraPath::CameraPath()
 : mDirty(true)
{
}

CameraPoint& CameraPath::operator[](int idx) {
    return mPathPoints[idx];
}

const CameraPoint& CameraPath::operator[](int idx) const {
    return mPathPoints[idx];
}

uint32 CameraPath::numPoints() const {
    return mPathPoints.size();
}

bool CameraPath::empty() const {
    return (mPathPoints.size() == 0);
}

DeltaTime CameraPath::startTime() const {
    return empty() ? DeltaTime::zero() : (*this)[0].time;
}

DeltaTime CameraPath::endTime() const {
    return empty() ? DeltaTime::zero() : (*this)[numPoints()-1].time;
}

int32 CameraPath::clampKeyIndex(int32 idx) const {
    if (idx < 0) return 0;
    if (idx >= (int32)numPoints()) return numPoints()-1;
    return idx;
}

DeltaTime CameraPath::keyFrameTime(int32 idx) const {
    if (idx < 0 || idx >= (int32)numPoints()) return DeltaTime::zero();
    return mPathPoints[idx].time;
}

void CameraPath::changeTimeDelta(int32 idx, const DeltaTime& d_dt) {
    if (idx < 0 || idx >= (int32)numPoints()) return;

    DeltaTime old_dt = mPathPoints[idx].dt;
    DeltaTime new_dt = old_dt + d_dt;
    if (new_dt < DeltaTime::zero()) new_dt = DeltaTime::zero();
    mPathPoints[idx].dt = new_dt;

    mDirty = true;
}

int32 CameraPath::insert(int32 idx, const Vector3d& pos, const Quaternion& orient, const DeltaTime& dt) {
    mDirty = true;

    CameraPoint cp(pos, orient, dt);
    int insert_idx = empty() ? 0 : idx + 1;
    mPathPoints.insert( mPathPoints.begin()+insert_idx, cp);

    // everything after this point needs its time shifted
    for(uint32 after_idx = insert_idx + 1; after_idx < mPathPoints.size(); after_idx++)
        mPathPoints[after_idx].time += dt;

    return insert_idx;
}

// returns the closest available key point index after the removal
int32 CameraPath::remove(int32 idx) {
    if (idx < 0 || idx >= (int32)numPoints())
        return idx;

    mDirty = true;

    mPathPoints.erase( mPathPoints.begin()+idx );

    return clampKeyIndex(idx);
}

void CameraPath::load(const String& filename) {
    mPathPoints.clear();

    if (!boost::filesystem::exists(filename)) {
        SILOG(ogre,error,"Error loading camera path -- file doesn't exist");
        return;
    }

    FILE* pathfile = fopen(filename.c_str(), "r");
    if (!pathfile) {
        SILOG(ogre,error,"Error loading camera path -- couldn't open file");
        return;
    }

    while(true) {
        float32 posx, posy, posz;
        float32 orientx, orienty, orientz, orientw;
        float32 dt;
        int nitems = fscanf(pathfile, "(%f %f %f) (%f %f %f %f) (%f)",
            &posx, &posy, &posz,
            &orientx, &orienty, &orientz, &orientw,
            &dt
        );
        if (nitems != 8) break;
        CameraPoint cp(
            Vector3d(posx, posy, posz),
            Quaternion(orientx, orienty, orientz, orientw, Quaternion::XYZW()),
            DeltaTime::seconds(dt)
        );
        mPathPoints.push_back(cp);
    }

    fclose(pathfile);

    mDirty = true;
}


void CameraPath::save(const String& filename) {
    FILE* pathfile = fopen(filename.c_str(), "w");
    if (!pathfile) {
        SILOG(ogre,error,"Error saving camera path -- couldn't open file");
        return;
    }

    for(uint32 i = 0; i < mPathPoints.size(); i++) {
        fprintf(pathfile, "(%f %f %f) (%f %f %f %f) (%f)\n",
            mPathPoints[i].position.x, mPathPoints[i].position.y, mPathPoints[i].position.z,
            mPathPoints[i].orientation.x, mPathPoints[i].orientation.y, mPathPoints[i].orientation.z, mPathPoints[i].orientation.w,
            mPathPoints[i].dt.toSeconds()
        );
    }

    fclose(pathfile);
}

void CameraPath::normalizePath() {
    computeTimes();
    computeDensities();
}

void CameraPath::computeDensities() {
    int k = 3;

    mDensities.clear();

    for(int32 idx = 0; idx < (int32)mPathPoints.size(); idx++) {
        uint32 min_idx = clampKeyIndex(idx - k);
        uint32 max_idx = clampKeyIndex(idx + k);

        DeltaTime min_time = (*this)[min_idx].time;
        DeltaTime max_time = (*this)[max_idx].time;

        double didx = (double)(max_idx - min_idx + 1);
        DeltaTime dt = max_time - min_time;

        if (dt == DeltaTime::zero())
            mDensities.push_back(100.0);
        else
            mDensities.push_back( didx / dt.toSeconds() );
    }
}

void CameraPath::computeTimes() {
    DeltaTime t = DeltaTime::zero();
    for(uint32 idx = 0; idx < mPathPoints.size(); idx++) {
        (*this)[idx].time = t;
        t += (*this)[idx].dt;
    }
}

bool CameraPath::evaluate(const DeltaTime& t, Vector3d* pos_out, Quaternion* orient_out) {
    if (mDirty) {
        normalizePath();
        mDirty = false;
    }

    uint32 idx = 0;
    while(idx+1 < numPoints() && keyFrameTime(idx+1) < t)
        idx++;

    if (idx+1 >= numPoints() || t > keyFrameTime(idx+1))
        return false;

    Vector3d pos_sum(0.0f, 0.0f, 0.0f);
    Quaternion orient_sum(0.0f, 0.0f, 0.0f, 0.0f, Quaternion::XYZW());
    float weight_sum = 0.0f;

    for(int32 i = 0; i < (int32)mPathPoints.size(); i++) {
        double difft = 0;
        if (keyFrameTime(i) > t)
            difft = (keyFrameTime(i) - t).toSeconds();
        else
            difft = (t - keyFrameTime(i)).toSeconds();
        float stddev = 1.0f / (float)mDensities[i];
        float weight = (float)exp( - difft * difft / (stddev*stddev));
        pos_sum += mPathPoints[i].position * weight;
        orient_sum += mPathPoints[i].orientation * weight;
        weight_sum += weight;
    }

    if (weight_sum == 0) return false;

    if (pos_out != NULL) *pos_out = pos_sum * (1.0f/weight_sum);
    if (orient_out != NULL) *orient_out = (orient_sum * (1.0f/weight_sum)).normal();

    return true;
}


} // namespace Graphics
} // namespace Sirikata
