/*  Sirikata
 *  InputBindingEvent.hpp
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

#ifndef _SIRIKATA_INPUT_BINDING_EVENT_HPP_
#define _SIRIKATA_INPUT_BINDING_EVENT_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/proxyobject/Invokable.hpp>

namespace Sirikata {
namespace SimpleCamera {

class InputBindingEvent {
public:
    enum Modifier {
        NONE = 0,
        SHIFT = 1,
        CTRL = 2,
        ALT = 4,
        SUPER = 8
    };

    static InputBindingEvent Key(String button, Modifier mod);
    static InputBindingEvent MouseClick(int32 button);
    static InputBindingEvent MouseDrag(int32 button);
    static InputBindingEvent Axis(uint32 index);
    static InputBindingEvent Web(const String& wvname, const String& name);

    InputBindingEvent();
    InputBindingEvent(const InputBindingEvent& rhs);
    InputBindingEvent(const boost::any& evt);
    ~InputBindingEvent();

    bool valid() const;

    bool isKey() const;
    String keyButton() const;
    Modifier keyModifiers() const;
    bool keyPressed() const;
    bool keyReleased() const;

    bool isMouseClick() const;
    int32 mouseClickButton() const;

    bool isMouseDrag() const;
    int32 mouseDragButton() const;

    float mouseX() const;
    float mouseY() const;

    bool isAxis() const;
    uint32 axisIndex() const;
    float axisValue() const;

    bool isWeb() const;
    String webViewName() const;
    String webName() const;

    InputBindingEvent& operator=(const InputBindingEvent& rhs);

    String toString() const;
    static InputBindingEvent fromString(const String& asString);

    // Checks if the events "match", which is looser than being equal, allowing
    // a binding to pair an actual event with an event template for a handler.
    bool matches(const InputBindingEvent& rhs) const;

    bool operator<(const InputBindingEvent& rhs) const;
private:
    static String keyModifiersAsString(Modifier m);
    static Modifier keyModifiersFromString(const String& s);

    // Get a type tag to help with comparisons
    int32 typeTag() const;

    Invokable::Dict mEvent;
};

std::istream& operator>>(std::istream& is, Sirikata::SimpleCamera::InputBindingEvent& ibe);
std::ostream& operator<<(std::ostream& os, const Sirikata::SimpleCamera::InputBindingEvent& ibe);

} // namespace SimpleCamera
} // namespace Sirikata

#endif //_SIRIKATA_INPUT_BINDING_EVENT_HPP_
