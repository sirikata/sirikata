/*  Sirikata
 *  presence.em
 *
 *  Copyright (c) 2011, Bhupesh Chandra
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


/* Add functions to the prototype for presence */

system.__presence_constructor__.prototype.runSimulation =
function(name)
{
  system.print("\n\nIn side the runSimulation\n\n");
  var pres = this;
  if(!system.shim.graphics)
  {
      system.print("\n\n Got the new graphics object\n\n");
      system.shim.graphics = new Object();
  }
  if(!system.shim.graphics[pres])
  {
    system.shim.graphics[pres] = new Object();
    system.print("\n\nBefore _runSimulation\n\n");
    system.shim.graphics[pres][name] = pres._runSimulation(name);
  }

  var return_val = system.shim.graphics[pres][name];

	return return_val;
};

system.__presence_constructor__.prototype.getSimulation =
function(name)
{

  if(!system.shim.graphics || !system.shim.graphics[this]) return undefined;
  return system.shim.graphics[this][name];
};

system.__presence_constructor__.prototype.onProxAdded =
function (funcToCallback)
{
    system.__sys_onProxAdded(this,funcToCallback);
};

system.__presence_constructor__.prototype.onProxRemoved =
function (funcToCallback)
{
    system.__sys_onProxRemoved(this,funcToCallback);
};



Object.defineProperty(system.__presence_constructor__.prototype, "position",
                      {
                          get: function() { return this.getPosition(); },
                          set: function() { return this.setPosition.apply(this, arguments); },
                          enumerable: true
                      }
);

Object.defineProperty(system.__presence_constructor__.prototype, "velocity",
                      {
                          get: function() { return this.getVelocity(); },
                          set: function() { return this.setVelocity.apply(this, arguments); },
                          enumerable: true
                      }
);

Object.defineProperty(system.__presence_constructor__.prototype, "orientation",
                      {
                          get: function() { return this.getOrientation(); },
                          set: function() { return this.setOrientation.apply(this, arguments); },
                          enumerable: true
                      }
);


Object.defineProperty(system.__presence_constructor__.prototype, "orientationVel",
                      {
                          get: function() { return this.getOrientationVel(); },
                          set: function() { return this.setOrientationVel.apply(this, arguments); },
                          enumerable: true
                      }
);


Object.defineProperty(system.__presence_constructor__.prototype, "scale",
                      {
                          get: function() { return this.getScale(); },
                          set: function() { return this.setScale.apply(this, arguments); },
                          enumerable: true
                      }
);


Object.defineProperty(system.__presence_constructor__.prototype, "mesh",
                      {
                          get: function() { return this.getMesh(); },
                          set: function() { return this.setMesh.apply(this, arguments); },
                          enumerable: true
                      }
);

system.__presence_constructor__.prototype.__prettyPrintFieldsData__ = [
    "position", "velocity",
    "orientation", "orientationVel",
    "scale", "mesh"
];
system.__presence_constructor__.prototype.__prettyPrintFields__ = function() {
    return this.__prettyPrintFieldsData__;
};
