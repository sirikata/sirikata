// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBOH_MANUAL_QUERY_PINTO_PROX_SIMULATION_TRAITS_HPP_
#define _SIRIKATA_LIBOH_MANUAL_QUERY_PINTO_PROX_SIMULATION_TRAITS_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/ObjectReference.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/UniqueID.hpp>

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

    typedef UniqueID32 UniqueIDGeneratorType;
}; // class ProxSimulationTraits

class ObjectProxSimulationTraits : public ProxSimulationTraits {
public:
    typedef ObjectReference ObjectIDType;
    typedef ObjectReference::Hasher ObjectIDHasherType;
    typedef ObjectReference::Null ObjectIDNullType;
    typedef ObjectReference::Random ObjectIDRandomType;
};

} // namespace Sirikata

#endif //_SIRIKATA_LIBOH_MANUAL_QUERY_PINTO_PROX_SIMULATION_TRAITS_HPP_
