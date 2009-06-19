/*  Sirikata liboh -- Ogre Graphics Plugin
 *  ViewportOverlay.cpp
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

#include "ViewportOverlay.hpp"

using namespace Ogre;

namespace Sirikata {
namespace Graphics {

ViewportOverlay::ViewportOverlay(const Ogre::String& name, Ogre::Viewport* viewport, int width, int height, 
	const OverlayPosition& pos, const Ogre::String& matName, Ogre::uchar zOrder, Tier tier)
: viewport(viewport), position(pos), isVisible(true), width(width), height(height), tier(tier), zOrder(zOrder)
{
	if(zOrder > 199)
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
			"Z-order is out of bounds, must be within [0, 199].", "ViewportOverlay::ViewportOverlay");

	OverlayManager& overlayManager = OverlayManager::getSingleton();

	panel = static_cast<PanelOverlayElement*>(overlayManager.createOverlayElement("Panel", name + "Panel"));
	panel->setMetricsMode(Ogre::GMM_PIXELS);
	panel->setMaterialName(matName);
	panel->setDimensions(width, height);
	
	overlay = overlayManager.create(name + "Overlay");
	overlay->add2D(panel);
	setZOrder(zOrder);
	resetPosition();

	if(viewport)
		viewport->getTarget()->addListener(this);
}

ViewportOverlay::~ViewportOverlay()
{
	if(viewport)
		viewport->getTarget()->removeListener(this);

	if(overlay)
	{
		overlay->remove2D(panel);
		OverlayManager::getSingletonPtr()->destroyOverlayElement(panel);
		OverlayManager::getSingletonPtr()->destroy(overlay);
	}
}

void ViewportOverlay::setViewport(Ogre::Viewport* newViewport)
{
	overlay->hide();

	if(viewport)
		viewport->getTarget()->removeListener(this);

	viewport = newViewport;

	if(viewport)
	{
		viewport->getTarget()->addListener(this);
		resetPosition();
	}
}

void ViewportOverlay::move(int deltaX, int deltaY)
{
	panel->setPosition(panel->getLeft()+deltaX, panel->getTop()+deltaY);
}

void ViewportOverlay::setPosition(const OverlayPosition& position)
{
	this->position = position;
	resetPosition();
}

void ViewportOverlay::resetPosition()
{
	if(!viewport)
	{
		panel->setPosition(0, 0);
		return;
	}

	int viewWidth = viewport->getActualWidth();
	int viewHeight = viewport->getActualHeight();

	if(position.usingRelative)
	{
		int left = 0 + position.data.rel.x;
		int center = (viewWidth/2)-(width/2) + position.data.rel.x;
		int right = viewWidth - width + position.data.rel.x;

		int top = 0 + position.data.rel.y;
		int middle = (viewHeight/2)-(height/2) + position.data.rel.y;
		int bottom = viewHeight-height + position.data.rel.y;

		switch(position.data.rel.position)
		{
		case RP_LEFT:
			panel->setPosition(left, middle);
			break;
		case RP_TOPLEFT:
			panel->setPosition(left, top);
			break;
		case RP_TOPCENTER:
			panel->setPosition(center, top);
			break;
		case RP_TOPRIGHT:
			panel->setPosition(right, top);
			break;
		case RP_RIGHT:
			panel->setPosition(right, middle);
			break;
		case RP_BOTTOMRIGHT:
			panel->setPosition(right, bottom);
			break;
		case RP_BOTTOMCENTER:
			panel->setPosition(center, bottom);
			break;
		case RP_BOTTOMLEFT:
			panel->setPosition(left, bottom);
			break;
		case RP_CENTER:
			panel->setPosition(center, middle);
			break;
		default:
			panel->setPosition(position.data.rel.x, position.data.rel.y);
			break;
		}
	}
	else
		panel->setPosition(position.data.abs.left, position.data.abs.top);
}

void ViewportOverlay::resize(int width, int height)
{
	this->width = width;
	this->height = height;
	panel->setDimensions(width, height);
}

void ViewportOverlay::hide()
{
	isVisible = false;
}

void ViewportOverlay::show()
{
	isVisible = true;
}

void ViewportOverlay::setTier(Tier tier)
{
	this->tier = tier;
	overlay->setZOrder(200 * tier + zOrder);
}

void ViewportOverlay::setZOrder(Ogre::uchar zOrder)
{
	this->zOrder = zOrder;
	overlay->setZOrder(200 * tier + zOrder);
}

Tier ViewportOverlay::getTier()
{
	return tier;
}

Ogre::uchar ViewportOverlay::getZOrder()
{
	return zOrder;
}

int ViewportOverlay::getX()
{
	return viewport? viewport->getActualLeft() + panel->getLeft() : 0;
}

int ViewportOverlay::getY()
{
	return viewport? viewport->getActualTop() + panel->getTop() : 0;
}

int ViewportOverlay::getRelativeX(int absX)
{
	return viewport? absX - viewport->getActualLeft() - panel->getLeft() : 0;
}

int ViewportOverlay::getRelativeY(int absY)
{
	return viewport? absY - viewport->getActualTop() - panel->getTop() : 0;
}

bool ViewportOverlay::isWithinBounds(int absX, int absY)
{
	if(!viewport)
		return false;

	if(absX < viewport->getActualLeft() || absX > viewport->getActualWidth() + viewport->getActualLeft() ||
		absY < viewport->getActualTop() || absY > viewport->getActualHeight() + viewport->getActualTop())
		return false;

	int localX = getRelativeX(absX);
	int localY = getRelativeY(absY);

	if(localX > 0 && localX < width)
		if(localY > 0 && localY < height)
			return true;

	return false;
}

bool ViewportOverlay::operator>(const ViewportOverlay& rhs) const
{
	return tier * 200 + zOrder > rhs.tier * 200 + rhs.zOrder;
}

bool ViewportOverlay::operator<(const ViewportOverlay& rhs) const
{
	return tier * 200 + zOrder < rhs.tier * 200 + rhs.zOrder;
}

void ViewportOverlay::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
}

void ViewportOverlay::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
}

void ViewportOverlay::preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	if(evt.source == viewport && isVisible)
		overlay->show();
}

void ViewportOverlay::postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	overlay->hide();
}

void ViewportOverlay::viewportAdded(const Ogre::RenderTargetViewportEvent& evt)
{
}

void ViewportOverlay::viewportRemoved(const Ogre::RenderTargetViewportEvent& evt)
{
}

}
}
