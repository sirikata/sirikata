/*  Meru
 *  GraphicsObject.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _GRAPHICS_OBJECT_H_
#define _GRAPHICS_OBJECT_H_

#include "../meruCompat/MeruDefs.hpp"

namespace Meru {

/** Base class for all objects in the graphics system.
 *  Contains functions for initializing and destroying
 *  graphics objects.
 */
class GraphicsObject
{
public:
  GraphicsObject();
  virtual ~GraphicsObject();

  /** Performs one-time initialization of the GraphicsObject.
   *  This doesn't imply the object will be visible.  This should
   *  almost always be called immediately after construction.
   */
  virtual void initialize() = 0;

  /** Perform one-time destruction of GraphicsObject.
   *  This should be called immediately before destruction.
   */
  virtual void destroy() = 0;

  /** Returns true if this entity is complete, i.e. all information
   *  needed to display the entity is available and loaded.
   */
  virtual bool complete() = 0;

  /** Update the location of this GraphicsObject.
   *  \param loc the location to place this object at
   */
  virtual void location(const Location& loc) = 0;

  /** Get the current location of this GraphicsObject. */
  virtual Location location() = 0;

  /** Updates the graphics to be consistent for the next rendered frame, e.g. makes sure
   *  object is above ground.
   */
  virtual void frame(const Duration& elapsedTime) = 0;

  /**
   * Displays information necessary to select the objecct
   */
  virtual void setSelected(bool selected) = 0;

};

}

#endif
