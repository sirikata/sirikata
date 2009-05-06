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
#include "SDLInputManager.hpp"
#include "SDLEvents.hpp"
#include <algorithm>
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <direct.h>
#include <stdlib.h>
#endif

template<> Sirikata::Graphics::WebViewManager* Ogre::Singleton<Sirikata::Graphics::WebViewManager>::ms_Singleton = 0;

namespace Sirikata {
namespace Graphics {

#define TIP_SHOW_DELAY 0.7
#define TIP_ENTRY_DELAY 2.0

std::string getCurrentWorkingDirectory()
{
	return "";
}

WebViewManager::WebViewManager(Ogre::Viewport* defaultViewport, SDLInputManager* inputMgr, const std::string &baseDirectory)
	: webCore(0), focusedWebView(0), mouseXPos(0), mouseYPos(0), mouseButtonRDown(false), zOrderCounter(5), 
	defaultViewport(defaultViewport), tooltipParent(0), lastTooltip(0), tooltipShowTime(0), isDraggingFocusedWebView(0)
{
	webCore = new Awesomium::WebCore(Awesomium::LOG_VERBOSE);
	webCore->setBaseDirectory(getCurrentWorkingDirectory() + baseDirectory + "\\");

	tooltipWebView = createWebView("__tooltip", 250, 50, OverlayPosition(0, 0), false, 70, TIER_FRONT);
	tooltipWebView->hide();
	tooltipWebView->setTransparent(true);
	tooltipWebView->loadFile("tooltip.html");
	//tooltipWebView->bind("resizeTooltip", JSDelegate(this, &WebViewManager::onResizeTooltip));
	tooltipWebView->bind("resizeTooltip", boost::bind(&WebViewManager::onResizeTooltip, this, _1, _2));
	//tooltipWebView->setIgnoresMouse();

	inputMgr->subscribe(Sirikata::Graphics::SDL::MouseMotionType()(), std::tr1::bind(&WebViewManager::onMouseMove, this, _1));
	inputMgr->subscribe(Sirikata::Graphics::SDL::MouseButtonPressed::getEventId(), std::tr1::bind(&WebViewManager::onMouseDown, this, _1));
	inputMgr->subscribe(Sirikata::Graphics::SDL::MouseButtonReleased::getEventId(), std::tr1::bind(&WebViewManager::onMouseUp, this, _1));
	inputMgr->subscribe(Sirikata::Graphics::SDL::KeyPressed::getEventId(), std::tr1::bind(&WebViewManager::onKeyDown, this, _1));
	inputMgr->subscribe(Sirikata::Graphics::SDL::KeyReleased::getEventId(), std::tr1::bind(&WebViewManager::onKeyUp, this, _1));
	inputMgr->subscribe(Sirikata::Graphics::SDL::TextInputType()(), std::tr1::bind(&WebViewManager::onKeyTextInput, this, _1));
}

WebViewManager::~WebViewManager()
{
	for(iter = activeWebViews.begin(); iter != activeWebViews.end();)
	{
		WebView* toDelete = iter->second;
		iter = activeWebViews.erase(iter);
		delete toDelete;
	}

	if(webCore)
		delete webCore;
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
	webCore->update();

	std::map<std::string,WebView*>::iterator end;
	end = activeWebViews.end();
	iter = activeWebViews.begin();

	while(iter != end)
	{
		if(iter->second->okayToDelete)
		{
			WebView* webViewToDelete = iter->second;
			iter = activeWebViews.erase(iter);
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
			iter++;
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
	iter = activeWebViews.find(webViewName);
	if(iter != activeWebViews.end())
		return iter->second;

	return 0;
}

void WebViewManager::destroyWebView(const std::string &webViewName)
{
	iter = activeWebViews.find(webViewName);
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

bool WebViewManager::injectMouseMove(int xPos, int yPos)
{
	bool eventHandled = false;

	if(focusedWebView && isDraggingFocusedWebView || focusedWebView && mouseButtonRDown)
	{
		if(focusedWebView->movable)
			focusedWebView->move(xPos-mouseXPos, yPos-mouseYPos);

		eventHandled = true;
	}
	else
	{
		WebView* top = getTopWebView(xPos, yPos);

		if(top)
		{
			top->injectMouseMove(top->getRelativeX(xPos), top->getRelativeY(yPos));
			eventHandled = true;

			for(iter = activeWebViews.begin(); iter != activeWebViews.end(); ++iter)
				if(iter->second->ignoringBounds)
					if(!(iter->second->isPointOverMe(xPos, yPos) && iter->second->overlay->panel->getZOrder() < top->overlay->panel->getZOrder()))
						iter->second->injectMouseMove(iter->second->getRelativeX(xPos), iter->second->getRelativeY(yPos));
		}

		if(tooltipParent)
		{
			if(!tooltipParent->isPointOverMe(xPos, yPos))
				handleTooltip(0, L"");
		}

		if(tooltipWebView->getVisibility())
			tooltipWebView->setPosition(OverlayPosition(xPos, yPos + 15));
	}

	mouseXPos = xPos;
	mouseYPos = yPos;

	return eventHandled;
}

bool WebViewManager::injectMouseWheel(int relScroll)
{
	if(focusedWebView)
	{
		focusedWebView->injectMouseWheel(relScroll);
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

	if(buttonID == LeftMouseButton && focusedWebView)
		focusedWebView->injectMouseUp(focusedWebView->getRelativeX(mouseXPos), focusedWebView->getRelativeY(mouseYPos));
	else if(buttonID == RightMouseButton)
	{
		mouseButtonRDown = false;
	}

	if(focusedWebView)
		return true;

	return false;
}

#if defined(_WIN32)
void WebViewManager::handleKeyboardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(focusedWebView)
		focusedWebView->webView->injectKeyboardEvent(0, msg, wParam, lParam);		
}
#endif

bool WebViewManager::focusWebView(int x, int y, WebView* selection)
{
	deFocusAllWebViews();
	WebView* webViewToFocus = selection? selection : getTopWebView(x, y);

	if(!webViewToFocus)
		return false;

	std::vector<WebView*> sortedWebViews;

	for(iter = activeWebViews.begin(); iter != activeWebViews.end(); iter++)
		if(iter->second->overlay)
			if(iter->second->overlay->getTier() == webViewToFocus->overlay->getTier())
				sortedWebViews.push_back(iter->second);

	struct compare { bool operator()(WebView* a, WebView* b){ return(a->overlay->getZOrder() > b->overlay->getZOrder()); }};
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
	focusedWebView->webView->focus();
	isDraggingFocusedWebView = false;

	return true;
}

WebView* WebViewManager::getTopWebView(int x, int y)
{
	WebView* top = 0;

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
	for(iter = activeWebViews.begin(); iter != activeWebViews.end(); iter++)
		iter->second->webView->unfocus();

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
	Sirikata::Graphics::SDL::MouseMotion* e = dynamic_cast<Sirikata::Graphics::SDL::MouseMotion*>(evt.get());

	this->injectMouseMove(e->mEvent.x, e->mEvent.y);

	return Sirikata::Task::EventResponse::nop();
}

Sirikata::Task::EventResponse WebViewManager::onMouseDown(Sirikata::Task::EventPtr evt)
{
	Sirikata::Graphics::SDL::MouseButtonPressed* e = dynamic_cast<Sirikata::Graphics::SDL::MouseButtonPressed*>(evt.get());

	if(e->mEvent.button == 1)
		this->injectMouseDown(LeftMouseButton);
	else if(e->mEvent.button == 3)
		this->injectMouseDown(RightMouseButton);

	return Sirikata::Task::EventResponse::nop();
}

Sirikata::Task::EventResponse WebViewManager::onMouseUp(Sirikata::Task::EventPtr evt)
{
	Sirikata::Graphics::SDL::MouseButtonReleased* e = dynamic_cast<Sirikata::Graphics::SDL::MouseButtonReleased*>(evt.get());

	if(e->mEvent.button == 1)
		this->injectMouseUp(LeftMouseButton);
	else if(e->mEvent.button == 3)
		this->injectMouseUp(RightMouseButton);

	return Sirikata::Task::EventResponse::nop();
}

#if defined(_WIN32)
unsigned int mapSDLToWindows(SDL_scancode scancode);
#endif

Sirikata::Task::EventResponse WebViewManager::onKeyDown(Sirikata::Task::EventPtr evt)
{
	Sirikata::Graphics::SDL::KeyPressed* e = dynamic_cast<Sirikata::Graphics::SDL::KeyPressed*>(evt.get());

#if defined(_WIN32)
	handleKeyboardMessage(0, WM_KEYDOWN, mapSDLToWindows(e->mEvent.keysym.scancode), 0);
#endif

	return Sirikata::Task::EventResponse::nop();
}

Sirikata::Task::EventResponse WebViewManager::onKeyUp(Sirikata::Task::EventPtr evt)
{
	Sirikata::Graphics::SDL::KeyReleased* e = dynamic_cast<Sirikata::Graphics::SDL::KeyReleased*>(evt.get());

#if defined(_WIN32)
	handleKeyboardMessage(0, WM_KEYUP, mapSDLToWindows(e->mEvent.keysym.scancode), 0);
#endif
	
	return Sirikata::Task::EventResponse::nop();
}

Sirikata::Task::EventResponse WebViewManager::onKeyTextInput(Sirikata::Task::EventPtr evt)
{
	Sirikata::Graphics::SDL::TextInput* e = dynamic_cast<Sirikata::Graphics::SDL::TextInput*>(evt.get());

#if defined(_WIN32)
	handleKeyboardMessage(0, WM_CHAR, (unsigned short)e->mEvent.text[0], 0);
#endif

	return Sirikata::Task::EventResponse::nop();
}

#if defined(_WIN32)
#define MAP_CHAR(X) case SDL_SCANCODE_##X: return #X[0]
#define MAP_VK(A, B) case SDL_SCANCODE_##A: return VK_##B

unsigned int mapSDLToWindows(SDL_scancode scancode)
{
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
	MAP_VK(KP_0, INSERT);			MAP_VK(KP_1, END);
	MAP_VK(KP_2, DOWN);				MAP_VK(KP_3, NEXT);
	MAP_VK(KP_4, LEFT);				MAP_VK(KP_6, RIGHT);
	MAP_VK(KP_7, HOME);				MAP_VK(KP_8, UP);
	MAP_VK(KP_9, PRIOR);
	default: return 0;
	}
}

#endif

}
}