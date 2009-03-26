/*  Sirikata liboh -- Ogre Graphics Plugin
 *  ViewportOverlay.hpp
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

#ifndef _SIRIKATA_GRAPHICS_VIEWPORT_OVERLAY_HPP_
#define _SIRIKATA_GRAPHICS_VIEWPORT_OVERLAY_HPP_

#include "Ogre.h"
#include "OgrePanelOverlayElement.h"

namespace Sirikata {
namespace Graphics {

/**
* An enumeration of relative positions for OverlayPosition.
*/
enum RelativePosition
{
	RP_LEFT,
	RP_TOPLEFT,
	RP_TOPCENTER,
	RP_TOPRIGHT,
	RP_RIGHT,
	RP_BOTTOMRIGHT,
	RP_BOTTOMCENTER,
	RP_BOTTOMLEFT,
	RP_CENTER
};

/**
* An object that holds position-data for a ViewportOverlay.
*/
class OverlayPosition
{
public:
	/**
	* Creates a relatively-positioned OverlayPosition object.
	*
	* @param	relPosition		The position of the ViewportOverlay in relation to the Viewport
	*
	* @param	offsetLeft	How many pixels from the left to offset the ViewportOverlay from the relative position.
	*
	* @param	offsetTop	How many pixels from the top to offset the ViewportOverlay from the relative position.
	*/
	OverlayPosition(const RelativePosition &relPosition, short offsetLeft = 0, short offsetTop = 0);

	/**
	* Creates an absolutely-positioned OverlayPosition object.
	*
	* @param	absoluteLeft	The number of pixels from the left of the Viewport
	*
	* @param	absoluteTop		The number of pixels from the top of the Viewport.
	*/
	OverlayPosition(short absoluteLeft, short absoluteTop);

protected:
	bool usingRelative;
	union {
		struct { RelativePosition position; short x; short y; } rel;
		struct { short left; short top; } abs;
	} data;

	friend class ViewportOverlay;
	OverlayPosition();
};

/**
* An enumeration of the three tiers ViewportOverlays can reside in.
*/
enum Tier
{
	TIER_BACK = 0,
	TIER_MIDDLE,
	TIER_FRONT
};

class ViewportOverlay : public Ogre::RenderTargetListener
{
public:
	Ogre::Viewport* viewport;
	Ogre::Overlay* overlay;
	Ogre::PanelOverlayElement* panel;
	OverlayPosition position;
	bool isVisible;
	int width, height;
	Tier tier;
	Ogre::uchar zOrder;

	ViewportOverlay(const Ogre::String& name, Ogre::Viewport* viewport, int width, int height, const OverlayPosition& pos, 
		const Ogre::String& matName, Ogre::uchar zOrder, Tier tier);
	~ViewportOverlay();

	void move(int deltaX, int deltaY);
	void setPosition(const OverlayPosition& position);
	void resetPosition();
	
	void resize(int width, int height);

	void hide();
	void show();

	void setTier(Tier tier);
	void setZOrder(Ogre::uchar zOrder);

	Tier getTier();
	Ogre::uchar getZOrder();

	int getX();
	int getY();
	
	int getRelativeX(int absX);
	int getRelativeY(int absY);

	bool isWithinBounds(int absX, int absY);

	bool operator>(const ViewportOverlay& rhs) const;
	bool operator<(const ViewportOverlay& rhs) const;

	void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
	void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
	void preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);
	void postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);
	void viewportAdded(const Ogre::RenderTargetViewportEvent& evt);
	void viewportRemoved(const Ogre::RenderTargetViewportEvent& evt);
};

}
}

#endif