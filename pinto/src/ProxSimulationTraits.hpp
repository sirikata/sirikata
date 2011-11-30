// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_PINTO_PROX_SIMULATION_TRAITS_HPP_
#define _SIRIKATA_PINTO_PROX_SIMULATION_TRAITS_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/MotionVector.hpp>

namespace Sirikata {

class ProxSimulationTraits {
public:
    typedef uint32 intType;
    typedef float32 realType;

    typedef Vector3f Vector3Type;
    typedef TimedMotionVector3f MotionVector3Type;

    typedef BoundingSphere3f BoundingSphereType;

    typedef SolidAngle SolidAngleType;

    typedef Time TimeType;
    typedef Duration DurationType;

    const static realType InfiniteRadius;
    const static intType InfiniteResults;
}; // class ProxSimulationTraits

class ServerProxSimulationTraits : public ProxSimulationTraits {
public:
    typedef ServerID ObjectIDType;
    typedef std::tr1::hash<ServerID> ObjectIDHasherType;
    typedef ServerIDNull ObjectIDNullType;
    typedef ServerIDRandom ObjectIDRandomType;
};

} // namespace Sirikata

#endif //_SIRIKATA_PINTO_PROX_SIMULATION_TRAITS_HPP_
