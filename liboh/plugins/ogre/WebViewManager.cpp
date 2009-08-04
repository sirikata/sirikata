/*  Sirikata liboh -- Ogre Graphics Plugin
 *  WebViewManager.cpp
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

#include "WebViewManager.hpp"
#include "WebView.hpp"
#include "input/SDLInputManager.hpp"
#include "input/InputEvents.hpp"
#include "SDL_scancode.h"
#include <algorithm>
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <direct.h>
#include <stdlib.h>
#endif

template<> Sirikata::Graphics::WebViewManager* Ogre::Singleton<Sirikata::Graphics::WebViewManager>::ms_Singleton = 0;

namespace Sirikata {
namespace Graphics {

using namespace Sirikata::Input;

#define TIP_SHOW_DELAY 0.7
#define TIP_ENTRY_DELAY 2.0

std::string getCurrentWorkingDirectory()
{
	return "";
}


static int InputButtonToAwesomiumButton(int32 input_button);

template<typename EventPtrType>
static WebViewCoord InputCoordToWebViewCoord(EventPtrType evt, float x, float y);

static unsigned int InputKeyToAwesomiumKey(SDL_scancode scancode, bool& numpad);
static int InputModifiersToAwesomiumModifiers(Modifier mod, bool numpad);


static const char* webview_chrome_html =
    "<html>\n"
    "<body>\n"
    "<script type=\"text/javascript\">\n"
    "</script>\n"
    "<table width=\"100%\"><tr>\n"
    "<td onclick=\"Client.navback()\">Back</td>\n"
    "<td onclick=\"Client.navforward()\">Forward</td>\n"
    "<td onclick=\"Client.navrefresh()\">Refresh</td>\n"
    "<td onclick=\"Client.navhome()\">Home</td>\n"
    "</tr></table>\n"
    "</body>\n"
    "</html>\n"
    ;


WebViewManager::WebViewManager(Ogre::Viewport* defaultViewport, const std::string &baseDirectory)
	: webCore(0), focusedWebView(0), tooltipParent(0),
          chromeWebView(NULL), focusedNonChromeWebView(NULL),
	  defaultViewport(defaultViewport), mouseXPos(0), mouseYPos(0),
	  isDragging(false), isResizing(false),
          zOrderCounter(5),
	  lastTooltip(0), tooltipShowTime(0), isDraggingFocusedWebView(0)
{
    tooltipWebView = 0;
#ifdef HAVE_AWESOMIUM
	webCore = new Awesomium::WebCore(Awesomium::LOG_VERBOSE);
	webCore->setBaseDirectory(getCurrentWorkingDirectory() + baseDirectory + "\\");

	tooltipWebView = createWebView("__tooltip", 250, 50, OverlayPosition(0, 0), false, 70, TIER_FRONT);
	tooltipWebView->hide();
	tooltipWebView->setTransparent(true);
	tooltipWebView->loadFile("tooltip.html");
	//tooltipWebView->bind("resizeTooltip", JSDelegate(this, &WebViewManager::onResizeTooltip));
	tooltipWebView->bind("resizeTooltip", std::tr1::bind(&WebViewManager::onResizeTooltip, this, _1, _2));
	//tooltipWebView->setIgnoresMouse();

        chromeWebView = createWebView("__chrome", 250, 45, OverlayPosition(RP_TOPCENTER), false, 70, TIER_FRONT);
        chromeWebView->loadHTML(std::string(webview_chrome_html));
        chromeWebView->bind("navback", std::tr1::bind(&WebViewManager::onChromeNav, this, _1, _2, NavigateBack));
        chromeWebView->bind("navforward", std::tr1::bind(&WebViewManager::onChromeNav, this, _1, _2, NavigateForward));
        chromeWebView->bind("navrefresh", std::tr1::bind(&WebViewManager::onChromeNav, this, _1, _2, NavigateRefresh));
        chromeWebView->bind("navhome", std::tr1::bind(&WebViewManager::onChromeNav, this, _1, _2, NavigateHome));
#endif
}

WebViewManager::~WebViewManager()
{
	WebViewMap::iterator iter;
	for(iter = activeWebViews.begin(); iter != activeWebViews.end();)
	{
		WebView* toDelete = iter->second;
		delete toDelete;
	}
#ifdef HAVE_AWESOMIUM
	if(webCore)
		delete webCore;
#endif
}

WebViewManager& WebViewManager::getSingleton()
{
	if(!ms_Singleton)
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED,
			"An attempt was made to retrieve the WebViewManager Singleton before it has been instantiated! Did you forget to do 'new WebViewManager(renderWin)'?",
			"WebViewManager::Get");

	return *ms_Singleton;
}

WebViewManager* WebViewManager::getSingletonPtr()
{
	return ms_Singleton;
}

void WebViewManager::Update()
{
#ifdef HAVE_AWESOMIUM
	webCore->update();
#endif
	WebViewMap::iterator end, iter;
	end = activeWebViews.end();
	iter = activeWebViews.begin();

	while(iter != end)
	{
		if(iter->second->okayToDelete)
		{
			WebView* webViewToDelete = iter->second;
			// erase does not invalidate other iterators.
			activeWebViews.erase(iter++);
			if(focusedWebView == webViewToDelete)
			{
				focusedWebView = NULL;
				focusedNonChromeWebView = NULL;
				isDraggingFocusedWebView = false;
			}

			delete webViewToDelete;
		}
		else
		{
			iter->second->update();
			++iter;
		}
	}

	if(tooltipShowTime)
	{
		if(tooltipShowTime < tooltipTimer.getMilliseconds())
		{
			tooltipWebView->show(true);
			tooltipShowTime = 0;
			lastTooltip = tooltipTimer.getMilliseconds();
		}
	}
}

WebView* WebViewManager::createWebView(const std::string &webViewName, unsigned short width, unsigned short height, const OverlayPosition &webViewPosition,
			bool asyncRender, int maxAsyncRenderRate, Tier tier, Ogre::Viewport* viewport)
{
	if(activeWebViews.find(webViewName) != activeWebViews.end())
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED,
			"An attempt was made to create a WebView named '" + webViewName + "' when a WebView by the same name already exists!",
			"WebViewManager::createWebView");

	int highestZOrder = -1;
	int zOrder = 0;

	WebViewMap::iterator iter;
	for(iter = activeWebViews.begin(); iter != activeWebViews.end(); iter++)
		if(iter->second->overlay)
			if(iter->second->overlay->getTier() == tier)
				if(iter->second->overlay->getZOrder() > highestZOrder)
					highestZOrder = iter->second->overlay->getZOrder();

	if(highestZOrder != -1)
		zOrder = highestZOrder + 1;

	return activeWebViews[webViewName] = new WebView(webViewName, width, height, webViewPosition, asyncRender, maxAsyncRenderRate, (Ogre::uchar)zOrder, tier,
		viewport? viewport : defaultViewport);
}

WebView* WebViewManager::createWebViewMaterial(const std::string &webViewName, unsigned short width, unsigned short height,
			bool asyncRender, int maxAsyncRenderRate, Ogre::FilterOptions texFiltering)
{
	if(activeWebViews.find(webViewName) != activeWebViews.end())
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED,
			"An attempt was made to create a WebView named '" + webViewName + "' when a WebView by the same name already exists!",
			"WebViewManager::createWebViewMaterial");

	return activeWebViews[webViewName] = new WebView(webViewName, width, height, asyncRender, maxAsyncRenderRate, texFiltering);
}

WebView* WebViewManager::getWebView(const std::string &webViewName)
{
	WebViewMap::iterator iter = activeWebViews.find(webViewName);
	if(iter != activeWebViews.end())
		return iter->second;

	return 0;
}

void WebViewManager::destroyWebView(const std::string &webViewName)
{
	WebViewMap::iterator iter = activeWebViews.find(webViewName);
	if(iter != activeWebViews.end())
		iter->second->okayToDelete = true;
}

void WebViewManager::destroyWebView(WebView* webViewToDestroy)
{
	if(webViewToDestroy)
		webViewToDestroy->okayToDelete = true;
}

void WebViewManager::resetAllPositions()
{
	WebViewMap::iterator iter;
	for(iter = activeWebViews.begin(); iter != activeWebViews.end(); iter++)
		if(!iter->second->isMaterialOnly())
			iter->second->resetPosition();
}

bool WebViewManager::isAnyWebViewFocused()
{
	if(focusedWebView)
		return true;

	return false;
}

WebView* WebViewManager::getFocusedWebView()
{
	return focusedWebView;
}

bool WebViewManager::injectMouseMove(const WebViewCoord& coord)
{
	bool eventHandled = false;

	if((focusedWebView && isDraggingFocusedWebView) || (focusedWebView && isDragging))
	{
		if(focusedWebView->movable)
			focusedWebView->move(coord.x-mouseXPos, coord.y-mouseYPos);

		eventHandled = true;
	}
        else if (focusedWebView && isResizing) {
            unsigned short w,h;
            focusedWebView->getExtents(w,h);
            int32 new_w = w + (coord.x - mouseXPos), new_h = h + (coord.y - mouseYPos);
            if (new_w < 100) new_w = 100;
            if (new_h < 100) new_h = 100;
            focusedWebView->resize(new_w, new_h);
            eventHandled = true;
        }
	else
	{
		WebView* top = getTopWebView(coord.x, coord.y);

		if(top)
		{
			top->injectMouseMove(top->getRelativeX(coord.x), top->getRelativeY(coord.y));
			eventHandled = true;

			WebViewMap::iterator iter;
			for(iter = activeWebViews.begin(); iter != activeWebViews.end(); ++iter)
				if(iter->second->ignoringBounds)
					if(!(iter->second->isPointOverMe(coord.x, coord.y) && iter->second->overlay->panel->getZOrder() < top->overlay->panel->getZOrder()))
						iter->second->injectMouseMove(iter->second->getRelativeX(coord.x), iter->second->getRelativeY(coord.y));
		}

		if(tooltipParent)
		{
			if(!tooltipParent->isPointOverMe(coord.x, coord.y))
				handleTooltip(0, L"");
		}

		if(tooltipWebView && tooltipWebView->getVisibility())
			tooltipWebView->setPosition(OverlayPosition(coord.x, coord.y + 15));
	}

	mouseXPos = coord.x;
	mouseYPos = coord.y;

	return eventHandled;
}

bool WebViewManager::injectMouseWheel(const WebViewCoord& relScroll)
{
	if(focusedWebView)
	{
		focusedWebView->injectMouseWheel(relScroll.x, relScroll.y);
		return true;
	}

	return false;
}

bool WebViewManager::injectMouseDown(int buttonID)
{
	if(buttonID == LeftMouseButton)
	{
		if(focusWebView(mouseXPos, mouseYPos))
		{
			int relX = focusedWebView->getRelativeX(mouseXPos);
			int relY = focusedWebView->getRelativeY(mouseYPos);

			focusedWebView->injectMouseDown(relX, relY);
		}
	}
	else if(buttonID == RightMouseButton)
	{
		isDragging = true;
		focusWebView(mouseXPos, mouseYPos);
	}
	else if(buttonID == MiddleMouseButton) {
            isResizing = true;
            focusWebView(mouseXPos, mouseYPos);
	}

	if(focusedWebView)
		return true;

	return false;
}

bool WebViewManager::injectMouseUp(int buttonID)
{
	isDraggingFocusedWebView = false;

	if(buttonID == LeftMouseButton && focusedWebView) {
		int relX = focusedWebView->getRelativeX(mouseXPos);
		int relY = focusedWebView->getRelativeY(mouseYPos);

		focusedWebView->injectMouseUp(relX, relY);
	}
	else if(buttonID == RightMouseButton)
	{
		isDragging = false;
	}
	else if(buttonID == MiddleMouseButton) {
            isResizing = false;
	}

	if(focusedWebView)
		return true;

	return false;
}

bool WebViewManager::injectKeyEvent(KeyEvent type, Modifier mods, KeyButton button) {
    if (focusedWebView) {
        bool numpad = false;
        bool pressed = (type == KEY_PRESSED);
        int vk_code = InputKeyToAwesomiumKey((SDL_scancode)button, numpad);
        int awemods = InputModifiersToAwesomiumModifiers(mods, numpad);

        focusedWebView->injectKeyEvent(pressed, awemods, vk_code, numpad);
        return true;
    }
    return false;
}

bool WebViewManager::injectTextEvent(std::string utf8text) {
	if (focusedWebView) {
		focusedWebView->injectTextEvent(utf8text);
		return true;
	}
	return false;
}

namespace {
struct compare { bool operator()(WebView* a, WebView* b){ return(a->getOverlay()->getZOrder() > b->getOverlay()->getZOrder()); }};
}

bool WebViewManager::focusWebView(int x, int y, WebView* selection)
{
	deFocusAllWebViews();
	WebView* webViewToFocus = selection? selection : getTopWebView(x, y);

	if(!webViewToFocus) {
            focusedNonChromeWebView = NULL;
            return false;
        }

	std::vector<WebView*> sortedWebViews;
	WebViewMap::iterator iter;

	for(iter = activeWebViews.begin(); iter != activeWebViews.end(); iter++)
		if(iter->second->overlay)
			if(iter->second->overlay->getTier() == webViewToFocus->overlay->getTier())
				sortedWebViews.push_back(iter->second);

	std::sort(sortedWebViews.begin(), sortedWebViews.end(), compare());

	if(sortedWebViews.size())
	{
		if(sortedWebViews.at(0) != webViewToFocus)
		{
			unsigned int popIdx = 0;
			for(; popIdx < sortedWebViews.size(); popIdx++)
				if(sortedWebViews.at(popIdx) == webViewToFocus)
					break;

			unsigned short highestZ = sortedWebViews.at(0)->overlay->getZOrder();
			for(unsigned int i = 0; i < popIdx; i++)
				sortedWebViews.at(i)->overlay->setZOrder(sortedWebViews.at(i+1)->overlay->getZOrder());

			sortedWebViews.at(popIdx)->overlay->setZOrder(highestZ);
		}
	}

	focusedWebView = webViewToFocus;
#ifdef HAVE_AWESOMIUM
	focusedWebView->webView->focus();
#endif

        if (focusedWebView != chromeWebView)
            focusedNonChromeWebView = focusedWebView;

	isDraggingFocusedWebView = false;

	return true;
}

WebView* WebViewManager::getTopWebView(int x, int y)
{
	WebView* top = 0;

	WebViewMap::iterator iter;
	for(iter = activeWebViews.begin(); iter != activeWebViews.end(); iter++)
	{
		if(!iter->second->isPointOverMe(x, y))
			continue;

		if(!top)
			top = iter->second;
		else
			top = top->overlay->panel->getZOrder() > iter->second->overlay->panel->getZOrder()? top : iter->second;
	}

	return top;
}

void WebViewManager::deFocusAllWebViews()
{
	WebViewMap::iterator iter;
#ifdef HAVE_AWESOMIUM
	for(iter = activeWebViews.begin(); iter != activeWebViews.end(); iter++)
		iter->second->webView->unfocus();
#endif
	/*
	astralMgr->defocusAll();

	hiddenWin->focus();
	hiddenWin->injectMouseMove(50, 50);
	hiddenWin->injectMouseDown(50, 50);
	hiddenWin->injectMouseUp(50, 50);

	focusedWebView = 0;
	*/

	focusedWebView = NULL;
	isDraggingFocusedWebView = false;
}

void WebViewManager::setDefaultViewport(Ogre::Viewport* newViewport)
{
	WebViewMap::iterator iter;
	for(iter = activeWebViews.begin(); iter != activeWebViews.end(); iter++)
	{
		if(iter->second->overlay)
			if(iter->second->overlay->viewport == defaultViewport)
				iter->second->overlay->setViewport(newViewport);
	}

	defaultViewport = newViewport;
}

void WebViewManager::onResizeTooltip(WebView* WebView, const Awesomium::JSArguments& args)
{
#ifdef HAVE_AWESOMIUM
	if(args.size() != 2)
		return;

	//tooltipWebView->resize(args[0].toInteger(), args[1].toInteger());
	//tooltipView->resize(256, 64);
	tooltipWebView->setPosition(OverlayPosition(mouseXPos, mouseYPos + 15));
	//popViewToFront(view);

	if(lastTooltip + TIP_ENTRY_DELAY > tooltipTimer.getMilliseconds())
	{
		tooltipWebView->show(true);
		lastTooltip = tooltipTimer.getMilliseconds();
	}
	else
	{
		tooltipShowTime = tooltipTimer.getMilliseconds() + TIP_SHOW_DELAY;
	}
#endif
}

void WebViewManager::handleTooltip(WebView* tooltipParent, const std::wstring& tipText)
{
	if(tipText.length())
	{
		this->tooltipParent = tooltipParent;
		tooltipShowTime = 0;
		tooltipWebView->hide(true);
		std::string tipStr(tipText.begin(), tipText.end());
		tooltipWebView->evaluateJS("setTooltip('" + tipStr + "')");
	}
	else
	{
		tooltipParent = 0;
		tooltipWebView->hide(true);
	}
}

void WebViewManager::handleRequestDrag(WebView* caller)
{
	focusWebView(0, 0, caller);
	isDraggingFocusedWebView = true;
}


void WebViewManager::onChromeNav(WebView* webview, const Awesomium::JSArguments& args, NavigationAction action) {
    if (focusedNonChromeWebView == NULL)
        return;

#ifdef HAVE_AWESOMIUM
    switch(action) {
      case NavigateBack:
        focusedNonChromeWebView->webView->goToHistoryOffset(-1);
        break;
      case NavigateForward:
        focusedNonChromeWebView->webView->goToHistoryOffset(1);
        break;
      case NavigateRefresh:
// Until we recompile Awesomium on Mac and Windows:
#if 0
        focusedNonChromeWebView->webView->refresh();
#else
        SILOG(ogre,error,"FIXME: refresh() is disabled...");
        focusedNonChromeWebView->webView->goToHistoryOffset(0);
#endif
        break;
      case NavigateHome:
        focusedNonChromeWebView->loadURL("http://www.google.com");
        break;
      default:
        SILOG(ogre,error,"Unknown navigation action from onChromeNav.");
        break;
    }
#endif //HAVE_AWESOMIUM
}

Sirikata::Task::EventResponse WebViewManager::onMouseMove(Sirikata::Task::EventPtr evt)
{
    MouseEventPtr e = std::tr1::dynamic_pointer_cast<MouseEvent>(evt);
	if (!e) {
		return Sirikata::Task::EventResponse::nop();
	}

        this->injectMouseMove(InputCoordToWebViewCoord(e, e->mX, e->mY));

	return Sirikata::Task::EventResponse::nop();
}

Sirikata::Task::EventResponse WebViewManager::onMouseClick(Sirikata::Task::EventPtr evt)
{
    MouseDownEventPtr e = std::tr1::dynamic_pointer_cast<MouseDownEvent>(evt);
	if (!e) {
		return Sirikata::Task::EventResponse::nop();
	}

	onMouseMove(evt);

	int awebutton = InputButtonToAwesomiumButton(e->mButton);
	bool success = (awebutton != UnknownMouseButton);

        // A special case is needed for scroll wheel events that show up as clicks
        if (awebutton == ScrollUpButton || awebutton == ScrollDownButton) {
            int32 factor = (awebutton == ScrollUpButton ? 1 : -1);
            int32 amount = 30; // XXX FIXME magic factor, should be a setting somewhere
            WebViewCoord relCoord(0, factor*amount);

            success = injectMouseWheel(relCoord);
            if (success)
		return Sirikata::Task::EventResponse::cancel();
            else
		return Sirikata::Task::EventResponse::nop();
        }

	if (success) {
		success = this->injectMouseDown(awebutton);
		success = success && this->injectMouseUp(awebutton);
	}

	if (success) {
		return Sirikata::Task::EventResponse::cancel();
	} else {
		return Sirikata::Task::EventResponse::nop();
	}
}

Sirikata::Task::EventResponse WebViewManager::onMouseDrag(Sirikata::Task::EventPtr evt)
{
    MouseDragEventPtr e = std::tr1::dynamic_pointer_cast<MouseDragEvent>(evt);
    if (!e) {
        return Sirikata::Task::EventResponse::nop();
    }

    int awebutton = InputButtonToAwesomiumButton(e->mButton);
    if (awebutton == UnknownMouseButton)
        return Sirikata::Task::EventResponse::nop();

    bool success = true;
    switch(e->mType) {
      case Sirikata::Input::DRAG_START:
        this->injectMouseMove(InputCoordToWebViewCoord(e, e->mXStart, e->mYStart));
        success = this->injectMouseDown(awebutton);
        break;
      case Sirikata::Input::DRAG_DRAG:
        success = this->injectMouseMove(InputCoordToWebViewCoord(e, e->mX, e->mY));
        break;
      case Sirikata::Input::DRAG_END:
        success = this->injectMouseUp(awebutton);
        break;
      default:
        SILOG(ogre,error,"Unknown drag event type.");
        break;
    }

    if (success)
        return Sirikata::Task::EventResponse::cancel();
    else
        return Sirikata::Task::EventResponse::nop();
}

Sirikata::Task::EventResponse WebViewManager::onButton(Sirikata::Task::EventPtr evt)
{
    ButtonEventPtr e = std::tr1::dynamic_pointer_cast<ButtonEvent>(evt);
	if (!e) {
		return Sirikata::Task::EventResponse::nop();
	}

	bool success = true;
	if(e->getDevice()->isKeyboard()) {
            success = this->injectKeyEvent(e->mEvent, e->mModifier, e->mButton);
	}
	if (success) {
		return Sirikata::Task::EventResponse::cancel();
	} else {
		return Sirikata::Task::EventResponse::nop();
	}
}

Sirikata::Task::EventResponse WebViewManager::onKeyTextInput(Sirikata::Task::EventPtr evt)
{
    TextInputEventPtr e = std::tr1::dynamic_pointer_cast<TextInputEvent>(evt);
	if (!e) {
		return Sirikata::Task::EventResponse::nop();
	}

	if (injectTextEvent(e->mText)) {
		return Sirikata::Task::EventResponse::cancel();
	} else {
		return Sirikata::Task::EventResponse::nop();
	}
}


static int InputButtonToAwesomiumButton(int32 input_button) {
    switch(input_button) {
      case 1:
        return LeftMouseButton;
      case 2:
        return MiddleMouseButton;
      case 3:
        return RightMouseButton;
// XXX FIXME these may only work for linux, depends on SDL input
      case 4:
        return ScrollUpButton;
      case 5:
        return ScrollDownButton;
      default:
        return UnknownMouseButton;
    }
}

template<typename EventPtrType>
static WebViewCoord InputCoordToWebViewCoord(EventPtrType evt, float x, float y) {
    unsigned int wid,hei;
    evt->getDevice()->getInputManager()->getWindowSize(wid,hei);
    return WebViewCoord(((x+1)*wid)/2, ((1-y)*hei)/2);
}

enum VirtKeys {
WIN_VK_CANCEL = 0x03,
WIN_VK_BACK = 0x08,
WIN_VK_TAB = 0x09,
WIN_VK_CLEAR = 0x0C,
WIN_VK_RETURN = 0x0D,
WIN_VK_SHIFT = 0x10,
WIN_VK_CONTROL = 0x11,
WIN_VK_MENU = 0x12,
WIN_VK_PAUSE = 0x13,
WIN_VK_CAPITAL = 0x14,
WIN_VK_ESCAPE = 0x1B,
WIN_VK_SPACE = 0x20,
WIN_VK_PRIOR = 0x21,
WIN_VK_NEXT = 0x22,
WIN_VK_END = 0x23,
WIN_VK_HOME = 0x24,
WIN_VK_LEFT = 0x25,
WIN_VK_UP = 0x26,
WIN_VK_RIGHT = 0x27,
WIN_VK_DOWN = 0x28,
WIN_VK_SELECT = 0x29,
WIN_VK_PRINT = 0x2A,
WIN_VK_EXECUTE = 0x2B,
WIN_VK_SNAPSHOT = 0x2C,
WIN_VK_INSERT = 0x2D,
WIN_VK_DELETE = 0x2E,
WIN_VK_HELP = 0x2F,
WIN_VK_LWINDOWS = 0x5B,
WIN_VK_RWINDOWS = 0x5C,
WIN_VK_SEPARATOR = 0x6C,
WIN_VK_SUBTRACT = 0x6D,
WIN_VK_DECIMAL = 0x6E,
WIN_VK_DIVIDE = 0x6F,
WIN_VK_F1 = 0x70,
WIN_VK_F2 = 0x71,
WIN_VK_F3 = 0x72,
WIN_VK_F4 = 0x73,
WIN_VK_F5 = 0x74,
WIN_VK_F6 = 0x75,
WIN_VK_F7 = 0x76,
WIN_VK_F8 = 0x77,
WIN_VK_F9 = 0x78,
WIN_VK_F10 = 0x79,
WIN_VK_F11 = 0x7A,
WIN_VK_F12 = 0x7B,
WIN_VK_F13 = 0x7C,
WIN_VK_F14 = 0x7D,
WIN_VK_F15 = 0x7E,
WIN_VK_F16 = 0x7F,
WIN_VK_F17 = 0x80,
WIN_VK_F18 = 0x81,
WIN_VK_F19 = 0x82,
WIN_VK_F20 = 0x83,
WIN_VK_F21 = 0x84,
WIN_VK_F22 = 0x85,
WIN_VK_F23 = 0x86,
WIN_VK_F24 = 0x87,
WIN_VK_NUMLOCK = 0x90,
WIN_VK_SCROLL = 0x91,
WIN_VK_LSHIFT = 0xA0,
WIN_VK_RSHIFT = 0xA1,
WIN_VK_LCONTROL = 0xA2,
WIN_VK_RCONTROL = 0xA3,
WIN_VK_LMENU = 0xA4,
WIN_VK_RMENU = 0xA5,
WIN_VK_OEM_1 = 0xBA,	// ';:' for US
WIN_VK_OEM_PLUS = 0xBB,	// '+' any country
WIN_VK_OEM_COMMA = 0xBC,	// ',' any country
WIN_VK_OEM_MINUS = 0xBD,	// '-' any country
WIN_VK_OEM_PERIOD = 0xBE,	// '.' any country
WIN_VK_OEM_2 = 0xBF,	// '/?' for US
WIN_VK_OEM_3 = 0xC0,	// '`~' for US
WIN_VK_OEM_4 = 0xDB,	//  '[{' for US
WIN_VK_OEM_5 = 0xDC,	//  '\|' for US
WIN_VK_OEM_6 = 0xDD,	//  ']}' for US
WIN_VK_OEM_7 = 0xDE,	//  ''"' for US
WIN_VK_OEM_8 = 0xDF,
WIN_VK_PLAY = 0xFA,
WIN_VK_ZOOM = 0xFB
};

#define MAP_CHAR(X) case SDL_SCANCODE_##X: return #X[0]
#define MAP_VK(A, B) case SDL_SCANCODE_##A: return WIN_VK_##B
#define MAP_NUMPAD_VK(A, B) case SDL_SCANCODE_##A: numpad=true; return WIN_VK_##B

static unsigned int InputKeyToAwesomiumKey(SDL_scancode scancode, bool& numpad)
{
	numpad = false;
	switch(scancode)
	{
	MAP_CHAR(A); MAP_CHAR(B); MAP_CHAR(C);
	MAP_CHAR(D); MAP_CHAR(E); MAP_CHAR(F);
	MAP_CHAR(G); MAP_CHAR(H); MAP_CHAR(I);
	MAP_CHAR(J); MAP_CHAR(K); MAP_CHAR(L);
	MAP_CHAR(M); MAP_CHAR(N); MAP_CHAR(O);
	MAP_CHAR(P); MAP_CHAR(Q); MAP_CHAR(R);
	MAP_CHAR(S); MAP_CHAR(T); MAP_CHAR(U);
	MAP_CHAR(V); MAP_CHAR(W); MAP_CHAR(X);
	MAP_CHAR(Y); MAP_CHAR(Z); MAP_CHAR(0);
	MAP_CHAR(1); MAP_CHAR(2); MAP_CHAR(3);
	MAP_CHAR(4); MAP_CHAR(5); MAP_CHAR(6);
	MAP_CHAR(7); MAP_CHAR(8); MAP_CHAR(9);
	MAP_VK(LSHIFT, LSHIFT);
	MAP_VK(RSHIFT, RSHIFT);
	MAP_VK(LCTRL, LCONTROL);
	MAP_VK(RCTRL, RCONTROL);
	MAP_VK(LALT, LMENU);
	MAP_VK(RALT, RMENU);
	MAP_VK(LGUI, LWINDOWS);
	MAP_VK(RGUI, RWINDOWS);
	MAP_VK(RETURN, RETURN);			MAP_VK(ESCAPE, ESCAPE);
	MAP_VK(BACKSPACE, BACK);		MAP_VK(TAB, TAB);
	MAP_VK(SPACE, SPACE);			MAP_VK(MINUS, OEM_MINUS);
	MAP_VK(EQUALS, OEM_PLUS);		MAP_VK(LEFTBRACKET, OEM_4);
	MAP_VK(RIGHTBRACKET, OEM_6);	MAP_VK(BACKSLASH, OEM_5);
	MAP_VK(SEMICOLON, OEM_1);		MAP_VK(APOSTROPHE, OEM_7);
	MAP_VK(GRAVE, OEM_3);			MAP_VK(COMMA, OEM_COMMA);
	MAP_VK(PERIOD, OEM_PERIOD);		MAP_VK(SLASH, OEM_2);
	MAP_VK(CAPSLOCK, CAPITAL);		MAP_VK(F1, F1);
	MAP_VK(F2, F2);					MAP_VK(F3, F3);
	MAP_VK(F4, F4);					MAP_VK(F5, F5);
	MAP_VK(F6, F6);					MAP_VK(F7, F7);
	MAP_VK(F8, F8);					MAP_VK(F9, F9);
	MAP_VK(F10, F10);				MAP_VK(F11, F11);
	MAP_VK(F12, F12);				MAP_VK(PRINTSCREEN, PRINT);
	MAP_VK(SCROLLLOCK, SCROLL);		MAP_VK(PAUSE, PAUSE);
	MAP_VK(INSERT, INSERT);			MAP_VK(HOME, HOME);
	MAP_VK(PAGEUP, PRIOR);			MAP_VK(DELETE, DELETE);
	MAP_VK(END, END);				MAP_VK(PAGEDOWN, NEXT);
	MAP_VK(RIGHT, RIGHT);			MAP_VK(LEFT, LEFT);
	MAP_VK(DOWN, DOWN);				MAP_VK(UP, UP);
	MAP_NUMPAD_VK(KP_0, INSERT);	MAP_NUMPAD_VK(KP_1, END);
	MAP_NUMPAD_VK(KP_2, DOWN);		MAP_NUMPAD_VK(KP_3, NEXT);
	MAP_NUMPAD_VK(KP_4, LEFT);		MAP_NUMPAD_VK(KP_6, RIGHT);
	MAP_NUMPAD_VK(KP_7, HOME);		MAP_NUMPAD_VK(KP_8, UP);
	MAP_NUMPAD_VK(KP_9, PRIOR);
	default: return 0;
	}
}

static int InputModifiersToAwesomiumModifiers(Modifier modifiers, bool numpad) {
    int awemods = 0;
#ifdef HAVE_AWESOMIUM
    if (modifiers &Input::MOD_SHIFT)
        awemods |= Awesomium::SHIFT_MOD;
    if (modifiers &Input::MOD_CTRL)
        awemods |= Awesomium::CONTROL_MOD;
    if (modifiers &Input::MOD_ALT)
        awemods |= Awesomium::ALT_MOD;
    if (modifiers &Input::MOD_GUI)
        awemods |= Awesomium::META_MOD;
    if (numpad)
        awemods |= Awesomium::KEYPAD_KEY;
#endif
    return awemods;
}

}
}
