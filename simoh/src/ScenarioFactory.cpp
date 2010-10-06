/*  Sirikata Network Services - Scenario factory
 *  ScenarioFactory.cpp
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
#include <sirikata/core/util/Platform.hpp>
#include "ScenarioFactory.hpp"
#include "DistributionPingScenario.hpp"
#include "DelugePairScenario.hpp"
#include "PingDelugeScenario.hpp"
#include "ByteTransferScenario.hpp"
#include "LoadPacketTrace.hpp"
#include "NullScenario.hpp"
#include "HitPointScenario.hpp"
#include "UnreliableHitPointScenario.hpp"
#include "OSegScenario.hpp"
#include "AirTrafficControllerScenario.hpp"
AUTO_SINGLETON_INSTANCE(Sirikata::ScenarioFactory);
namespace Sirikata {
ScenarioFactory::ScenarioFactory(){
    DistributionPingScenario::addConstructorToFactory(this);
    DelugePairScenario::addConstructorToFactory(this);
    PingDelugeScenario::addConstructorToFactory(this);
    OSegScenario::addConstructorToFactory(this);
    LoadPacketTrace::addConstructorToFactory(this);
    ByteTransferScenario::addConstructorToFactory(this);
    NullScenario::addConstructorToFactory(this);
    HitPointScenario::addConstructorToFactory(this);
    UnreliableHitPointScenario::addConstructorToFactory(this);
    AirTrafficControllerScenario::addConstructorToFactory(this);
}
ScenarioFactory::~ScenarioFactory(){}
ScenarioFactory&ScenarioFactory::getSingleton(){
    return Sirikata::AutoSingleton<ScenarioFactory>::getSingleton();
}
void ScenarioFactory::destroy(){
    Sirikata::AutoSingleton<ScenarioFactory>::destroy();
}

}
