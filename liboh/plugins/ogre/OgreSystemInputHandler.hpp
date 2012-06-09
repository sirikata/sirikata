// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/proxyobject/Platform.hpp>
#include "OgreSystem.hpp"
#include <sirikata/ogre/WebView.hpp>
#include <sirikata/ogre/WebViewManager.hpp>
#include "OgreMeshRaytrace.hpp"
#include <sirikata/core/transfer/DiskManager.hpp>
#include <sirikata/ogre/input/InputListener.hpp>
#include <sirikata/ogre/input/InputEventCompletion.hpp>

namespace Sirikata {

namespace Input {
class InputDevice;
}

namespace Graphics {

class OgreSystemInputHandler : public Input::InputListener {
public:
    OgreSystemInputHandler(OgreSystem *parent);
    ~OgreSystemInputHandler();

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
    virtual Input::EventResponse onKeyPressedEvent(Input::ButtonPressedPtr ev);
    virtual Input::EventResponse onKeyRepeatedEvent(Input::ButtonRepeatedPtr ev);
    virtual Input::EventResponse onKeyReleasedEvent(Input::ButtonReleasedPtr ev);
    virtual Input::EventResponse onKeyDownEvent(Input::ButtonDownPtr ev);
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


    class WebViewInputListener : public Input::InputListener {
    public:
        WebViewInputListener(OgreSystemInputHandler* par)
         : mParent(par)
        {}

        virtual Input::EventResponse onInputDeviceEvent(Input::InputDeviceEventPtr ev) { return Input::EventResponse::nop(); }
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
        virtual Input::EventResponse onWebViewEvent(Input::WebViewEventPtr ev) { return Input::EventResponse::nop(); }

        OgreSystemInputHandler* mParent;
        std::set<int> mWebViewActiveButtons;
    };
    friend class WebViewInputListener;
    class DelegateInputListener : public Input::InputListener {
    public:
        DelegateInputListener(OgreSystemInputHandler* par)
         : mParent(par)
        {}

        virtual Input::EventResponse onInputDeviceEvent(Input::InputDeviceEventPtr ev) { return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onKeyPressedEvent(Input::ButtonPressedPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onKeyRepeatedEvent(Input::ButtonRepeatedPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onKeyReleasedEvent(Input::ButtonReleasedPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onKeyDownEvent(Input::ButtonDownPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onAxisEvent(Input::AxisEventPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onTextInputEvent(Input::TextInputEventPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onMouseHoverEvent(Input::MouseHoverEventPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onMousePressedEvent(Input::MousePressedEventPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onMouseReleasedEvent(Input::MouseReleasedEventPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onMouseClickEvent(Input::MouseClickEventPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onMouseDragEvent(Input::MouseDragEventPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }
        virtual Input::EventResponse onWebViewEvent(Input::WebViewEventPtr ev) { delegateEvent(ev); return Input::EventResponse::cancel(); }

        void delegateEvent(Input::InputEventPtr inputev);

        OgreSystemInputHandler* mParent;
        //key and value are same.
        std::map<Invokable*,Invokable*> mDelegates;
    };
    friend class DelegateInputListener;

    OgreSystem *mParent;

    WebViewInputListener mWebViewInputListener;
    DelegateInputListener mDelegateInputListener;
    Input::InputEventCompletion mEventCompleter;

    int mWhichRayObject;

    int mLastHitCount;
    float mLastHitX;
    float mLastHitY;

    Task::LocalTime mLastCameraTime;
    Task::LocalTime mLastFpsTime;
    Task::LocalTime mLastRenderStatsTime;

    bool mUIReady;
};


}
}
