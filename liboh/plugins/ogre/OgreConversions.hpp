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
    return (pos - base).convert<Ogre::Vector3>();
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
    return rgb.convert<Ogre::ColourValue>();
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
#endif
