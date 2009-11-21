/*  Sirikata liboh -- Ogre Graphics Plugin
 *  WebView.hpp
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

#ifndef _SIRIKATA_GRAPHICS_WEBVIEW_HPP_
#define _SIRIKATA_GRAPHICS_WEBVIEW_HPP_

#include "OgreHeaders.hpp"
#include "Ogre.h"
#include "WebViewManager.hpp"
#include "oh/ProxyObjectListener.hpp"
#include <oh/WebViewListener.hpp>
#include <oh/ProxyWebViewObject.hpp>

#ifndef HAVE_BERKELIUM
namespace Berkelium {
  struct Rect {};
  struct Window;
  struct Widget;
  struct WindowDelegate {
    struct Data { char *message; int length; };
  };
}
#endif

namespace Sirikata {
namespace Graphics {

	class WebView;

	typedef std::tr1::function<void (WebView*, const JSArguments&)> JSDelegate;

	/**
	* A 'WebView' is essentially an offscreen browser window rendered to a dynamic texture (encapsulated
	* as an Ogre Material) that can optionally be contained within a viewport overlay.
	*/
	class WebView
        : public Ogre::ManualResourceLoader,
#ifdef HAVE_BERKELIUM
          public Berkelium::WindowDelegate,
#endif
          public Sirikata::WebViewListener,
          public Sirikata::ProxyObjectListener
	{
	public:

		void destroyed(); // From ProxyObjectListener

		void setProxyObject(const std::tr1::shared_ptr<ProxyWebViewObject>& proxyObject);

		/**
		* Loads a URL into the main frame.
		*/
		void loadURL(const std::string& url);

		/**
		* Loads a local file into the main frame.
		*
		* @note	The file should reside in the base directory.
		*/
		void loadFile(const std::string& file);

		/**
		* Loads a string of HTML directly into the main frame.
		*
		* @note	Relative URLs will be resolved using the base directory.
		*/
		void loadHTML(const std::string& html);

		/**
		* Evaluates Javascript in the context of the current page.
		*
		* @param	script	The Javascript to evaluate/execute.
		*/
		void evaluateJS(const std::string& javascript);

		/**
		* Sets a global 'Client' callback that can be invoked via Javascript from
		* within all pages loaded into this WebView.
		*
		* @param	name	The name of the callback.
		* @param	callback	The C++ callback to invoke when called via Javascript.
		*/
		void bind(const std::string& name, JSDelegate callback);


		void setViewport(Ogre::Viewport* newViewport);

		void setTransparent(bool isTransparent);

		/**
		* Normally, mouse movement is only injected into a specific WebView from WebViewManager if the mouse is within the boundaries of
		* a WebView and over an opaque area (not transparent). This behavior may be detrimental to certain WebViews, for
		* example an animated 'dock' with floating icons on a transparent background: the mouse-out event would never
		* be invoked on each icon because the WebView only received mouse movement input over opaque areas. Use this function
		* to makes this WebView always inject mouse movement, regardless of boundaries or transparency.
		*
		* @param	ignoreBounds	Whether or not this WebView should ignore bounds/transparency when injecting mouse movement.
		*
		* @note
		*	The occlusivity of each WebView will still be respected, mouse movement will not be injected
		*	if another WebView is occluding this WebView.
		*/
		void setIgnoreBounds(bool ignoreBounds = true);

		/**
		* Using alpha-masking/transparency doesn't just affect the visuals of a WebView; by default, WebViews 'ignore'
		* mouse movement/clicks over 'transparent' areas of a WebView (Areas with opacity less than 5%). You may
		* disable this behavior or redefine the 'transparent' threshold of opacity to something else other
		* than 5%.
		*
		* @param	ignoreTrans		Whether or not this WebView should ignore 'transparent' areas when mouse-picking.
		*
		* @param	defineThreshold		Areas with opacity less than this percent will be ignored
		*								(if ignoreTrans is true, of course). Default is 5% (0.05).
		*/
		void setIgnoreTransparent(bool ignoreTrans, float threshold = 0.05);

		/**
		* Adjusts the number of times per second this WebView may update.
		*
		* @param	maxUPS		The maximum number of times per second this WebView can update. Set this to '0' to
		*						use no update limiting (default).
		*/
		void setMaxUPS(unsigned int maxUPS = 0);

		/**
		* Toggles whether or not this WebView is movable. (not applicable to WebViews created as materials)
		*
		* @param	isMovable	Whether or not this WebView should be movable.
		*/
		void setMovable(bool isMovable = true);

		/**
		* Changes the overall opacity of this WebView to a certain percentage.
		*
		* @param	opacity		The opacity percentage as a float.
		*						Fully Opaque = 1.0, Fully Transparent = 0.0.
		*/
		void setOpacity(float opacity);

		/**
		* Sets the default position of this WebView to a new position and then moves
		* the WebView to that position. (not applicable to WebViews created as materials)
		*
		* @param	viewPosition	The new OverlayPosition to set the WebView to.
		*/
		void setPosition(const OverlayPosition &viewPosition);

		/**
		* Resets the position of this WebView to its default position. (not applicable to WebViews created as materials)
		*/
		void resetPosition();

		void hide();

		/**
		* Hides this WebView.
		*
		* @param	fade	Whether or not to fade this WebView down.
		*
		* @param	fadeDurationMS	If fading, the number of milliseconds to fade for.
		*/
		void hide(bool fade, unsigned short fadeDurationMS = 300);

		void show();

		/**
		* Shows this WebView.
		*
		* @param	fade	Whether or not to fade the WebView up.
		*
		* @param	fadeDurationMS	If fading, the number of milliseconds to fade for.
		*/
		void show(bool fade, unsigned short fadeDurationMS = 300);

		/**
		* Sends a focus message to this webView to highlight active form elements, etc.
		*/
		void focus();
		/**
		* Sends a 'blurs'/unfocus message to this webview, which causes form elements to become grayed out.
		*/
		void unfocus();

		/**
		* 'Raises' this WebView by popping it to the front of all other WebViews. (not applicable to WebViews created as materials)
		*/
		void raise();

		/**
		* Moves this WebView by relative amounts. (not applicable to WebViews created as materials or non-movable WebViews)
		*
		* @param	deltaX	The relative X amount to move this WebView by. Positive amounts move it right.
		*
		* @param	deltaY	The relative Y amount to move this WebView by. Positive amounts move it down.
		*/
		void move(int deltaX, int deltaY);

		/**
		* Retrieves the width and height that this WebView was created with.
		*
		* @param[out]	width	The unsigned short that will be used to store the retrieved width.
		*
		* @param[out]	height	The unsigned short that will be used to store the retrieved height.
		*/
		void getExtents(unsigned short &width, unsigned short &height);

		/**
		* Transforms an X-coordinate in screen-space to that of this WebView's relative space.
		*
		* @param	absX	The X-coordinate in screen-space to transform.
		*
		* @return	The X-coordinate in this WebView's relative space.
		*/
		int getRelativeX(int absX);

		/**
		* Transforms a Y-coordinate in screen-space to that of this WebView's relative space.
		*
		* @param	absX	The Y-coordinate in screen-space to transform.
		*
		* @return	The Y-coordinate in this WebView's relative space.
		*/
		int getRelativeY(int absY);

		/**
		* Returns whether or not this WebView was created as a material.
		*/
		bool isMaterialOnly();

		ViewportOverlay* getOverlay();

		/**
		* Returns the name of this WebView.
		*/
		std::string getName();

		/**
		* Returns the name of the Ogre::Material used internally by this WebView.
		*/
		std::string getViewTextureName();

		/**
		* Returns the name of the Ogre::Material used internally by this WebView.
		*/
		std::string getMaterialName();

		/**
		* Returns whether or not this WebView is currently visible. (See WebView::hide and WebView::show)
		*/
		bool getVisibility();

		/**
		* Returns whether or not this WebView is visible,
		* ignoring fading.  In other words, this will return
		* true if show() was called last, false if hide() was
		* called last, even if the WebView is still fading out.
		*/
		bool getNonStrictVisibility();

		/**
		* Gets the derived UV's of this WebView's internal texture. On certain systems we must compensate for lack of
		* NPOT-support on the videocard by using the next-highest POT texture. Normal WebViews compensate their UV's accordingly
		* however WebViews created as materials will need to adjust their own by use of this function.
		*
		* @param[out]	u1	The Ogre::Real that will be used to store the retrieved u1-coordinate.
		* @param[out]	v1	The Ogre::Real that will be used to store the retrieved v1-coordinate.
		* @param[out]	u2	The Ogre::Real that will be used to store the retrieved u2-coordinate.
		* @param[out]	v2	The Ogre::Real that will be used to store the retrieved v2-coordinate.
		*/
		void getDerivedUV(Ogre::Real& u1, Ogre::Real& v1, Ogre::Real& u2, Ogre::Real& v2);

		/**
		* Injects the mouse's current coordinates (in this WebView's own local coordinate space, see WebView::getRelativeX and
		* WebView::getRelativeY) into this WebView.
		*
		* @param	xPos	The X-coordinate of the mouse, relative to this WebView's origin.
		* @param	yPos	The Y-coordinate of the mouse, relative to this WebView's origin.
		*/
		void injectMouseMove(int xPos, int yPos);

		/**
		* Injects mouse wheel events into this WebView.
		*
		* @param	relScroll	The relative Scroll-Value of the mouse.
		*/
		void injectMouseWheel(int scrollX, int scrollY);

		/**
		* Injects mouse down events into this WebView. You must supply the current coordinates of the mouse in this
		* WebView's own local coordinate space. (see WebView::getRelativeX and WebView::getRelativeY)
		*
		* @param	xPos	The absolute X-Value of the mouse, relative to this WebView's origin.
		* @param	yPos	The absolute Y-Value of the mouse, relative to this WebView's origin.
		*/
		void injectMouseDown(int xPos, int yPos);

		/**
		* Injects mouse up events into this WebView. You must supply the current coordinates of the mouse in this
		* WebView's own local coordinate space. (see WebView::getRelativeX and WebView::getRelativeY)
		*
		* @param	xPos	The absolute X-Value of the mouse, relative to this WebView's origin.
		* @param	yPos	The absolute Y-Value of the mouse, relative to this WebView's origin.
		*/
		void injectMouseUp(int xPos, int yPos);

            void injectKeyEvent(bool press, int modifiers, int vk_code, int raw_scancode);

		void injectTextEvent(std::string utf8);

		void resize(int width, int height);

	protected:
#ifdef HAVE_BERKELIUM
		Berkelium::Window* webView;
        Berkelium::Rect blitNewImage(Ogre::HardwarePixelBufferSharedPtr pixelBuffer,
                                              const unsigned char*srcBuffer, const Berkelium::Rect&rect,
                                              int dx, int dy, const Berkelium::Rect&clipRect);
        void compositeWidgets(Berkelium::Window*);
#endif
		std::string viewName;
		unsigned short viewWidth;
		unsigned short viewHeight;
		ViewportOverlay* overlay;
		bool movable;
		unsigned int maxUpdatePS;
		Ogre::Timer timer;
		unsigned long lastUpdateTime;
		float opacity;
		bool usingMask;
		unsigned char* alphaCache;
		size_t alphaCachePitch;
		Ogre::Pass* matPass;
		Ogre::TextureUnitState* baseTexUnit;
		Ogre::TextureUnitState* maskTexUnit;
		bool ignoringTrans;
		float transparent;
		bool isWebViewTransparent;
		bool ignoringBounds;
		bool okayToDelete;

        Ogre::TexturePtr viewTexture;
        Ogre::TexturePtr backingTexture;
        std::map<Berkelium::Widget*,Ogre::TexturePtr>widgetTextures;
		double fadeValue;
		bool isFading;
		double deltaFadePerMS;
		double lastFadeTimeMS;

		bool compensateNPOT;
		unsigned short texWidth;
		unsigned short texHeight;
		size_t texDepth;
		size_t texPitch;
		std::map<std::string, JSDelegate> delegateMap;
		Ogre::FilterOptions texFiltering;
		std::pair<std::string, std::string> maskImageParameters;

		std::tr1::shared_ptr<ProxyWebViewObject> proxyObject;

		friend class WebViewManager;

		WebView(const std::string& name, unsigned short width, unsigned short height, const OverlayPosition &viewPosition,
			bool asyncRender, int maxAsyncRenderRate, Ogre::uchar zOrder, Tier tier, Ogre::Viewport* viewport);

		WebView(const std::string& name, unsigned short width, unsigned short height,
			bool asyncRender, int maxAsyncRenderRate, Ogre::FilterOptions texFiltering);

		~WebView();

		void createWebView(bool asyncRender, int maxAsyncRenderRate);

		void createMaterial();

		void loadResource(Ogre::Resource* resource);

		void update();

		void updateFade();

		bool isPointOverMe(int x, int y);

        void onAddressBarChanged(Berkelium::Window*, const char*url, size_t urlLength);
        void onStartLoading(Berkelium::Window*, const char* url, size_t urlLength);
        void onLoad(Berkelium::Window*);
        void onLoadError(Berkelium::Window*, const char* error, size_t errorLength);
        void onPaint(Berkelium::Window*, const unsigned char*, const Berkelium::Rect&, int x, int y, const Berkelium::Rect&);
        void onBeforeUnload(Berkelium::Window*, bool*);
        void onCancelUnload(Berkelium::Window*);
        void onCrashed(Berkelium::Window*);
        void onResponsive(Berkelium::Window*);
        void onUnresponsive(Berkelium::Window*);
        void onCreatedWindow(Berkelium::Window*, Berkelium::Window*);

        void onChromeSend(Berkelium::Window *win, Berkelium::WindowDelegate::Data msg, const Berkelium::WindowDelegate::Data*str, size_t numStr);

    /** Linux only. uses an OpenGL texture.
     * If not using OpenGL, each srcRect will get its own call to 'onPaint'
     * It should be possible to paint plugins directly onto the canvas.
     * If this is not possible, then plugins may be created as widgets with
     * a negative z-index (i.e. below anything else on the screen).

    virtual void onPaintPluginTexture(
        Berkelium::Window *win,
        void* sourceGLTexture,
        const std::vector<Berkelium::Rect> srcRects, // relative to destRect
        const Berkelium::Rect &destRect);
    */
    virtual void onWidgetCreated(Berkelium::Window *win, Berkelium::Widget *newWidget, int zIndex);
    virtual void onWidgetDestroyed(Berkelium::Window *win, Berkelium::Widget *newWidget);
    virtual void onWidgetResize(Berkelium::Window *win, Berkelium::Widget *widg, int w, int h);
    virtual void onWidgetMove(Berkelium::Window *win, Berkelium::Widget *widg, int x, int y);
    virtual void onWidgetPaint(
        Berkelium::Window *win,
        Berkelium::Widget *wid,
        const unsigned char *sourceBuffer,
        const Berkelium::Rect &rect,
        int dx, int dy,
        const Berkelium::Rect &scrollRect);

	};

}
}

#endif
