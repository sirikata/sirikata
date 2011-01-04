/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  WebView.cpp
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

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/core/util/TemporalValue.hpp>
#include "WebView.hpp"
#include "WebViewManager.hpp"
#include <OgreBitwise.h>
#include <sirikata/core/util/UUID.hpp>

#ifdef HAVE_BERKELIUM
#include "berkelium/StringUtil.hpp"
#include "berkelium/ScriptVariant.hpp"
#endif

using namespace Ogre;

namespace Sirikata {
namespace Graphics {

#ifdef HAVE_BERKELIUM
using Berkelium::UTF8String;
#endif

WebView::WebView(
    const std::string& name, const std::string& type,
    unsigned short width, unsigned short height,
    const OverlayPosition &viewPosition, Ogre::uchar zOrder, Tier tier,
    Ogre::Viewport* viewport, const WebViewBorderSize& border)
{
#ifdef HAVE_BERKELIUM
	webView = 0;
#endif
	viewName = name;
        viewType = type;
	viewWidth = width;
	viewHeight = height;
	maxUpdatePS = 0;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	isWebViewTransparent = false;
	ignoringBounds = false;
	okayToDelete = false;
	compensateNPOT = false;
	texWidth = width;
	texHeight = height;
	alphaCache = 0;
	alphaCachePitch = 0;
	matPass = 0;
	baseTexUnit = 0;
	maskTexUnit = 0;
	fadeValue = 1;
	isFading = false;
	deltaFadePerMS = 0;
	lastFadeTimeMS = 0;
	texFiltering = Ogre::FO_NONE;

    mBorderLeft = border.mBorderLeft;
    mBorderRight = border.mBorderRight;
    mBorderTop = border.mBorderTop;
    mBorderBottom = border.mBorderBottom;

    createMaterial();

#ifdef HAVE_BERKELIUM
    overlay = new ViewportOverlay(name + "_overlay", viewport, width, height, viewPosition, getMaterialName(), zOrder, tier);
    if(compensateNPOT)
        overlay->panel->setUV(0, 0, (Real)viewWidth/(Real)texWidth, (Real)viewHeight/(Real)texHeight);
#else
    overlay = NULL;
#endif
}

WebView::WebView(const std::string& name, const std::string& type, unsigned short width, unsigned short height,
			Ogre::FilterOptions texFiltering)
{
#ifdef HAVE_BERKELIUM
	webView = 0;
#endif

    mBorderLeft=2;
    mBorderRight=2;
    mBorderTop=25;
    mBorderBottom=2;
	viewName = name;
        viewType = type;
	viewWidth = width;
	viewHeight = height;
	overlay = 0;
	maxUpdatePS = 0;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	ignoringBounds = false;
	okayToDelete = false;
	compensateNPOT = false;
	texWidth = width;
	texHeight = height;
	alphaCache = 0;
	alphaCachePitch = 0;
	matPass = 0;
	baseTexUnit = 0;
	maskTexUnit = 0;
	fadeValue = 1;
	isFading = false;
	deltaFadePerMS = 0;
	lastFadeTimeMS = 0;
	this->texFiltering = texFiltering;

	createMaterial();
}

WebView::~WebView()
{
	if(alphaCache)
		delete[] alphaCache;
#ifdef HAVE_BERKELIUM
    webView->destroy();
#endif
	if(overlay)
		delete overlay;

	MaterialManager::getSingletonPtr()->remove(getMaterialName());
	if (!this->viewTexture.isNull()) {
        ResourcePtr res(this->viewTexture);
        this->viewTexture.setNull();
        Ogre::TextureManager::getSingleton().remove(res);
    }
}

void WebView::createWebView(bool asyncRender, int maxAsyncRenderRate)
{
#ifdef HAVE_BERKELIUM
    initializeWebView(Berkelium::Window::create(
                          WebViewManager::getSingleton().bkContext));
#endif
}

void WebView::initializeWebView(
#ifdef HAVE_BERKELIUM
    Berkelium::Window *win
#endif
    )
{
#ifdef HAVE_BERKELIUM
    webView = win;
    webView->setDelegate(this);
    webView->addBindOnStartLoading(WideString::point_to(L"chrome"),
                  Berkelium::Script::Variant::emptyObject());
    webView->addBindOnStartLoading(WideString::point_to(L"chrome.send_"),
                  Berkelium::Script::Variant::bindFunction(
                      WideString::point_to(L"send"), false));
    webView->addEvalOnStartLoading(
        WideString::point_to(L"chrome.send = function(n, args) {\n"
                             L"console.log([n].concat(args));\n"
                             L"chrome.send_.apply(this, [n].concat(args));\n"
                             L"};"));
    //make sure that the width and height of the border do not dominate the size
    if (viewWidth>mBorderLeft+mBorderRight&&viewHeight>mBorderTop+mBorderBottom) {
        webView->resize(viewWidth-mBorderLeft-mBorderRight, viewHeight-mBorderTop-mBorderBottom);
    } else {
        webView->resize(0, 0);
    }
#endif
}

void WebView::createMaterial()
{
	if(opacity > 1) opacity = 1;
	else if(opacity < 0) opacity = 0;

	if(!Bitwise::isPO2(viewWidth) || !Bitwise::isPO2(viewHeight))
	{
		if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_NON_POWER_OF_2_TEXTURES))
		{
			if(Root::getSingleton().getRenderSystem()->getCapabilities()->getNonPOW2TexturesLimited())
				compensateNPOT = true;
		}
		else compensateNPOT = true;
#ifdef __APPLE__
//cus those fools always report #t when I ask if they support this or that
//and then fall back to their buggy and terrible software driver which has never once in my life rendered a single correct frame.
		compensateNPOT=true;
#endif
		if(compensateNPOT)
		{
			texWidth = Bitwise::firstPO2From(viewWidth);
			texHeight = Bitwise::firstPO2From(viewHeight);
		}
	}


	// Create the texture
#if defined(HAVE_BERKELIUM) || !defined(__APPLE__)
	TexturePtr texture = TextureManager::getSingleton().createManual(
		getViewTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC, this);
    this->viewTexture = texture;

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	unsigned int texDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	unsigned int texPitch = (pixelBox.rowPitch*texDepth);

     uint8* pDest = static_cast<uint8*>(pixelBox.data);

     memset(pDest, 128, texHeight*texPitch);

     pixelBuffer->unlock();
 #endif
     MaterialPtr material = MaterialManager::getSingleton().create(getMaterialName(),
         ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
     matPass = material->getTechnique(0)->getPass(0);
     //matPass->setSeparateSceneBlending (SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA, SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA);
     matPass->setSeparateSceneBlending (SBF_ONE, SBF_ONE_MINUS_SOURCE_ALPHA, SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA);
     matPass->setDepthWriteEnabled(false);

     baseTexUnit = matPass->createTextureUnitState(getViewTextureName());

     baseTexUnit->setTextureFiltering(texFiltering, texFiltering, FO_NONE);
     if(texFiltering == FO_ANISOTROPIC)
         baseTexUnit->setTextureAnisotropy(4);
 }

 // This is for when the rendering device has a hiccup and loses the dynamic texture
 void WebView::loadResource(Resource* resource)
 {
     Texture *tex = static_cast<Texture*>(resource);

     tex->setTextureType(TEX_TYPE_2D);
     tex->setWidth(texWidth);
     tex->setHeight(texHeight);
     tex->setNumMipmaps(0);
     tex->setFormat(PF_BYTE_BGRA);
     tex->setUsage(TU_DYNAMIC);
     tex->createInternalResources();

     // force update
 }

 void WebView::update()
 {
     if(maxUpdatePS)
         if(timer.getMilliseconds() - lastUpdateTime < 1000 / maxUpdatePS)
             return;

     updateFade();

     if(usingMask)
         baseTexUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, fadeValue * opacity);
     else if(isWebViewTransparent)
         baseTexUnit->setAlphaOperation(LBX_BLEND_TEXTURE_ALPHA, LBS_MANUAL, LBS_TEXTURE, fadeValue * opacity);
     else
         baseTexUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, fadeValue * opacity);

     lastUpdateTime = timer.getMilliseconds();
 }

 void WebView::updateFade()
 {
     if(isFading)
     {
         fadeValue += deltaFadePerMS * (timer.getMilliseconds() - lastFadeTimeMS);

         if(fadeValue > 1)
         {
             fadeValue = 1;
             isFading = false;
         }
         else if(fadeValue < 0)
         {
             fadeValue = 0;
             isFading = false;
             if(overlay) {overlay->hide();}
         }

         lastFadeTimeMS = timer.getMilliseconds();
     }
 }

 bool WebView::isPointOverMe(int x, int y)
 {
     if(isMaterialOnly())
         return false;
     if(!overlay || !overlay->isVisible || !overlay->viewport)
         return false;

     int localX = overlay->getRelativeX(x);
     int localY = overlay->getRelativeY(y);

     if(localX > 0 && localX < overlay->width)
         if(localY > 0 && localY < overlay->height)
             return !ignoringTrans || !alphaCache ? true :
                 alphaCache[localY * alphaCachePitch + localX] > 255 * transparent;

     return false;
 }

 void WebView::loadURL(const std::string& url)
 {
 #if defined(HAVE_BERKELIUM)
     webView->navigateTo(url.data(),url.length());
 #endif
 }

 void WebView::loadFile(const std::string& file)
 {
 #if defined(HAVE_BERKELIUM)
     std::string url = file;
     if (file.length() == 0) {
         url = "about:blank";
     } else if (file[0]=='/') {
         url = "file://"+file;
     } else {
         url = "file://"+WebViewManager::getSingleton().getBaseDir()+"/"+file;
     }
     webView->navigateTo(url.data(),url.length());
 #endif
 }
 static std::string htmlPrepend("data:text/html;charset=utf-8,");
 void WebView::loadHTML(const std::string& html)
 {
 #if defined(HAVE_BERKELIUM)
     char * data= new char[htmlPrepend.length()+html.length()+1];
     memcpy(data,htmlPrepend.data(),htmlPrepend.length());
     memcpy(data+htmlPrepend.length(),html.data(),html.length());
     data[htmlPrepend.length()+html.length()]=0;
     webView->navigateTo(data,htmlPrepend.length()+html.length());
     delete[] data;
 #endif
 }

 void WebView::evaluateJS(const std::string& utf8jsoo)
 {
     std::string utf8js = utf8jsoo + ";//";
 #if defined(HAVE_BERKELIUM)
     WideString wideJS = Berkelium::UTF8ToWide(
         UTF8String::point_to(utf8js.data(), utf8js.length()));
     UTF8String uJS = Berkelium::WideToUTF8(
         WideString::point_to(wideJS.data(), wideJS.length()));
     SILOG(webview,debug,"Eval JS: "<<uJS);
     webView->executeJavascript(wideJS);
     Berkelium::stringUtil_free(uJS);
     Berkelium::stringUtil_free(wideJS);
 #endif
 }

 void WebView::bind(const std::string& name, JSDelegate callback)
 {
     delegateMap[name] = callback;
 }

 void WebView::setViewport(Ogre::Viewport* newViewport)
 {
     if(overlay) {
         overlay->setViewport(newViewport);
     }
 }

 void WebView::setTransparent(bool isTransparent)
 {
     if(!isTransparent)
     {
         if(alphaCache && !usingMask)
         {
             delete[] alphaCache;
             alphaCache = 0;
         }
     }
     else
     {
         if(!alphaCache && !usingMask)
         {
             alphaCache = new unsigned char[texWidth * texHeight];
             alphaCachePitch = texWidth;
         }
     }

 #if defined(HAVE_BERKELIUM)
     webView->setTransparent(isTransparent);
 #endif
     isWebViewTransparent = isTransparent;
 }

 void WebView::setIgnoreBounds(bool ignoreBounds)
 {
     ignoringBounds = ignoreBounds;
 }

 void WebView::setIgnoreTransparent(bool ignoreTrans, float threshold)
 {
     ignoringTrans = ignoreTrans;

     if(threshold > 1) threshold = 1;
     else if(threshold < 0) threshold = 0;

     transparent = threshold;
 }

 void WebView::setMaxUPS(unsigned int maxUPS)
 {
     maxUpdatePS = maxUPS;
 }

 void WebView::setOpacity(float opacity)
 {
     if(opacity > 1) opacity = 1;
     else if(opacity < 0) opacity = 0;

     this->opacity = opacity;
 }

 void WebView::setPosition(const OverlayPosition &viewPosition)
 {
     if(overlay){
         overlay->setPosition(viewPosition);
     }
 }

 void WebView::resetPosition()
 {
     if(overlay){
         overlay->resetPosition();
     }
 }

 void WebView::hide()
 {
     hide(false, 0);
 }

 void WebView::hide(bool fade, unsigned short fadeDurationMS)
 {
     updateFade();

     if(fade)
     {
         isFading = true;
         deltaFadePerMS = -1 / (double)fadeDurationMS;
         lastFadeTimeMS = timer.getMilliseconds();
     }
     else
     {
         isFading = false;
         fadeValue = 0;
         if (overlay) {overlay->hide();}
     }
 }

 void WebView::show()
 {
     show(false, 0);
 }

 void WebView::show(bool fade, unsigned short fadeDurationMS)
 {
     updateFade();

     if(fade)
     {
         isFading = true;
         deltaFadePerMS = 1 / (double)fadeDurationMS;
         lastFadeTimeMS = timer.getMilliseconds();
     }
     else
     {
         isFading = false;
         fadeValue = 1;
     }

     if (overlay) {overlay->show();}
 }

 void WebView::focus()
 {
 #if defined(HAVE_BERKELIUM)
     webView->focus();
 #endif
 }

 void WebView::unfocus()
 {
 #if defined(HAVE_BERKELIUM)
     webView->unfocus();
 #endif
 }

 void WebView::raise()
 {
     if(overlay) {
         WebViewManager::getSingleton().focusWebView(this);
     } else {
         focus();
     }
 }

 void WebView::move(int deltaX, int deltaY)
 {
     if(overlay) {
         overlay->move(deltaX, deltaY);
     }
 }

 void WebView::getExtents(unsigned short &width, unsigned short &height)
 {
     width = viewWidth;
     height = viewHeight;
 }

 int WebView::getRelativeX(int absX)
 {
     if(isMaterialOnly())
         return 0;
     else
         return overlay->getRelativeX(absX);
 }

 int WebView::getRelativeY(int absY)
 {
     if(isMaterialOnly())
         return 0;
     else
         return overlay->getRelativeY(absY);
 }

 bool WebView::inDraggableRegion(int relX, int relY) {
     if(relY <= mBorderTop) {
         return true;
     }
     return false;
 }

 bool WebView::isMaterialOnly()
 {
     return !overlay;
 }

 ViewportOverlay* WebView::getOverlay()
 {
     return overlay;
 }

 std::string WebView::getName()
 {
     return viewName;
 }

 std::string WebView::getType()
 {
     return viewType;
 }

 std::string WebView::getViewTextureName()
 {
     return viewName;
 }

 std::string WebView::getMaterialName()
 {
     return viewName + "Material";
 }

 bool WebView::getVisibility()
 {
     if(isMaterialOnly())
         return fadeValue != 0;
     else
         return overlay->isVisible;
 }

 bool WebView::getNonStrictVisibility() {
	 if (!overlay) {return true;}
     if (isFading) {
         // When fading, we are actually the *opposite* of what the overlay claims.
         return !overlay->isVisible;
     }
     else {
         // If we're not fading, then we can trust the overlay.
         return overlay->isVisible;
     }
 }

 void WebView::getDerivedUV(Ogre::Real& u1, Ogre::Real& v1, Ogre::Real& u2, Ogre::Real& v2)
 {
	u1 = v1 = 0;
	u2 = v2 = 1;

	if(compensateNPOT)
	{
		u2 = (Ogre::Real)viewWidth/(Ogre::Real)texWidth;
		v2 = (Ogre::Real)viewHeight/(Ogre::Real)texHeight;
	}
}

void WebView::injectMouseMove(int xPos, int yPos)
{
    if (xPos>mBorderLeft&&yPos>mBorderTop&&xPos<viewWidth-mBorderRight) {
#if defined(HAVE_BERKELIUM)
        webView->mouseMoved(xPos-mBorderLeft, yPos-mBorderTop);
#endif
    }else {
        //handle tug on border
    }
}

void WebView::injectMouseWheel(int relScrollX, int relScrollY)
{
#if defined(HAVE_BERKELIUM)
    webView->mouseWheel(relScrollX, relScrollY);
#endif
}

void WebView::injectMouseDown(int xPos, int yPos)
{
    if (xPos>mBorderLeft&&yPos>mBorderTop&&xPos<viewWidth-mBorderRight) {
#if defined(HAVE_BERKELIUM)
        webView->mouseMoved(xPos-mBorderLeft, yPos-mBorderTop);
    webView->mouseButton(0, true);
#endif
    }else {
        //handle tug on border
    }
}

void WebView::injectMouseUp(int xPos, int yPos)
{
    if (xPos>mBorderLeft&&yPos>mBorderTop&&xPos<viewWidth-mBorderRight) {
#if defined(HAVE_BERKELIUM)
        webView->mouseMoved(xPos-mBorderLeft, yPos-mBorderTop);
        webView->mouseButton(0, false);
#endif
    }else {
        //handle tug on border
    }
}

void WebView::injectKeyEvent(bool press, int modifiers, int vk_code, int scancode) {
#if defined(HAVE_BERKELIUM)
	webView->keyEvent(press, modifiers, vk_code, scancode);
#endif
}

void WebView::injectTextEvent(std::string utf8) {
#if defined(HAVE_BERKELIUM)
    wchar_t *outchars = new wchar_t[utf8.size()+1];
    size_t len = mbstowcs(outchars, utf8.c_str(), utf8.size());
    webView->textEvent(outchars,len);
    delete []outchars;
#endif
}

void WebView::resize(int width, int height)
{
	if(width == viewWidth && height == viewHeight)
		return;

	viewWidth = width;
	viewHeight = height;

	int newTexWidth = viewWidth;
	int newTexHeight = viewHeight;

	if(!Bitwise::isPO2(viewWidth) || !Bitwise::isPO2(viewHeight))
	{
		if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_NON_POWER_OF_2_TEXTURES))
		{
			if(Root::getSingleton().getRenderSystem()->getCapabilities()->getNonPOW2TexturesLimited())
				compensateNPOT = true;
		}
		else compensateNPOT = true;
		compensateNPOT=true;
		if(compensateNPOT)
		{
			newTexWidth = Bitwise::firstPO2From(viewWidth);
			newTexHeight = Bitwise::firstPO2From(viewHeight);
		}
	}

	if (overlay) {overlay->resize(viewWidth, viewHeight);}
#if defined(HAVE_BERKELIUM)
    //make sure that the width and height of the border do not dominate the size
    if (viewWidth>mBorderLeft+mBorderRight&&viewHeight>mBorderTop+mBorderBottom) {
        webView->resize(viewWidth-mBorderLeft-mBorderRight, viewHeight-mBorderTop-mBorderBottom);
    } else {
        webView->resize(0, 0);
    }
#endif

    uint16 oldTexWidth = texWidth;
    uint16 oldTexHeight = texHeight;

    texWidth = newTexWidth;
    texHeight = newTexHeight;

    if (compensateNPOT) {
        // FIXME: How can we adjust UV coordinates on a mesh that we are bound to?
        Ogre::Real u1,v1,u2,v2;
        getDerivedUV(u1, v1,  u2,v2);
        if (overlay) {
            overlay->panel->setUV(u1, v1, u2, v2);
        }
    }

    if (texWidth == oldTexWidth && texHeight == oldTexHeight)
        return;

	matPass->removeAllTextureUnitStates();
	maskTexUnit = 0;
#if defined(HAVE_BERKELIUM)

	if (!this->viewTexture.isNull()) {
        ResourcePtr res(this->viewTexture);
        this->viewTexture.setNull();
        Ogre::TextureManager::getSingleton().remove(res);
    }

	this->viewTexture = TextureManager::getSingleton().createManual(
		getViewTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC, this);

    TexturePtr texture = this->viewTexture;

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	unsigned int texDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	unsigned int texPitch = (pixelBox.rowPitch*texDepth);

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	memset(pDest, 128, texHeight*texPitch);

	pixelBuffer->unlock();

	if (!this->backingTexture.isNull()) {
        ResourcePtr res(this->backingTexture);
        this->backingTexture.setNull();
        Ogre::TextureManager::getSingleton().remove(res);

        this->backingTexture = TextureManager::getSingleton().createManual(
            "B"+getViewTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
            TU_DYNAMIC, this);
    }

#endif

	baseTexUnit = matPass->createTextureUnitState(viewTexture->getName());

	baseTexUnit->setTextureFiltering(texFiltering, texFiltering, FO_NONE);
	if(texFiltering == FO_ANISOTROPIC)
		baseTexUnit->setTextureAnisotropy(4);

    if(alphaCache)
	{
		delete[] alphaCache;
		alphaCache = new unsigned char[texWidth * texHeight];
		alphaCachePitch = texWidth;
	}
}



#ifdef HAVE_BERKELIUM
///////// Berkelium Callbacks...
void WebView::onAddressBarChanged(Berkelium::Window*, URLString newURL) {
    SILOG(webview,debug,"onAddressBarChanged"<<newURL);
}
void WebView::onStartLoading(Berkelium::Window*, URLString newURL) {
    SILOG(webview,debug,"onStartLoading"<<newURL);
}
void WebView::onTitleChanged(Berkelium::Window*, WideString wtitle) {
    UTF8String textString = Berkelium::WideToUTF8(wtitle);
    std::string title (textString.data(), textString.length());
    Berkelium::stringUtil_free(textString);
    SILOG(webview,debug,"onTitleChanged"<<title);
}
void WebView::onTooltipChanged(Berkelium::Window*, WideString wtext) {
    UTF8String textString = Berkelium::WideToUTF8(wtext);
    std::string tooltip (textString.data(), textString.length());
    Berkelium::stringUtil_free(textString);
    SILOG(webview,debug,"onTooltipChanged"<<tooltip);
}

void WebView::onLoad(Berkelium::Window*) {
    SILOG(webview,debug,"onLoad");
}

void WebView::onConsoleMessage(Berkelium::Window *win, WideString wmessage,
                               WideString wsourceId, int line_no) {
    UTF8String textString = Berkelium::WideToUTF8(wmessage);
    UTF8String sourceString = Berkelium::WideToUTF8(wsourceId);
    std::string message (textString.data(), textString.length());
    std::string sourceId (sourceString.data(), sourceString.length());
    Berkelium::stringUtil_free(textString);
    Berkelium::stringUtil_free(sourceString);
    SILOG(webview,debug,"onConsoleMessage " << message << " at file " << sourceId << ":" << line_no);
}
void WebView::onScriptAlert(Berkelium::Window *win, WideString message,
                            WideString defaultValue, URLString url,
                            int flags, bool &success, WideString &value) {
    // FIXME: C++ string conversion functions are a pile of garbage.
    UTF8String textString = Berkelium::WideToUTF8(message);
    SILOG(webview,debug,"onScriptAlert "<<textString);
}

Berkelium::Rect WebView::getBorderlessRect(Ogre::HardwarePixelBufferSharedPtr pixelBuffer) const {
    Berkelium::Rect pixelBufferRect;
    pixelBufferRect.mHeight=pixelBuffer->getHeight()-mBorderTop-mBorderBottom;
    pixelBufferRect.mWidth=pixelBuffer->getWidth()-mBorderLeft-mBorderRight;
    pixelBufferRect.mTop=0;
    pixelBufferRect.mLeft=0;
    return pixelBufferRect;
}

Berkelium::Rect WebView::getBorderedRect(const Berkelium::Rect& orig) const {
    return orig.translate(mBorderLeft, mBorderTop);
}

void WebView::blitNewImage(
    HardwarePixelBufferSharedPtr pixelBuffer,
    const unsigned char* srcBuffer, const Berkelium::Rect& srcRect,
    const Berkelium::Rect& copyRect,
    bool updateAlphaCache
) {
    Berkelium::Rect destRect = getBorderlessRect(pixelBuffer);

    destRect = destRect.intersect(srcRect);
    destRect = destRect.intersect(copyRect);

    // Find the location of the rect in the source data. It needs to be offset
    // since newPixelBuffer doesn't necessarily cover the entire image.
    Berkelium::Rect destRectInSrc = destRect.translate(-srcRect.left(), -srcRect.top());

    Berkelium::Rect borderedDestRect = getBorderedRect(destRect);

    // For new data, because of the way HardwarePixelBuffers work (no copy
    // subregions from *Memory*), we have to copy each line over individually
    for(int y = 0; y < destRectInSrc.height(); y++) {
        pixelBuffer->blitFromMemory(
            Ogre::PixelBox(
                destRectInSrc.width(), 1, 1, Ogre::PF_A8R8G8B8,
                const_cast<unsigned char*>(srcBuffer + (srcRect.width()*(destRectInSrc.top()+y) + destRectInSrc.left()) * 4)
            ),
            Ogre::Box(borderedDestRect.left(), borderedDestRect.top() + y, borderedDestRect.right(), borderedDestRect.top() + y + 1)
        );

        if(updateAlphaCache && isWebViewTransparent && !usingMask && ignoringTrans && alphaCache && alphaCachePitch) {
            for(int x = 0; x < destRectInSrc.width(); x++)
                alphaCache[ alphaCachePitch*(borderedDestRect.top()+y) + (borderedDestRect.left()+x) ] =
                    srcBuffer[ (srcRect.width()*(destRectInSrc.top()+y) + destRectInSrc.left()+x)*4 + 3 ];
        }
    }
}

void WebView::blitScrollImage(
    HardwarePixelBufferSharedPtr pixelBuffer,
    const Berkelium::Rect& scrollOrigRect,
    int scroll_dx, int scroll_dy,
    bool updateAlphaCache
) {
    assert(scroll_dx != 0 || scroll_dy != 0);

    Berkelium::Rect scrolledRect = scrollOrigRect.translate(scroll_dx, scroll_dy);

    Berkelium::Rect scrolled_shared_rect = scrollOrigRect.intersect(scrolledRect);
    // Only do scrolling if they have non-zero intersection
    if (scrolled_shared_rect.width() == 0 || scrolled_shared_rect.height() == 0) return;
    Berkelium::Rect shared_rect = scrolled_shared_rect.translate(-scroll_dx, -scroll_dy);

    size_t width = shared_rect.width();
    size_t height = shared_rect.height();

    Ogre::TexturePtr shadow = Ogre::TextureManager::getSingleton().createManual(
        "_ _internal","_ _internal",
        Ogre::TEX_TYPE_2D,
        Bitwise::firstPO2From(width), Bitwise::firstPO2From(height), 1, 1,
        PF_BYTE_BGRA
    );
    {
        HardwarePixelBufferSharedPtr shadowBuffer = shadow->getBuffer();

        Berkelium::Rect borderedScrollRect = getBorderedRect(shared_rect);
        Berkelium::Rect borderedScrolledRect = getBorderedRect(scrolled_shared_rect);

        shadowBuffer->blit(
            pixelBuffer,
            Ogre::Box( borderedScrollRect.left(), borderedScrollRect.top(), borderedScrollRect.right(), borderedScrollRect.bottom()),
            Ogre::Box(0,0,width,height));

        pixelBuffer->blit(
            shadowBuffer,
            Ogre::Box(0,0,width,height),
            Ogre::Box(borderedScrolledRect.left(), borderedScrolledRect.top(), borderedScrolledRect.right(), borderedScrolledRect.bottom()));
    }
    Ogre::ResourcePtr shadowResource(shadow);
    Ogre::TextureManager::getSingleton().remove(shadowResource);

    // FIXME We should be updating the alpha cache here, but that would require
    // pulling data back from the card...
    //if(updateAlphaCache && isWebViewTransparent && !usingMask && ignoringTrans && alphaCache && alphaCachePitch) {
    //}
}

void WebView::onPaint(Berkelium::Window*win,
                      const unsigned char*srcBuffer, const Berkelium::Rect& srcRect,
                      size_t num_copy_rects, const Berkelium::Rect *copy_rects,
                      int dx, int dy, const Berkelium::Rect& scroll_rect) {
    TexturePtr texture = backingTexture.isNull()?viewTexture:backingTexture;
    HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();

    // Now, we first handle scrolling. We need to do this first since it
    // requires shifting existing data, some of which will be overwritten by
    // the regular dirty rect update.
    if (dx != 0 || dy != 0)
        blitScrollImage(pixelBuffer, scroll_rect, dx, dy, false);

    for (size_t i = 0; i < num_copy_rects; i++)
        blitNewImage(pixelBuffer, srcBuffer, srcRect, copy_rects[i], true);

    if (!backingTexture.isNull())
        compositeWidgets(win);
}
void WebView::onCrashed(Berkelium::Window*) {
    SILOG(webview,debug,"onCrashed");
}
void WebView::onResponsive(Berkelium::Window*) {
    SILOG(webview,debug,"onResponsive");
}
void WebView::onUnresponsive(Berkelium::Window*) {
    SILOG(webview,debug,"onUnresponsive");
}
void WebView::onCreatedWindow(Berkelium::Window*, Berkelium::Window*newwin) {
    std::string name;
    {
        static int i = 0;
        i++;
        std::ostringstream os;
        os << "_blank" << i;
        name = os.str();
    }
    SILOG(webview,debug,"onCreatedWindow "<<name);

    Berkelium::Rect r;
    r.mLeft = 0;
    r.mTop = 0;
    r.mWidth = 600;
    r.mHeight = 400;
    Berkelium::Widget *wid = newwin->getWidget();
    if (wid && wid->getRect().mWidth > 0 && wid->getRect().mHeight > 0) {
        r = wid->getRect();
    }
    WebViewManager::getSingleton().createWebViewPopup(
        name, r.width(), r.height(),
        OverlayPosition(r.left(), r.top()),
        newwin, TIER_MIDDLE,
        overlay?overlay->viewport:WebViewManager::getSingleton().defaultViewport);
}

void WebView::onWidgetCreated(Berkelium::Window *win, Berkelium::Widget *newWidget, int zIndex) {
    SILOG(webview,debug,"onWidgetCreated");
}
void WebView::onWidgetDestroyed(Berkelium::Window *win, Berkelium::Widget *wid) {
    std::map<Berkelium::Widget*,TexturePtr>::iterator where=widgetTextures.find(wid);
    if (where!=widgetTextures.end()) {
        if (!where->second.isNull()) {
            ResourcePtr mfd(where->second);
            widgetTextures.erase(where);
            TextureManager::getSingleton().remove(mfd);
        }else widgetTextures.erase(where);
    }
    if (widgetTextures.size()==0&&!backingTexture.isNull()) {

        ResourcePtr mfd(backingTexture);
        viewTexture->getBuffer()->blit(backingTexture->getBuffer(),
                                       Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()),
                                       Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()));
        backingTexture.setNull();
        TextureManager::getSingleton().remove(mfd);
    }
    SILOG(webview,debug,"onWidgetDestroyed");
}
void WebView::onWidgetResize(Berkelium::Window *win, Berkelium::Widget *widg, int w, int h) {
    TexturePtr tmp (TextureManager::getSingleton().createManual(
                        "Widget"+UUID::random().toString(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        TEX_TYPE_2D, Bitwise::firstPO2From(w), Bitwise::firstPO2From(h), 0, PF_BYTE_BGRA,
        TU_DYNAMIC, this));
    TexturePtr old=widgetTextures[widg];
    if (!old.isNull()) {
        int minwid=old->getWidth()<(unsigned int)w?old->getWidth():w;
        int minhei=old->getHeight()<(unsigned int)h?old->getHeight():h;
        tmp->getBuffer()->blit(old->getBuffer(),Ogre::Box(0,0,minwid,minhei),Ogre::Box(0,0,minwid,minhei));
/*        viewBuffer->blit(backingBuffer,
                         Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()),
                         Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()));
*/
    }
    widgetTextures[widg]=tmp;
    if (!old.isNull()){
        ResourcePtr mfd(old);
        old.setNull();
        TextureManager::getSingleton().remove(mfd);
    }
    SILOG(webview,debug,"onWidgetResize");
}
void WebView::onWidgetMove(Berkelium::Window *win, Berkelium::Widget *widg, int x, int y) {
    SILOG(webview,debug,"onWidgetMove");
    if (!backingTexture.isNull()) {
        compositeWidgets(win);
    }
}

void WebView::compositeWidgets(Berkelium::Window*win) {
    if (viewTexture.isNull()||backingTexture.isNull()) {
        SILOG(webview,fatal,"View or backing texture null during ocmpositing step");
        assert(false);
    }else {
        viewTexture->getBuffer()->blit(backingTexture->getBuffer(),
                                       Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()),
                                       Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()));
        int count=0;
        for (Berkelium::Window::BackToFrontIter i=win->backIter(),ie=win->backEnd();
             i!=ie;
             ++i,++count) {
            SILOG(webkit,warning,"Widget count: "<<count);
            std::map<Berkelium::Widget*,Ogre::TexturePtr>::iterator where=widgetTextures.find(*i);
            if (where!=widgetTextures.end()) {
                SILOG(webkit,warning,"Widget found: "<<*i);
                if (!where->second.isNull()){
                    Berkelium::Rect rect=(*i)->getRect();
                    Berkelium::Rect srcRect = rect;
                    SILOG(webkit,warning,"Blitting to "<<rect.left()<<","<<rect.top()<<" - "<<rect.right()<<","<<rect.bottom());
                    Berkelium::Rect windowRect;
					windowRect.mTop = 0;
					windowRect.mLeft = 0;
					windowRect.mHeight = viewTexture->getBuffer()->getHeight();
					windowRect.mWidth = viewTexture->getBuffer()->getWidth();
					rect = rect.intersect(windowRect);
					srcRect.mTop = rect.mTop - srcRect.mTop;
					srcRect.mLeft = rect.mLeft - srcRect.mLeft;
					srcRect.mHeight = rect.height();
					srcRect.mWidth = rect.width();
                    viewTexture->getBuffer()->blit(where->second->getBuffer(),
                                                   Ogre::Box(srcRect.left(),srcRect.top(),srcRect.right(),srcRect.bottom()),
                                                   Ogre::Box(rect.left(),rect.top(),rect.right(),rect.bottom()));
                }else {
                    widgetTextures.erase(where);
                }
            }
        }

    }
}

void WebView::onWidgetPaint(
        Berkelium::Window *win,
        Berkelium::Widget *wid,
        const unsigned char *srcBuffer,
        const Berkelium::Rect &srcRect,
        size_t num_copy_rects,
        const Berkelium::Rect *copy_rects,
        int dx, int dy,
        const Berkelium::Rect &scroll_rect) {
    return;
    if (backingTexture.isNull()&&!viewTexture.isNull()) {
        backingTexture=TextureManager::getSingleton().createManual(
            "B"+getViewTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            TEX_TYPE_2D, viewTexture->getWidth(), viewTexture->getHeight(), 0, PF_BYTE_BGRA,
		TU_DYNAMIC, this);
        HardwarePixelBufferSharedPtr viewBuffer = viewTexture->getBuffer();
        HardwarePixelBufferSharedPtr backingBuffer = backingTexture->getBuffer();
        backingBuffer->blit(viewBuffer,
                            Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()),
                            Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()));
    }
    TexturePtr widgetTex=widgetTextures[wid];
    if (widgetTex.isNull()) {
        onWidgetResize(win,wid,wid->getRect().width(),wid->getRect().height());
        widgetTex=widgetTextures[wid];
    }

    HardwarePixelBufferSharedPtr pixelBuffer = widgetTex->getBuffer();
    if (dx != 0 || dy != 0)
        blitScrollImage(pixelBuffer, scroll_rect, dx, dy, false);

    for (size_t i = 0; i < num_copy_rects; i++)
        blitNewImage(pixelBuffer, srcBuffer, srcRect, copy_rects[i], false);

    compositeWidgets(win);
    SILOG(webview,debug,"onWidgetPaint");
}

void WebView::onJavascriptCallback(Berkelium::Window *win, void* replyMsg, URLString origin, WideString funcName, Berkelium::Script::Variant *args, size_t numArgs) {
    if (numArgs < 1) {
        if (replyMsg)
            win->synchronousScriptReturn(replyMsg, Berkelium::Script::Variant());
        return;
    }
    const Berkelium::Script::Variant &name = args[0];
    args++;
    numArgs--;
    UTF8String nameUTF8 = Berkelium::WideToUTF8(name.toString());
    std::string nameStr(nameUTF8.get<std::string>());
    Berkelium::stringUtil_free(nameUTF8);
	std::map<std::string, JSDelegate>::iterator i = delegateMap.find(nameStr);

    if(i != delegateMap.end())
    {
        JSArguments argVector;
        std::vector<UTF8String> argStorage;
        for (size_t j=0;j!=numArgs;++j) {
            UTF8String temp = Berkelium::WideToUTF8(args[j].toString());
            argStorage.push_back(temp);
            argVector.push_back(JSArgument(temp.data(), temp.length()));
        }
		i->second(this, argVector);
        for (size_t j=0;j<argStorage.size();j++) {
            Berkelium::stringUtil_free(argStorage[j]);
        }
	}
    if (replyMsg)
        win->synchronousScriptReturn(replyMsg, Berkelium::Script::Variant());
}

boost::any WebView::invoke(std::vector<boost::any>& params)
{
  std::cout << "\n\n\n Inside WebView::invoke() \n\n\n" ;
  std::string name="";
  /*Check the first param */

  if(!params[0].empty() && params[0].type() == typeid(std::string) )
  {
    name = boost::any_cast<std::string>(params[0]);
  }

  // This will bind a callback for the graphics to the script
  if(name == "bind")
  {
    // we need to bind a function to some event.
    // second argument is the event name
    std::cout << "\n\nIn WebView::invoke\n\n";
    std::string event = "";
    if(!params[1].empty() && params[1].type() == typeid(std::string) )
    {
      event = boost::any_cast<std::string>(params[1]);
    }

    // the third argument has to be a function ptr
    //This function would take any

    //just _1, _2 for now
    std::cout << "\n\n Trying to get the param[2] \n\n";
    Invokable* invokable = boost::any_cast<Invokable*>(params[2]);
    bind("event", std::tr1::bind(&WebView::translateParamsAndInvoke, this, invokable, _1, _2));
    Invokable* inv_result = this;
    return boost::any(inv_result);

  }

  // This will write message from the script to the graphics window
  if(name == "write")
  {
    std::cout << "\n\n In WebView::invoke \n\n";

    // get the params[1] for the value of the message to be writte on the window

    std::string msg="";
    if(!params[1].empty() && params[1].type() == typeid(std::string) )
    {
      msg = boost::any_cast<std::string>(params[1]);
    }

    if(msg.empty()) return boost::any();

    std::cout << "msg is " << msg << "\n\n";
    // whenthe msg is not empty
    // FIXME: we need to escape strings
    String jsScript = String("addMessage(\"") +msg + String("\")");
    evaluateJS(jsScript);

    //JSArguments args;
    //args.push_back(JSArgument("ExecScript"));
    //args.push_back(JSArgument("Command"));
    //args.push_back(JSArgument(msg.data()));

    //WebViewManager::getSingletonPtr()->onRaiseWebViewEvent(this, *const_cast<JSArguments*>(&args));


    return boost::any();

  }


  return boost::any();
}

void WebView::translateParamsAndInvoke(Invokable* _invokable, WebView* wv, const JSArguments& args)
{
  std::vector<boost::any> params;
  // Do the translation here
  for(unsigned int i = 2 ; i < args.size(); i++)
  {
    const char* s = args[i].begin();

    params.push_back(String(s));
  }

  //After translation
  _invokable->invoke(params);

}

#endif //HAVE_BERKELIUM


const WebView::WebViewBorderSize WebView::mDefaultBorder(2,2,25,2);

}
}
