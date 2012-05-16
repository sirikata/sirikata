/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  OgreSystemMouseHandler.hpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

#include <sirikata/proxyobject/Platform.hpp>
#include "OgreSystem.hpp"
#include <sirikata/ogre/WebView.hpp>
#include <sirikata/ogre/WebViewManager.hpp>
#include "OgreMeshRaytrace.hpp"
#include <sirikata/core/transfer/DiskManager.hpp>
#include <sirikata/ogre/input/InputListener.hpp>

namespace Sirikata {

namespace Input {
class InputDevice;
}

namespace Graphics {

class OgreSystemMouseHandler : public Input::InputListener {
public:
    OgreSystemMouseHandler(OgreSystem *parent);
    ~OgreSystemMouseHandler();

    void alert(const String& title, const String& text);
    void tick(const Task::LocalTime& t);

    // Invoked when the main UI components are ready
    void uiReady();

    void addDelegate(Invokable* del);
    void removeDelegate(Invokable* del);

    // FIXME no reason for this to be in this class.
    SpaceObjectReference pick(Vector2f p, int direction, const SpaceObjectReference& ignore, Vector3f* hitPointOut=NULL);

    //FIXME should this be public?
    WebView* mUIWidgetView;
    void ensureUI();
    void windowResized(uint32 w, uint32 h);
private:
    void delegateEvent(Sirikata::Input::InputEventPtr inputev);

    // Gets the current set of modifiers from the input system. Used for mouse
    // events exposed via Invokable interface since the internal mouse events
    // don't come with modifiers.
    Sirikata::Input::Modifier getCurrentModifiers() const;

    ProxyEntity* hoverEntity (Camera *cam, Time time, float xPixel, float yPixel, bool mousedown, int *hitCount,int which=0, Vector3f* hitPointOut=NULL, SpaceObjectReference ignore = SpaceObjectReference::null());

    bool recentMouseInRange(float x, float y, float *lastX, float *lastY);

    void createUIAction(const String& ui_page);

    // InputListener Interface
    virtual Input::EventResponse onInputDeviceEvent(Input::InputDeviceEventPtr ev);
    virtual Input::EventResponse onKeyPressedEvent(Input::ButtonPressedPtr ev) { return onKeyEvent(ev); }
    virtual Input::EventResponse onKeyRepeatedEvent(Input::ButtonRepeatedPtr ev) { return onKeyEvent(ev); }
    virtual Input::EventResponse onKeyReleasedEvent(Input::ButtonReleasedPtr ev) { return onKeyEvent(ev); }
    virtual Input::EventResponse onKeyDownEvent(Input::ButtonDownPtr ev) { return onKeyEvent(ev); }
    Input::EventResponse onKeyEvent(Input::ButtonEventPtr ev);
    virtual Input::EventResponse onAxisEvent(Input::AxisEventPtr ev);
    virtual Input::EventResponse onTextInputEvent(Input::TextInputEventPtr ev);
    virtual Input::EventResponse onMouseHoverEvent(Input::MouseHoverEventPtr ev);
    virtual Input::EventResponse onMousePressedEvent(Input::MousePressedEventPtr ev);
    virtual Input::EventResponse onMouseReleasedEvent(Input::MouseReleasedEventPtr ev);
    virtual Input::EventResponse onMouseClickEvent(Input::MouseClickEventPtr ev);
    virtual Input::EventResponse onMouseDragEvent(Input::MouseDragEventPtr ev);
    virtual Input::EventResponse onWebViewEvent(Input::WebViewEventPtr ev);

    void fpsUpdateTick(const Task::LocalTime& t);

    void renderStatsUpdateTick(const Task::LocalTime& t);

    boost::any onUIAction(WebView* webview, const JSArguments& args);

    OgreSystem *mParent;

    //key and value are same.
    std::map<Invokable*,Invokable*> mDelegates;


    int mWhichRayObject;

    int mLastHitCount;
    float mLastHitX;
    float mLastHitY;

    std::set<int> mWebViewActiveButtons;

    Task::LocalTime mLastCameraTime;
    Task::LocalTime mLastFpsTime;
    Task::LocalTime mLastRenderStatsTime;

    bool mUIReady;
};


}
}
