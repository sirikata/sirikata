/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  InputResponse.hpp
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

#ifndef _SIRIKATA_INPUT_RESPONSES_HPP_
#define _SIRIKATA_INPUT_RESPONSES_HPP_

#include <sirikata/core/util/Platform.hpp>
#include "InputBindingEvent.hpp"

namespace Sirikata {
namespace SimpleCamera {

/** Base class for input responses. Implementations will generally handle
 *  two things: wrap a generic callback, e.g. one requiring a float
 *  parameter, and handle translating events to that type of parameter.
 */
class InputResponse {
public:
    virtual ~InputResponse();

    /** Invokes the input response for any type of InputEvent. */
    virtual void invoke(InputBindingEvent& evt);

    typedef std::vector<InputBindingEvent> InputEventDescriptorList;
    /** Get a list of InputEventDescriptors which specify the events that this
     *  response will handle, given a higher level description of the input
     *  to bind to this response.
     */
    virtual InputEventDescriptorList getInputEvents(const InputBindingEvent& descriptor) const = 0;

protected:
    virtual void defaultAction() {}
};

class SimpleInputResponse : public InputResponse {
public:
    typedef std::tr1::function<void(void)> ResponseCallback;

    SimpleInputResponse(ResponseCallback cb);

    virtual InputEventDescriptorList getInputEvents(const InputBindingEvent& descriptor) const;
protected:
    virtual void defaultAction();

private:
    ResponseCallback mCallback;
};

class FloatToggleInputResponse : public InputResponse {
public:
    typedef std::tr1::function<void(float)> ResponseCallback;

    FloatToggleInputResponse(ResponseCallback cb, float onval, float offval);

    virtual void invoke(InputBindingEvent& evt);

    virtual InputEventDescriptorList getInputEvents(const InputBindingEvent& descriptor) const;
private:
    ResponseCallback mCallback;
    float mOnValue;
    float mOffValue;
};

class Vector2fInputResponse : public InputResponse {
public:
    typedef std::tr1::function<void(Vector2f)> ResponseCallback;

    Vector2fInputResponse(ResponseCallback cb);

    virtual void invoke(InputBindingEvent& evt);

    virtual InputEventDescriptorList getInputEvents(const InputBindingEvent& descriptor) const;
private:
    ResponseCallback mCallback;
};

class AxisInputResponse : public InputResponse {
public:
    typedef std::tr1::function<void(float)> ResponseCallback;

    AxisInputResponse(ResponseCallback cb);

    virtual void invoke(InputBindingEvent& evt);

    virtual InputEventDescriptorList getInputEvents(const InputBindingEvent& descriptor) const;
private:
    ResponseCallback mCallback;
};

} // namespace SimpleCamera
} // namespace Sirikata

#endif //_SIRIKATA_INPUT_RESPONSES_HPP_
