/*  Sirikata liboh -- Ogre Graphics Plugin
 *  WebViewManager.hpp
 *
 *  Copyright (c) 2009, Adam Jean Simmons
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

#ifndef _SIRIKATA_GRAPHICS_WEBVIEWMANAGER_HPP_
#define _SIRIKATA_GRAPHICS_WEBVIEWMANAGER_HPP_

#include "Ogre.h"
#include "ViewportOverlay.hpp"
#include <task/EventManager.hpp>

#ifdef HAVE_AWESOMIUM
#include "WebCore.h"
#else
namespace Awesomium {
  struct JSArguments;
  struct WebCore;
}
#endif

namespace Sirikata {
namespace Input { class SDLInputManager; }
namespace Graphics {

class WebView;

/**
* Enumerates internal mouse button IDs. Used by WebViewManager::injectMouseDown, WebViewManager::injectMouseUp
*/
enum MouseButtonID
{
	LeftMouseButton = 0,
	RightMouseButton, 
	MiddleMouseButton
};

/**
* Supreme dictator and Singleton: WebViewManager
*
* The class you will need to go to for all your WebView-related needs.
*/
class WebViewManager : public Ogre::Singleton<WebViewManager>
{
public:
	/**
	* Creates the WebViewManager singleton.
	*
	* @param	defaultViewport		The default Ogre::Viewport to place WebViews in. This can be overriden
	*								per-WebView via the last parameter of WebViewManager::createWebView.
	*
	* @param	baseDirectory		The relative path to your base directory. This directory is used
	*								by WebView::loadFile and WebView::loadHTML (to resolve relative URLs).
	*
	* @throws	Ogre::Exception::ERR_INTERNAL_ERROR		Throws this when LLMozLib fails initialization
	*/
	WebViewManager(Ogre::Viewport* defaultViewport, Input::SDLInputManager* inputMgr, const std::string &baseDirectory = "WebViewLocal");

	/**
	* Destroys any active WebViews, the WebViewMouse singleton (if instantiated).
	*/
	~WebViewManager();

	/**
	* Gets the WebViewManager Singleton.
	*
	* @return	A reference to the WebViewManager Singleton.
	*
	* @throws	Ogre::Exception::ERR_RT_ASSERTION_FAILED	Throws this if WebViewManager has not been instantiated yet.
	*/
	static WebViewManager& getSingleton();

	/**
	* Gets the WebViewManager Singleton as a pointer.
	*
	* @return	If the WebViewManager has been instantiated, returns a pointer to the WebViewManager Singleton,
	*			otherwise this returns 0.
	*/
	static WebViewManager* getSingletonPtr();

	/**
	* Gives each active WebView a chance to update, each may or may not update their internal textures
	* based on various conditions.
	*/
	void Update();

	/**
	* Creates a WebView.
	*/
	WebView* createWebView(const std::string &webViewName, unsigned short width, unsigned short height, const OverlayPosition &webViewPosition, 
		bool asyncRender = false, int maxAsyncRenderRate = 70, Tier tier = TIER_MIDDLE, Ogre::Viewport* viewport = 0);

	/**
	* Creates a WebViewMaterial. WebViewMaterials are just like WebViews except that they lack a movable overlay element. 
	* Instead, you handle the material and apply it to anything you like. Mouse input for WebViewMaterials should be 
	* injected via the WebView::injectMouse_____ API calls instead of the global WebViewManager::injectMouse_____ calls.
	*/
	WebView* createWebViewMaterial(const std::string &webViewName, unsigned short width, unsigned short height,
		bool asyncRender = false, int maxAsyncRenderRate = 70, Ogre::FilterOptions texFiltering = Ogre::FO_ANISOTROPIC);

	/**
	* Retrieve a pointer to a WebView by name.
	*
	* @param	webViewName	The name of the WebView to retrieve.
	*
	* @return	If the WebView is found, returns a pointer to the WebView, otherwise returns 0.
	*/
	WebView* getWebView(const std::string &webViewName);

	/**
	* Destroys a WebView.
	*
	* @param	webViewName	The name of the WebView to destroy.
	*/
	void destroyWebView(const std::string &webViewName);

	/**
	* Destroys a WebView.
	*
	* @param	webViewName	A pointer to the WebView to destroy.
	*/
	void destroyWebView(WebView* webViewToDestroy);

	/**
	* Resets the positions of all WebViews to their default positions. (not applicable to WebViewMaterials)
	*/
	void resetAllPositions();

	/**
	* Checks whether or not a WebView is focused/selected. (not applicable to WebViewMaterials)
	*
	* @return	True if a WebView is focused, False otherwise.
	*/
	bool isAnyWebViewFocused();

	/**
	* Gets the currently focused/selected WebView. (not applicable to WebViewMaterials)
	*
	* @return	A pointer to the WebView that is currently focused, returns 0 if none are focused.
	*/
	WebView* getFocusedWebView();

	/**
	* Injects the mouse's current position into WebViewManager. Used to generally keep track of where the mouse 
	* is for things like moving WebViews around, telling the internal pages of each WebView where the mouse is and
	* where the user has clicked, etc. (not applicable to WebViewMaterials)
	*
	* @param	xPos	The current X-coordinate of the mouse.
	* @param	yPos	The current Y-coordinate of the mouse.
	*
	* @return	Returns True if the injected coordinate is over a WebView, False otherwise.
	*/
	bool injectMouseMove(int xPos, int yPos);

	/**
	* Injects mouse wheel events into WebViewManager. Used to scroll the focused WebView. (not applicable to WebViewMaterials)
	*
	* @param	relScroll	The relative Scroll-Value of the mouse.
	*
	* @note
	*	To inject this using OIS: on a OIS::MouseListener::MouseMoved event, simply 
	*	inject "arg.state.Z.rel" of the "MouseEvent".
	*
	* @return	Returns True if the mouse wheel was scrolled while a WebView was focused, False otherwise.
	*/
	bool injectMouseWheel(int relScrollX, int relScrollY);

	/**
	* Injects mouse down events into WebViewManager. Used to know when the user has pressed a mouse button
	* and which button they used. (not applicable to WebViewMaterials)
	*
	* @param	buttonID	The ID of the button that was pressed. Left = 0, Right = 1, Middle = 2.
	*
	* @return	Returns True if the mouse went down over a WebView, False otherwise.
	*/
	bool injectMouseDown(int buttonID);

	/**
	* Injects mouse up events into WebViewManager. Used to know when the user has released a mouse button 
	* and which button they used. (not applicable to WebViewMaterials)
	*
	* @param	buttonID	The ID of the button that was released. Left = 0, Right = 1, Middle = 2.
	*
	* @return	Returns True if the mouse went up while a WebView was focused, False otherwise.
	*/
	bool injectMouseUp(int buttonID);

	bool injectKeyEvent(bool press, int modifiers, int scancode);

	bool injectTextEvent(std::string utf8);

	/**
	* De-Focuses any currently-focused WebViews.
	*/
	void deFocusAllWebViews();

	void setDefaultViewport(Ogre::Viewport* newViewport);

protected:
	friend class WebView; // Our very close friend <3

	Awesomium::WebCore* webCore;
	typedef std::map<std::string,WebView*> WebViewMap;
    WebViewMap activeWebViews;
	WebView* focusedWebView, *tooltipWebView, *tooltipParent;
	Ogre::Viewport* defaultViewport;
	int mouseXPos, mouseYPos;
	bool mouseButtonRDown;
	unsigned short zOrderCounter;
	Ogre::Timer tooltipTimer;
	double lastTooltip, tooltipShowTime;
	bool isDraggingFocusedWebView;

	bool focusWebView(int x, int y, WebView* selection = 0);
	WebView* getTopWebView(int x, int y);
	void onResizeTooltip(WebView* WebView, const Awesomium::JSArguments& args);
	void handleTooltip(WebView* tooltipParent, const std::wstring& tipText);
	void handleRequestDrag(WebView* caller);
	Sirikata::Task::EventResponse onMouseMove(Sirikata::Task::EventPtr evt);
	Sirikata::Task::EventResponse onMouseDrag(Sirikata::Task::EventPtr evt);
	Sirikata::Task::EventResponse onMouseClick(Sirikata::Task::EventPtr evt);
	Sirikata::Task::EventResponse onButton(Sirikata::Task::EventPtr evt);
	Sirikata::Task::EventResponse onKeyTextInput(Sirikata::Task::EventPtr evt);
};

}
}

#endif
