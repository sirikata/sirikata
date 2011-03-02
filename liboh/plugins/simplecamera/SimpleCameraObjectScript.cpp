/*  Sirikata
 *  SimpleCameraObjectScript.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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


#include <sirikata/oh/Platform.hpp>

#include <sirikata/core/util/KnownServices.hpp>


#include "SimpleCameraObjectScript.hpp"

namespace Sirikata {
namespace SimpleCamera {

SimpleCameraObjectScript::SimpleCameraObjectScript(HostedObjectPtr ho, const String& args)
 : mParent(ho),
   mGraphics(NULL)
{
    mParent->addListener((SessionEventListener*)this);

    // Setup input responses
    mInputResponses["suspend"] = new SimpleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::suspendAction, this));
    mInputResponses["resume"] = new SimpleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::resumeAction, this));
    mInputResponses["toggleSuspend"] = new SimpleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::toggleSuspendAction, this));
    mInputResponses["quit"] = new SimpleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::quitAction, this));
    mInputResponses["screenshot"] = new SimpleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::screenshotAction, this));
/*
    mInputResponses["moveForward"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::moveAction, this, Vector3f(0, 0, -1), _1), 1, 0);
    mInputResponses["moveBackward"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::moveAction, this, Vector3f(0, 0, 1), _1), 1, 0);
    mInputResponses["moveLeft"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::moveAction, this, Vector3f(-1, 0, 0), _1), 1, 0);
    mInputResponses["moveRight"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::moveAction, this, Vector3f(1, 0, 0), _1), 1, 0);
    mInputResponses["moveDown"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::moveAction, this, Vector3f(0, -1, 0), _1), 1, 0);
    mInputResponses["moveUp"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::moveAction, this, Vector3f(0, 1, 0), _1), 1, 0);

    mInputResponses["rotateXPos"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::rotateAction, this, Vector3f(1, 0, 0), _1), 1, 0);
    mInputResponses["rotateXNeg"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::rotateAction, this, Vector3f(-1, 0, 0), _1), 1, 0);
    mInputResponses["rotateYPos"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::rotateAction, this, Vector3f(0, 1, 0), _1), 1, 0);
    mInputResponses["rotateYNeg"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::rotateAction, this, Vector3f(0, -1, 0), _1), 1, 0);
    mInputResponses["rotateZPos"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::rotateAction, this, Vector3f(0, 0, 1), _1), 1, 0);
    mInputResponses["rotateZNeg"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::rotateAction, this, Vector3f(0, 0, -1), _1), 1, 0);

    mInputResponses["stableRotatePos"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::stableRotateAction, this, 1.f, _1), 1, 0);
    mInputResponses["stableRotateNeg"] = new FloatToggleInputResponse(std::tr1::bind(&SimpleCameraObjectScript::stableRotateAction, this, -1.f, _1), 1, 0);
*/
    mInputBinding.addFromFile("keybinding.default", mInputResponses);
}

SimpleCameraObjectScript::~SimpleCameraObjectScript()
{
    mParent->removeListener((SessionEventListener*)this);

    for (InputBinding::InputResponseMap::iterator iter = mInputResponses.begin(), iterend = mInputResponses.end(); iter != iterend; ++iter)
        delete iter->second;
}

void SimpleCameraObjectScript::updateAddressable()
{
}

void SimpleCameraObjectScript::attachScript(const String& script_name)
{
}

void SimpleCameraObjectScript::onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name, int token) {
    mGraphics = mParent->runSimulation(name, "ogregraphics");
    Invokable::Array args;
    args.push_back( (String)"setInputHandler" );
    args.push_back( (Invokable*)this );
    mGraphics->invoke(args);
}

void SimpleCameraObjectScript::onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name) {
}

boost::any SimpleCameraObjectScript::invoke(std::vector<boost::any>& params) {
    // Convert to an InputBindingEvent and handle it.
    assert(params.size() == 1);
    InputBindingEvent evt(params[0]);
    mInputBinding.handle(evt);
    return boost::any();
}


static String fillZeroPrefix(const String& prefill, int32 nwide) {
    String retval = prefill;
    while((int)retval.size() < nwide)
        retval = String("0") + retval;
    return retval;
}

void SimpleCameraObjectScript::suspendAction() {
    Invokable::Array args;
    args.push_back( (String)"suspend" );
    mGraphics->invoke(args);
}

void SimpleCameraObjectScript::resumeAction() {
    Invokable::Array args;
    args.push_back( (String)"suspend" );
    mGraphics->invoke(args);
}

void SimpleCameraObjectScript::toggleSuspendAction() {
    Invokable::Array args;
    args.push_back( (String)"toggleSuspend" );
    mGraphics->invoke(args);
}

void SimpleCameraObjectScript::screenshotAction() {
    Invokable::Array args;
    args.push_back( (String)"screenshot" );
    mGraphics->invoke(args);
}

void SimpleCameraObjectScript::quitAction() {
    Invokable::Array args;
    args.push_back( (String)"quit" );
    mGraphics->invoke(args);
}

} // namespace SimpleCamera
} // namespace Sirikata
