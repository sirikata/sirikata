/*  Sirikata Graphical Object Host
 *  OgreConversions.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#ifndef OGRE_CONVERSIONS_HPP
#define OGRE_CONVERSIONS_HPP

namespace Sirikata {
typedef Vector4f ColorAlpha;
typedef Vector3f Color;
namespace Graphics {
inline Ogre::Quaternion toOgre(const Sirikata::Quaternion &quat) {
    Ogre::Quaternion ret;
    ret.w = quat.w;
    ret.x = quat.x;
    ret.y = quat.y;
    ret.z = quat.z;
    return ret;
}

inline Ogre::Vector3 toOgre(const Sirikata::Vector3f &pos) {
    return pos.convert<Ogre::Vector3>();
}

// Ogre uses floating points internally. Base should be equal to the translation of the scene.
inline Ogre::Vector3 toOgre(const Sirikata::Vector3d &pos, const Sirikata::Vector3d &base) {
    return (pos - base).downCast<Ogre::Real>().convert<Ogre::Vector3>();
}

inline Ogre::Vector4 toOgre(const Sirikata::Vector4f &pos) {
    return pos.convert<Ogre::Vector4>();
}

inline Ogre::ColourValue toOgreRGBA(const Sirikata::ColorAlpha &rgba) {
    return rgba.convert<Ogre::ColourValue>();
}

inline Ogre::ColourValue toOgreRGB(const Sirikata::Color &rgb) {
    return rgb.convert<Ogre::ColourValue>();
}

inline Ogre::ColourValue toOgreRGBA(const Sirikata::Color &rgb, float32 alpha) {
    return Ogre::ColourValue(rgb[0], rgb[1], rgb[2], alpha);
}

inline Sirikata::Quaternion fromOgre(const Ogre::Quaternion &quat) {
    return Sirikata::Quaternion(quat.x,quat.y,quat.z,quat.w,Quaternion::XYZW());
}

inline Sirikata::Vector3f fromOgre(const Ogre::Vector3 &pos) {
    return Sirikata::Vector3f(pos);
}

inline Sirikata::Vector3d fromOgre(const Ogre::Vector3 &pos, const Vector3d &base) {
    return Sirikata::Vector3d(pos) + base;
}

inline Sirikata::Vector4f fromOgre(const Ogre::Vector4 &pos) {
    return Sirikata::Vector4f(pos.x,pos.y,pos.z,pos.w);
}

inline Sirikata::ColorAlpha fromOgreRGBA(const Ogre::ColourValue &rgba) {
    return Sirikata::ColorAlpha(rgba.r,rgba.b,rgba.g,rgba.a);
}

inline Sirikata::Color fromOgreRGB(const Ogre::ColourValue &rgba) {
    return Sirikata::Color(rgba.r,rgba.g,rgba.b);
}
}
}
#endif // OGRE_CONVERSIONS_HPP
