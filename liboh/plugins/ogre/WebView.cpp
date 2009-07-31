/*  Sirikata liboh -- Ogre Graphics Plugin
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

#include "WebView.hpp"
#include <OgreBitwise.h>

using namespace Ogre;

namespace Sirikata {
namespace Graphics {

WebView::WebView(const std::string& name, unsigned short width, unsigned short height, const OverlayPosition &viewPosition,
			bool asyncRender, int maxAsyncRenderRate, Ogre::uchar zOrder, Tier tier, Ogre::Viewport* viewport)
{
	webView = 0;
	viewName = name;
	viewWidth = width;
	viewHeight = height;
	movable = true;
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

	createMaterial();

	overlay = new ViewportOverlay(name + "_overlay", viewport, width, height, viewPosition, getMaterialName(), zOrder, tier);

	if(compensateNPOT)
		overlay->panel->setUV(0, 0, (Real)viewWidth/(Real)texWidth, (Real)viewHeight/(Real)texHeight);

	createWebView(asyncRender, maxAsyncRenderRate);
}

WebView::WebView(const std::string& name, unsigned short width, unsigned short height,
			bool asyncRender, int maxAsyncRenderRate, Ogre::FilterOptions texFiltering)
{
	webView = 0;
	viewName = name;
	viewWidth = width;
	viewHeight = height;
	overlay = 0;
	movable = false;
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
	createWebView(asyncRender, maxAsyncRenderRate);
}

WebView::~WebView()
{
	if(alphaCache)
		delete[] alphaCache;
#ifdef HAVE_AWESOMIUM
	if(webView)
		webView->destroy();
#endif
	if(overlay)
		delete overlay;

	MaterialManager::getSingletonPtr()->remove(viewName + "Material");
	TextureManager::getSingletonPtr()->remove(viewName + "Texture");
	if(usingMask) TextureManager::getSingletonPtr()->remove(viewName + "MaskTexture");

	if(proxyObject.use_count())
		proxyObject->WebViewProvider::removeListener(this);
}

void WebView::setProxyObject(const std::tr1::shared_ptr<ProxyWebViewObject>& proxyObject)
{
	if(this->proxyObject.use_count())
		proxyObject->WebViewProvider::removeListener(this);

	this->proxyObject = proxyObject;

	if(this->proxyObject.use_count())
		proxyObject->WebViewProvider::addListener(this);
}

void WebView::createWebView(bool asyncRender, int maxAsyncRenderRate)
{
#ifdef HAVE_AWESOMIUM
	webView = Awesomium::WebCore::Get().createWebView(viewWidth, viewHeight, false, asyncRender, maxAsyncRenderRate);
	webView->setListener(this);

	bind("drag", std::tr1::bind(&WebView::onRequestDrag, this, _1, _2));
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
#if defined(HAVE_AWESOMIUM) || !defined(__APPLE__)
	TexturePtr texture = TextureManager::getSingleton().createManual(
		viewName + "Texture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC_WRITE_ONLY_DISCARDABLE, this);

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	texDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	texPitch = (pixelBox.rowPitch*texDepth);

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	memset(pDest, 128, texHeight*texPitch);

	pixelBuffer->unlock();
#endif
	MaterialPtr material = MaterialManager::getSingleton().create(viewName + "Material",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	matPass = material->getTechnique(0)->getPass(0);
	matPass->setSceneBlending(SBT_TRANSPARENT_ALPHA);
	matPass->setDepthWriteEnabled(false);

	baseTexUnit = matPass->createTextureUnitState(viewName + "Texture");

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
	tex->setUsage(TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	tex->createInternalResources();

	// force update
}

void WebView::update()
{
#ifdef HAVE_AWESOMIUM
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

	if(!webView->isDirty())
		return;

	TexturePtr texture = TextureManager::getSingleton().getByName(viewName + "Texture");

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

	uint8* destBuffer = static_cast<uint8*>(pixelBox.data);

	webView->render(destBuffer, (int)texPitch, (int)texDepth);

	if(isWebViewTransparent && !usingMask && ignoringTrans)
	{
		for(int row = 0; row < texHeight; row++)
			for(int col = 0; col < texWidth; col++)
				alphaCache[row * alphaCachePitch + col] = destBuffer[row * texPitch + col * 4 + 3];
	}

	pixelBuffer->unlock();

	lastUpdateTime = timer.getMilliseconds();
#endif
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
			overlay->hide();
		}

		lastFadeTimeMS = timer.getMilliseconds();
	}
}

bool WebView::isPointOverMe(int x, int y)
{
	if(isMaterialOnly())
		return false;
	if(!overlay->isVisible || !overlay->viewport)
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
#ifdef HAVE_AWESOMIUM
	webView->loadURL(url);
#endif
}

void WebView::loadFile(const std::string& file)
{
#ifdef HAVE_AWESOMIUM
	webView->loadFile(file);
#endif
}

void WebView::loadHTML(const std::string& html)
{
#ifdef HAVE_AWESOMIUM
	webView->loadHTML(html);
#endif
}

void WebView::evaluateJS(const std::string& javascript)
{
#ifdef HAVE_AWESOMIUM
	webView->executeJavascript(javascript);
#endif
}

void WebView::evaluateJS(const std::string& javascript, const Awesomium::JSArguments& args)
{
#ifdef HAVE_AWESOMIUM
	std::string resultScript;
	char paramName[15];
	unsigned int i, count;

	for(i = 0, count = 0; i < javascript.length(); i++)
	{
		if(javascript[i] == '?')
		{
			count++;
			if(count <= args.size())
			{
				sprintf(paramName, "__p00%d", count - 1);
				setProperty(paramName, args[count-1]);
				resultScript += "Client.";
				resultScript += paramName;
			}
			else
			{
				resultScript += "undefined";
			}
		}
		else
		{
			resultScript.push_back(javascript[i]);
		}
	}

	webView->executeJavascript(resultScript);
#endif
}

Awesomium::FutureJSValue WebView::evaluateJSWithResult(const std::string& javascript)
{
#ifdef HAVE_AWESOMIUM
	return webView->executeJavascriptWithResult(javascript);
#else
	return 0;
#endif
}

void WebView::bind(const std::string& name, JSDelegate callback)
{
	delegateMap[name] = callback;

#ifdef HAVE_AWESOMIUM
	webView->setCallback(name);
#endif
}

void WebView::setProperty(const std::string& name, const Awesomium::JSValue& value)
{
#ifdef HAVE_AWESOMIUM
	webView->setProperty(name, value);
#endif
}

void WebView::setViewport(Ogre::Viewport* newViewport)
{
	if(overlay)
		overlay->setViewport(newViewport);
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

#ifdef HAVE_AWESOMIUM
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

void WebView::setMask(std::string maskFileName, std::string groupName)
{
	if(usingMask)
	{
		if(maskTexUnit)
		{
			matPass->removeTextureUnitState(1);
			maskTexUnit = 0;
		}

		if(!TextureManager::getSingleton().getByName(viewName + "MaskTexture").isNull())
			TextureManager::getSingleton().remove(viewName + "MaskTexture");
	}

	if(alphaCache)
	{
		delete[] alphaCache;
		alphaCache = 0;
	}

	if(maskFileName == "")
	{
		usingMask = false;
		maskImageParameters.first = "";
		maskImageParameters.second = "";

		if(isWebViewTransparent)
		{
			setTransparent(true);
			update();
		}

		return;
	}

	maskImageParameters.first = maskFileName;
	maskImageParameters.second = groupName;

	if(!maskTexUnit)
	{
		maskTexUnit = matPass->createTextureUnitState();
		maskTexUnit->setIsAlpha(true);
		maskTexUnit->setTextureFiltering(FO_NONE, FO_NONE, FO_NONE);
		maskTexUnit->setColourOperationEx(LBX_SOURCE1, LBS_CURRENT, LBS_CURRENT);
		maskTexUnit->setAlphaOperation(LBX_MODULATE);
	}

	Image srcImage;
	srcImage.load(maskFileName, groupName);

	Ogre::PixelBox srcPixels = srcImage.getPixelBox();
	unsigned char* conversionBuf = 0;

	if(srcImage.getFormat() != Ogre::PF_BYTE_A)
	{
		size_t dstBpp = Ogre::PixelUtil::getNumElemBytes(Ogre::PF_BYTE_A);
		conversionBuf = new unsigned char[srcImage.getWidth() * srcImage.getHeight() * dstBpp];
		Ogre::PixelBox convPixels(Ogre::Box(0, 0, srcImage.getWidth(), srcImage.getHeight()), Ogre::PF_BYTE_A, conversionBuf);
		Ogre::PixelUtil::bulkPixelConversion(srcImage.getPixelBox(), convPixels);
		srcPixels = convPixels;
	}

	TexturePtr maskTexture = TextureManager::getSingleton().createManual(
		viewName + "MaskTexture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_A, TU_STATIC_WRITE_ONLY);

	HardwarePixelBufferSharedPtr pixelBuffer = maskTexture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	size_t maskTexDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	alphaCachePitch = pixelBox.rowPitch;

	alphaCache = new unsigned char[alphaCachePitch*texHeight];

	uint8* buffer = static_cast<uint8*>(pixelBox.data);

	memset(buffer, 0, alphaCachePitch * texHeight);

	size_t minRowSpan = std::min(alphaCachePitch, srcPixels.rowPitch);
	size_t minHeight = std::min(texHeight, (unsigned short)srcPixels.getHeight());

	if(maskTexDepth == 1)
	{
		for(unsigned int row = 0; row < minHeight; row++)
			memcpy(buffer + row * alphaCachePitch, (unsigned char*)srcPixels.data + row * srcPixels.rowPitch, minRowSpan);

		memcpy(alphaCache, buffer, alphaCachePitch*texHeight);
	}
	else if(maskTexDepth == 4)
	{
		size_t destRowOffset, srcRowOffset, cacheRowOffset;

		for(unsigned int row = 0; row < minHeight; row++)
		{
			destRowOffset = row * alphaCachePitch * maskTexDepth;
			srcRowOffset = row * srcPixels.rowPitch;
			cacheRowOffset = row * alphaCachePitch;

			for(unsigned int col = 0; col < minRowSpan; col++)
				alphaCache[cacheRowOffset + col] = buffer[destRowOffset + col * maskTexDepth + 3] = ((unsigned char*)srcPixels.data)[srcRowOffset + col];
		}
	}
	else
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED,
			"Unexpected depth and format were encountered while creating a PF_BYTE_A HardwarePixelBuffer. Pixel format: " +
                    StringConverter::toString((uint32)pixelBox.format) + ", Depth:" + StringConverter::toString(maskTexDepth), "WebView::setMask");
	}

	pixelBuffer->unlock();

	if(conversionBuf)
		delete[] conversionBuf;

	maskTexUnit->setTextureName(viewName + "MaskTexture");
	usingMask = true;
}

void WebView::setMaxUPS(unsigned int maxUPS)
{
	maxUpdatePS = maxUPS;
}

void WebView::setMovable(bool isMovable)
{
	if(!isMaterialOnly())
		movable = isMovable;
}

void WebView::setOpacity(float opacity)
{
	if(opacity > 1) opacity = 1;
	else if(opacity < 0) opacity = 0;

	this->opacity = opacity;
}

void WebView::setPosition(const OverlayPosition &viewPosition)
{
	if(overlay)
		overlay->setPosition(viewPosition);
}

void WebView::resetPosition()
{
	if(overlay)
		overlay->resetPosition();
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
		overlay->hide();
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

	overlay->show();
}

void WebView::focus()
{
	if(overlay)
		WebViewManager::getSingleton().focusWebView(0, 0, this);
#ifdef HAVE_AWESOMIUM
	else
		webView->focus();
#endif
}

void WebView::move(int deltaX, int deltaY)
{
	if(overlay)
		overlay->move(deltaX, deltaY);
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
#ifdef HAVE_AWESOMIUM
	webView->injectMouseMove(xPos, yPos);
#endif
}

void WebView::injectMouseWheel(int relScrollX, int relScrollY)
{
#ifdef HAVE_AWESOMIUM
	webView->injectMouseWheelXY(relScrollX, relScrollY);
#endif
}

void WebView::injectMouseDown(int xPos, int yPos)
{
#ifdef HAVE_AWESOMIUM
	webView->injectMouseDown(Awesomium::LEFT_MOUSE_BTN);
#endif
}

void WebView::injectMouseUp(int xPos, int yPos)
{
#ifdef HAVE_AWESOMIUM
	webView->injectMouseUp(Awesomium::LEFT_MOUSE_BTN);
#endif
}

void WebView::injectKeyEvent(bool press, int modifiers, int vk_code, int scancode) {
#ifdef HAVE_AWESOMIUM
	webView->injectKeyEvent(press, modifiers, vk_code, scancode);
#endif
}

void WebView::injectTextEvent(std::string utf8) {
#ifdef HAVE_AWESOMIUM
	wchar_t *outchars = new wchar_t[utf8.size()+1];
	size_t len = mbstowcs(outchars, utf8.c_str(), utf8.size());
	std::wstring widestr(outchars, len);
	webView->injectTextEvent(widestr);
#endif
}

void WebView::captureImage(const std::string& filename)
{
#ifdef HAVE_AWESOMIUM
	Ogre::Image result;

	int bpp = isWebViewTransparent? 4 : 3;

	unsigned char* buffer = OGRE_ALLOC_T(unsigned char, viewWidth * viewHeight * bpp, Ogre::MEMCATEGORY_GENERAL);

	webView->render(buffer, viewWidth * bpp, bpp);

	result.loadDynamicImage(buffer, viewWidth, viewHeight, 1, isWebViewTransparent? Ogre::PF_BYTE_BGRA : Ogre::PF_BYTE_BGR, false);
	result.save(Awesomium::WebCore::Get().getBaseDirectory() + "\\" + filename);

	OGRE_FREE(buffer, Ogre::MEMCATEGORY_GENERAL);
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

	overlay->resize(viewWidth, viewHeight);
#ifdef HAVE_AWESOMIUM
	webView->resize(viewWidth, viewHeight);
#endif
	if(newTexWidth == texWidth && newTexHeight == texHeight)
		return;

	texWidth = newTexWidth;
	texHeight = newTexHeight;
    if (compensateNPOT) {
        Ogre::Real u1,v1,u2,v2;
        getDerivedUV(u1, v1,  u2,v2);
        overlay->panel->setUV(u1, v1, u2, v2);
    }

	matPass->removeAllTextureUnitStates();
	maskTexUnit = 0;
#if defined(HAVE_AWESOMIUM)|| !defined (__APPLE__)

	Ogre::TextureManager::getSingleton().remove(viewName + "Texture");

	TexturePtr texture = TextureManager::getSingleton().createManual(
		viewName + "Texture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC_WRITE_ONLY_DISCARDABLE, this);

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	texDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	texPitch = (pixelBox.rowPitch*texDepth);

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	memset(pDest, 128, texHeight*texPitch);

	pixelBuffer->unlock();
#endif

	baseTexUnit = matPass->createTextureUnitState(viewName + "Texture");

	baseTexUnit->setTextureFiltering(texFiltering, texFiltering, FO_NONE);
	if(texFiltering == FO_ANISOTROPIC)
		baseTexUnit->setTextureAnisotropy(4);

	if(usingMask)
	{
		setMask(maskImageParameters.first, maskImageParameters.second);
	}
	else if(alphaCache)
	{
		delete[] alphaCache;
		alphaCache = new unsigned char[texWidth * texHeight];
		alphaCachePitch = texWidth;
	}
}

void WebView::onBeginNavigation(const std::string& url)
{
}

void WebView::onBeginLoading(const std::string& url, int statusCode, const std::wstring& mimeType)
{
}

void WebView::onFinishLoading()
{
}

void WebView::onCallback(const std::string& name, const Awesomium::JSArguments& args)
{
	std::map<std::string, JSDelegate>::iterator i = delegateMap.find(name);

	if(i != delegateMap.end())
		i->second(this, args);
}

void WebView::onReceiveTitle(const std::wstring& title)
{
}

void WebView::onChangeTooltip(const std::wstring& tooltip)
{
	WebViewManager::getSingleton().handleTooltip(this, tooltip);
}

#if defined(_WIN32)
void WebView::onChangeCursor(const HCURSOR& cursor)
{
}
#endif

void WebView::onChangeKeyboardFocus(bool isFocused)
{
}

void WebView::onBeginNavigation(const std::string& url, const std::wstring& frameName) {
}

void WebView::onBeginLoading(const std::string& url, const std::wstring& frameName, int statusCode, const std::wstring& mimeType) {
}

void WebView::onReceiveTitle(const std::wstring& title, const std::wstring& frameName) {
}

void WebView::onChangeTargetURL(const std::string& url) {
}


void WebView::onRequestDrag(WebView *caller, const Awesomium::JSArguments &args)
{
	WebViewManager::getSingleton().handleRequestDrag(this);
}


}
}
