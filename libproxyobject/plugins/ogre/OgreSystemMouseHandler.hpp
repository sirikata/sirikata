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
#include "task/UniqueId.hpp"
#include "WebView.hpp"
#include "WebViewManager.hpp"
#include "OgreMeshRaytrace.hpp"
#include "task/Event.hpp"
#include "task/EventManager.hpp"
#include <sirikata/core/transfer/DiskManager.hpp>
#include "input/InputEvents.hpp"

namespace Sirikata {

namespace Input {
class InputDevice;
}

namespace Graphics {

class OgreSystemMouseHandler {
public:
    OgreSystemMouseHandler(OgreSystem *parent);
    ~OgreSystemMouseHandler();

    void alert(const String& title, const String& text);
    void tick(const Task::LocalTime& t);

    void setDelegate(Invokable* del);

    // FIXME no reason for this to be in this class.
    SpaceObjectReference pick(Vector2f p, int direction);
private:
    void delegateEvent(Input::InputEventPtr inputev);

    // Gets the current set of modifiers from the input system. Used for mouse
    // events exposed via Invokable interface since the internal mouse events
    // don't come with modifiers.
    Input::Modifier getCurrentModifiers() const;

    void mouseOverWebView(Camera *cam, Time time, float xPixel, float yPixel, bool mousedown, bool mouseup);
    Entity* hoverEntity (Camera *cam, Time time, float xPixel, float yPixel, bool mousedown, int *hitCount,int which=0);

    bool recentMouseInRange(float x, float y, float *lastX, float *lastY);

    void createUIAction(const String& ui_page);

    void handleScriptReply(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload);

    void zoomAction(float value, Vector2f axes);
    Task::EventResponse keyHandler(Task::EventPtr ev);
    Task::EventResponse axisHandler(Task::EventPtr ev);
    Task::EventResponse textInputHandler(Task::EventPtr ev);
    Task::EventResponse mouseHoverHandler(Task::EventPtr ev);
    Task::EventResponse mousePressedHandler(Task::EventPtr ev);
    Task::EventResponse mouseReleasedHandler(Task::EventPtr ev);
    Task::EventResponse mouseClickHandler(Task::EventPtr ev);
    Task::EventResponse mouseDragHandler(Task::EventPtr evbase);
    Task::EventResponse webviewHandler(Task::EventPtr ev);

    void fpsUpdateTick(const Task::LocalTime& t);

    void renderStatsUpdateTick(const Task::LocalTime& t);

    void webViewNavigateAction(WebViewManager::NavigationAction action);
    void webViewNavigateStringAction(WebViewManager::NavigationAction action, const String& arg);

    Task::EventResponse deviceListener(Task::EventPtr evbase);

    void onUIDirectoryListingFinished(String initial_path,
        std::tr1::shared_ptr<Transfer::DiskManager::ScanRequest::DirectoryListing> dirListing);

    void onUIAction(WebView* webview, const JSArguments& args);

    int mWhichRayObject;

    OgreSystem *mParent;
    std::vector<Task::SubscriptionId> mEvents;
    typedef std::multimap<Input::InputDevice*, Task::SubscriptionId> DeviceSubMap;
    DeviceSubMap mDeviceSubscriptions;

    Invokable* mDelegate;

    struct UIInfo {
        UIInfo()
         : scripting(NULL),chat(NULL)
        {}

        WebView* scripting;
        WebView* chat;
    };
    typedef std::map<SpaceObjectReference, UIInfo> ObjectUIMap;
    ObjectUIMap mObjectUIs;
    // Currently we don't have a good way to push the space object reference to
    // the webview because doing it too early causes it to fail since the JS in
    // the page hasn't been executed yet.  Instead, we maintain a map so we can
    // extract it from the webview ID.
    typedef std::map<WebView*, ProxyObjectWPtr> ScriptingUIObjectMap;
    ScriptingUIObjectMap mScriptingUIObjects;
    typedef std::map<SpaceObjectReference, WebView*> ScriptingUIWebViewMap;
    ScriptingUIWebViewMap mScriptingUIWebViews;

    IntersectResult mMouseDownTri;
    ProxyObjectWPtr mMouseDownObject;
    int mMouseDownSubEntity; // not dereferenced.
    int mLastHitCount;
    float mLastHitX;
    float mLastHitY;

    std::set<int> mWebViewActiveButtons;

    Task::LocalTime mLastCameraTime;
    Task::LocalTime mLastFpsTime;
    Task::LocalTime mLastRenderStatsTime;

    WebView* mUIWidgetView;

    // To avoid too many messages, update only after a timeout
    float mNewQueryAngle;
    Network::IOTimerPtr mQueryAngleTimer;

    // Port for sending scripting requests and receiving scripting
    // responses. *Not* used for receiving scripting requests, so it is randomly
    // selected.
    ODP::Port* mScriptingRequestPort;
};


}
}
