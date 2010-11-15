/*  Meru
 *  MeruDefs.hpp
 *
 *  Copyright (c) 2009, Stanford University
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

#include <sirikata/core/util/Platform.hpp>

#ifndef _MERU_DEFS_HPP_
#define _MERU_DEFS_HPP_

#include <string>
#include <vector>
#include <limits>
#include <map>
#include <queue>

#include <sirikata/core/util/Logging.hpp>

#include <sirikata/core/util/BoundingBox.hpp>
#include <sirikata/core/util/BoundingInfo.hpp>
#include <sirikata/core/util/BoundingSphere.hpp>
#include <sirikata/core/util/Location.hpp>
#include <sirikata/core/util/ObjectReference.hpp>
#include <sirikata/core/util/Quaternion.hpp>
#include <sirikata/core/util/SelfWeakPtr.hpp>
#include <sirikata/core/util/Sha256.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/util/SpaceID.hpp>
#include <sirikata/core/queue/ThreadSafeQueue.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/util/Vector3.hpp>
#include "../task/Event.hpp"
#include "../task/UniqueId.hpp"
#include <sirikata/core/transfer/TransferData.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include <sirikata/proxyobject/ProxyCameraObject.hpp>
#include <sirikata/mesh/LightInfo.hpp>

/*
#include <tr1/shared_ptr>
#include <tr1/weak_ptr>
#include <tr1/function>
*/

// Forward declare a class that we're going to import from another library and use in our own namespace
namespace Mid { class UUID; }

namespace Sirikata {
namespace Graphics {
class OgreSystem;
class Entity;
class LightEntity;
class MeshEntity;
class CameraEntity;
}
namespace Task {
class EventResponse;
}
}

/** Graphics dependency code borrowed from Meru. */
namespace Meru {

using ::Sirikata::Transfer::SparseData;
using ::Sirikata::Transfer::DenseDataPtr;
typedef SparseData MemoryBuffer;
typedef DenseDataPtr ResourceBuffer;

using ::Sirikata::String;

#ifdef _WIN32
inline bool isblank(int x){ return ((x == ' ') || (x == '\t')); }
#endif
///For when you want to allocate a class that explicitly has uninitialized components
class UninitializedMemory{};
extern UninitializedMemory uninitialized;


typedef std::vector<String> ParameterList;
class SystemParameters;

class Statistics;

typedef ::Sirikata::Vector3f Color;
using ::Sirikata::SelfWeakPtr;
using ::Sirikata::SpaceObjectReference;
using ::Sirikata::SpaceID;
using ::Sirikata::ObjectReference;
using ::Sirikata::Vector3;
using ::Sirikata::Vector3f;
using ::Sirikata::Vector3d;
using ::Sirikata::Quaternion;
using ::Sirikata::ThreadSafeQueue;
using ::Sirikata::LightInfo;
using ::Sirikata::Location;
using ::Sirikata::Time;
using ::Sirikata::Duration;
using ::Sirikata::ProxyCameraObject;
using ::Sirikata::ProxyMeshObject;
using ::Sirikata::BoundingBox;
using ::Sirikata::BoundingSphere;
using ::Sirikata::BoundingInfo;
using ::Sirikata::Graphics::OgreSystem;
using ::Sirikata::Graphics::Entity;
using ::Sirikata::Graphics::LightEntity;
using ::Sirikata::Graphics::MeshEntity;
using ::Sirikata::Graphics::CameraEntity;
using ::Sirikata::InitializeGlobalOptions;
using ::Sirikata::InitializeClassOptions;
using ::Sirikata::OptionValue;
using ::Sirikata::OptionValueType;

using ::Sirikata::Task::IdPair;
using ::Sirikata::Task::Event;
using ::Sirikata::Task::EventPtr;
using ::Sirikata::Task::SubscriptionId;
using ::Sirikata::Task::EventResponse;

typedef MeshEntity GraphicsEntity;


using ::Sirikata::UUID;
using ::Sirikata::URI;
using ::Sirikata::SHA256;
using ::Sirikata::Transfer::DenseData;
using ::Sirikata::Transfer::SparseData;
using ::Sirikata::Transfer::DenseDataPtr;
using ::Sirikata::Transfer::RemoteFileId;

namespace Transfer = ::Sirikata::Transfer;

typedef ::Sirikata::Transfer::RemoteFileId ResourceHash;

using ::Sirikata::int64;
using ::Sirikata::int32;
using ::Sirikata::int16;
using ::Sirikata::int8;
using ::Sirikata::uint64;
using ::Sirikata::uint32;
using ::Sirikata::uint16;
using ::Sirikata::uint8;
using ::Sirikata::float32;
using ::Sirikata::float64;

using ::std::tr1::placeholders::_1;
using ::std::tr1::placeholders::_2;
using ::std::tr1::placeholders::_3;
using ::std::tr1::placeholders::_4;
using ::std::tr1::placeholders::_5;
using ::std::tr1::placeholders::_6;
using ::std::tr1::placeholders::_7;
using ::std::tr1::placeholders::_8;
using ::std::tr1::placeholders::_9;

#define CURRENT_TIME (Time::now())

namespace Digest {

    using ::Sirikata::SHA256;

} // namespace Digest



} // namespace Meru

#endif //_MERU_DEFS_HPP_
