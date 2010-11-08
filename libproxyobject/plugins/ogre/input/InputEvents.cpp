/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  InputEvents.cpp
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

#include <sirikata/core/util/Standard.hh>
#include "InputEvents.hpp"

namespace Sirikata {
namespace Input {

EventDescriptor WindowEvent::getDescriptor() const {
    WindowEventType t = (WindowEventType)-1;
    if (getId().primary() == Shown())
        t = WindowShown;
    else if (getId().primary() == Hidden())
        t = WindowHidden;
    else if (getId().primary() == Exposed())
        t = WindowExposed;
    else if (getId().primary() == Moved())
        t = WindowMoved;
    else if (getId().primary() == Resized())
        t = WindowResized;
    else if (getId().primary() == Minimized())
        t = WindowMinimized;
    else if (getId().primary() == Maximized())
        t = WindowMaximized;
    else if (getId().primary() == Restored())
        t = WindowRestored;
    else if (getId().primary() == MouseEnter())
        t = WindowMouseEnter;
    else if (getId().primary() == MouseLeave())
        t = WindowMouseLeave;
    else if (getId().primary() == FocusGained())
        t = WindowFocusGained;
    else if (getId().primary() == FocusLost())
        t = WindowFocusLost;
    else if (getId().primary() == Quit())
        t = WindowQuit;
    else
        assert(false);

    return EventDescriptor::Window(t);
}


WebViewEvent::WebViewEvent(const String &wvName, const String& _name, const std::vector<String>& _args)
 : InputEvent(InputDeviceWPtr(), IdPair(getEventId(), _name)),
   webview(wvName),
   name(_name),
   args(_args)
{
}



WebViewEvent::WebViewEvent(const String &wvName, const std::vector<DataReference<const char*> >& jsargs)
 : InputEvent(InputDeviceWPtr(), IdPair(getEventId(),
       jsargs.empty()?std::string():std::string(jsargs[0].data(), jsargs[0].length()))),
   webview(wvName)
{
    if (jsargs.size() >= 1) {
        name = std::string(jsargs[0].data(), jsargs[0].length());
        args.reserve(jsargs.size()-1);
        for (size_t i = 1; i < jsargs.size(); i++) {
            args.push_back(std::string(jsargs[i].data(), jsargs[i].length()));
        }
    }
}






WebViewEvent::~WebViewEvent() {
}

EventDescriptor WebViewEvent::getDescriptor() const {
    return EventDescriptor::Web(webview, name);
}

}
}
