/*  Sirikata Object Host -- Overlay Position
 *  LightInfo.hpp
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

#ifndef _SIRIKATA_OVERLAY_POSITION_HPP_
#define _SIRIKATA_OVERLAY_POSITION_HPP_

#include "Platform.hpp"

namespace Sirikata {

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
* Describes the position of a viewport-overlay in relative/absolute metrics.
* Used by WebViewListener and ProxyWebViewObject.
*/
class SIRIKATA_OH_EXPORT OverlayPosition
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
	OverlayPosition(const RelativePosition &relPosition, short offsetLeft = 0, short offsetTop = 0)
	{
		usingRelative = true;
		data.rel.position = relPosition;
		data.rel.x = offsetLeft;
		data.rel.y = offsetTop;
	}

	/**
	* Creates an absolutely-positioned OverlayPosition object.
	*
	* @param	absoluteLeft	The number of pixels from the left of the Viewport
	*
	* @param	absoluteTop		The number of pixels from the top of the Viewport.
	*/
	OverlayPosition(short absoluteLeft, short absoluteTop)
	{
		usingRelative = false;
		data.abs.left = absoluteLeft;
		data.abs.top = absoluteTop;
	}

	/**
	* Creates an empty OverlayPosition object (defaults to top-left corner).
	*/
	OverlayPosition()
	{
		usingRelative = false;
		data.abs.left = 0;
		data.abs.top = 0;
	}

	bool usingRelative;
	union {
		struct { RelativePosition position; short x; short y; } rel;
		struct { short left; short top; } abs;
	} data;
};

}

#endif