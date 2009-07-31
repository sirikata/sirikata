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

static int InputButtonToAwesomiumButton(int32 input_button) {
    switch(input_button) {
      case 1:
        return LeftMouseButton;
      case 2:
        return MiddleMouseButton;
      case 3:
        return RightMouseButton;
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

WebViewManager::WebViewManager(Ogre::Viewport* defaultViewport, const std::string &baseDirectory)
	: webCore(0), focusedWebView(0), tooltipParent(0),
	  defaultViewport(defaultViewport), mouseXPos(0), mouseYPos(0),
	  mouseButtonRDown(false), zOrderCounter(5),
	  lastTooltip(0), tooltipShowTime(0), isDraggingFocusedWebView(0)
{
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
				focusedWebView = 0;
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

	if((focusedWebView && isDraggingFocusedWebView) || (focusedWebView && mouseButtonRDown))
	{
		if(focusedWebView->movable)
			focusedWebView->move(coord.x-mouseXPos, coord.y-mouseYPos);

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

		if(tooltipWebView->getVisibility())
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
		mouseButtonRDown = true;
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
		mouseButtonRDown = false;
	}

	if(focusedWebView)
		return true;

	return false;
}

bool WebViewManager::injectKeyEvent(bool press, int modifiers, int scancode) {
	if (focusedWebView) {
		focusedWebView->injectKeyEvent(press, modifiers, scancode);
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

	if(!webViewToFocus)
		return false;

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

	focusedWebView = 0;
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
            success = this->injectKeyEvent(e->pressed(), e->mModifier, e->mButton);
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


}
}
