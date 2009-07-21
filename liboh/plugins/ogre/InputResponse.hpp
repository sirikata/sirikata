/*  Sirikata liboh -- Ogre Graphics Plugin
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

#include <util/Platform.hpp>
#include "input/InputEvents.hpp"
#include "input/InputEventDescriptor.hpp"
#include "InputBindingEvent.hpp"

namespace Sirikata {
namespace Graphics {

/** Base class for input responses. Implementations will generally handle
 *  two things: wrap a generic callback, e.g. one requiring a float
 *  parameter, and handle translating events to that type of parameter.
 */
class InputResponse {
public:
    virtual ~InputResponse();

    /** Methods that invoke the response based on specific event types.
     *  They convert the event to the parameters needed by the reponse
     *  and then invoke it. The default implementations simply ignore
     *  the event.
     */
    virtual void invoke(Input::ButtonEventPtr& evt);
    virtual void invoke(Input::ButtonPressedEventPtr& evt);
    virtual void invoke(Input::ButtonReleasedEventPtr& evt);
    virtual void invoke(Input::ButtonDownEventPtr& evt);
    virtual void invoke(Input::AxisEventPtr& evt);
    virtual void invoke(Input::TextInputEventPtr& evt);
    virtual void invoke(Input::MouseHoverEventPtr& evt);
    virtual void invoke(Input::MouseClickEventPtr& evt);
    virtual void invoke(Input::MouseDragEventPtr& evt);
    virtual void invoke(Input::WindowEventPtr& evt);
    virtual void invoke(Input::DragAndDropEventPtr& evt);

    /** Invokes the input response for any type of InputEvent.  This should
     *  generally be avoided if you know the type of event, but if you don't
     *  know the type this might be handy.
     */
    void invoke(Input::InputEventPtr& evt);

    typedef std::vector<Input::EventDescriptor> InputEventDescriptorList;
    /** Get a list of InputEventDescriptors which specify the events that this
     *  response will handle, given a higher level description of the input
     *  to bind to this response.
     */
    virtual InputEventDescriptorList getInputEvents(const InputBindingEvent& descriptor) const = 0;
protected:
    /** Default action for events that aren't understood. */
    virtual void defaultAction();
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

    virtual void invoke(Input::ButtonPressedEventPtr& evt);
    virtual void invoke(Input::ButtonReleasedEventPtr& evt);

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

    virtual void invoke(Input::MouseClickEventPtr& evt);
    virtual void invoke(Input::MouseDragEventPtr& evt);

    virtual InputEventDescriptorList getInputEvents(const InputBindingEvent& descriptor) const;
private:
    ResponseCallback mCallback;
};

class AxisInputResponse : public InputResponse {
public:
    typedef std::tr1::function<void(float,Vector2f)> ResponseCallback;

    AxisInputResponse(ResponseCallback cb);

    virtual void invoke(Input::AxisEventPtr& evt);

    virtual InputEventDescriptorList getInputEvents(const InputBindingEvent& descriptor) const;
private:
    ResponseCallback mCallback;
};

} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_INPUT_RESPONSES_HPP_
