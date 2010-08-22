/*  Sirikata
 *  ProxSimulationTraits.hpp
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

#ifndef _SIRIKATA_PROX_SIMULATION_TRAITS_HPP_
#define _SIRIKATA_PROX_SIMULATION_TRAITS_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/util/MotionVector.hpp>

#include <sirikata/space/Platform.hpp>

namespace Sirikata {

class SIRIKATA_SPACE_EXPORT ProxSimulationTraits {
public:
    typedef float32 realType;

    typedef Vector3f Vector3Type;
    typedef TimedMotionVector3f MotionVector3Type;

    typedef BoundingSphere3f BoundingSphereType;

    typedef SolidAngle SolidAngleType;

    typedef Time TimeType;
    typedef Duration DurationType;

    const static realType InfiniteRadius;
}; // class ProxSimulationTraits

class SIRIKATA_SPACE_EXPORT ObjectProxSimulationTraits : public ProxSimulationTraits {
public:
    typedef UUID ObjectIDType;
    typedef UUID::Hasher ObjectIDHasherType;
};

class SIRIKATA_SPACE_EXPORT ServerProxSimulationTraits : public ProxSimulationTraits {
public:
    typedef ServerID ObjectIDType;
    typedef std::tr1::hash<ServerID> ObjectIDHasherType;
};

} // namespace Sirikata

#endif //_SIRIKATA_PROX_SIMULATION_TRAITS_HPP_
