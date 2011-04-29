/*  Sirikata
 *  vec3.em
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

system.require('std/core/pretty.em');

// This is just a document hack.
// System should not be in the if as there is something wrong if it
// does.
if(system == undefined)
{
  /** @namespace */
  system = new Object(); 
}

// (function() {
//      // Scope of this function hides the real print function and the callback from direct manipulation.

//      var realprint = system.print;
//      var printhandler = undefined;
//      var sendHome = system.sendHome;
//      var registerHandler = system.registerHandler;
//      var timeout = system.timeout;
//      var import = system.import;
//      var canSendMessage = system.canSendMessage;
//      var canRecvMessage = system.canRecvMessage;
//      var canProx = system.canProx;
//      var canImport = system.canImport;
//      var canCreatePresence = system.canCreatePresence;
//      var canCreateEntity = system.canCreateEntity;
//      var canEval = system.canEval;
//      var getPosition = system.getPosition;
//      var getVersion = system.getVersion;
//      var eval = system.eval;
//      var createSandbox = system.create_context;
//      var createPresence = system.create_presence;
//      var createEntity = system.create_entity;
    
//      var create_context = system.create_context;
//      var create_presence = system.create_presence;
//      var create_entity = system.create_entity;
      
//      var onPresenceConnected = system.onPresenceConnected;
//      var onPresenceDisconnected = system.onPresenceDisconnected;
//      var require = system.require;
     
//      var reset = system.reset;
//      var set_script = system.set_script;
//      var setScript = system.set_script;
      
     
      
       

    
//      system.onPrint = function(cb) {
//          printhandler = cb;
//      };
//      /** @function 
//          @description Prints the argument 
//          @see system.prettyPrint
//      */
//      system.print = function(/** Object */ obj) {
//          if (printhandler !== undefined && printhandler !== null)
//          {
//              printhandler.apply(this, arguments);                                  
//          }
//          else
//          {
//              realprint.apply(this, arguments);                                  
//          }
         
//      };
//      system.__debugPrint = function()
//      {
//          realprint.apply(this,arguments);
//      };
    
//     /** I an not defining any callback handlers. They can be, if required */
//     /** Since these functions are not getting added to the prototype of the system object, it will generate static functions */
//     /** If added to prototype, it generates the member functions in the documentation */
//     /** @function 

//       This function sends the msg object back to the visible associated with this sandbox
//       @return  void
//     */
//     system.sendHome = function(){
//       sendHome.apply(this, arguments);
//     }

//     // Not exposing this
//     /** @ignore */
//     system.registerHandler = function(){
//       return registerHandler.apply(this, arguments);
//     }

//     /** @function 
//       @param time number of seconds to wait before executing the callback
//       @param callback The function to invoke once "time" number of seconds have passed
//       @return a object representing a handle for this timer. This handle can be used in future to suspend and resume the timer
//     */
//     system.timeout = function(/** Number */time, /** function */ callback)
//     {
//       return timeout.apply(this, arguments);
//     }
    
//     /** @function
//       @param scriptFile The Emerson file to import and execute in the current script
//     */
//     system.import = function(/** String */ scriptFile)
//     {
//       import.apply(this, arguments);
//     }

//     /** @function 
//         @description Tells whether the script is allowed to send messages
//         @type Boolean
//         @return TRUE if the script is allowed to send a message, FALSE otherwise
//     */
//     system.canSendMessage = function()
//     {
//       return canSendMessage.apply(this, arguments);
//     }
    
//     /** @function 
//         @description Tells whether the script is allowed to register to receive messages
//         @type Boolean
//         @return TRUE if the script is allowed to receive a message, FALSE otherwise
//     */
//     system.canRecvMessage = function()
//     {
//       return canRecvMessage.apply(this, arguments);
//     }

//     /** @function 
//         @description Tells whether the script is allowed to register for proximity queries
//         @type Boolean
//         @return TRUE if the script is allowed to register for proximity queries, FALSE otherwise
//     */
//     system.canProx = function()
//     {
//       return canProx.apply(this, arguments);
//     }
    
//     /** @function 
//         @description Tells whether the script is allowed to import another emerson script
//         @type Boolean
//         @return TRUE if the script is allowed to import an emerson script using system.import, FALSE otherwise 
//     */
//     system.canImport = function()
//     {
//       return canImport.apply(this, arguments);
//     }

    
//     /** @function 
//         @description Tells whether the script is allowed to create a new entity
//         @type Boolean
//         @return TRUE if the script is allowed to create a new entity on the current host, FALSE otherwise
//     */
//     system.canCreateEntity = function()
//     {
//       return canCreateEntity.apply(this, arguments);
//     }
    
//     /** @function
//         @description Tells whether the script is allowed to create a new presence
//         @type Boolean
//         @return TRUE if the script is allowed to create a new presence for the entity, FALSE otherwise 
//     */    
//     system.canCreatePresence = function()
//     {
//       return canCreatePresence.apply(this, arguments);
//     }
//     /** @deprecated Use createSandbox */
//     system.create_context = function()
//     {
//       return create_context.apply(this, arguments);
//     }
//     /** @function 
//         @description Creates a new sandbox with associated capabilities
//         @type context  
//         @param presence The presence that the context is associated with. (will use this as sender of messages). If this arg is null, then just passes through the parent context's presence
//         @param visible The visible object that can always send messages to. if null, will use same spaceobjectreference as one passed in for arg0.
//         @param canSendMsg can I send messages to everyone?
//         @param canRecvMsg can I receive messages from everyone?
//         @param canProx can I make my own prox queries argument
//         @param canImport can I import argument
//         @param canCreatePresene can I create presences.
//         @param canCreateEntity can I create entites
//         @param canEval can I call eval directly through system object.


//         @see system.canSendMessage
//         @see system.canRecvMessage
//         @see system.canCreatePresence
//         @see system.canCreateEntity 
//     */
//     system.createSandbox = function(/** Presence */ presence, /** Visible */ visible, /** Boolean */ canSendMsg, /** Boolean */ canRecvMsg,
//                                     /** Boolean */ canProx, /** Boolean */ canImport, /** Boolean */ canCreatePresence, /** Boolean */ canCreateEntity,
//                                     /** Boolean */ canEval)
//     {
//       return createSandbox.apply(this, arguments);
//     }
//     /** @deprecated Use createPresence */
//     system.create_presence = function()
//     {
//       return create_presence.apply(this, arguments);
//     }
//     /** @function 
//       @description This function call creates a new presence for the entity running this script. 
//                    Note: Presence's initial position is the same as the presence that created it. Scale is set to 1.

//       @throws {Exception}  if sandbox does not have capability to create presences. 

//       @see system.canCreatePresence

//       @param mesh  a uri for a mesh for the new presence.
//       @param callback function to be called when presence gets connected to the world. (Function has form func (pres), where pres contains the presence just connected.)

//       @return Presence object. Presence is not connected to world until receive notification. (Ie, don't call setVelocity, setPosition, etc. until the second paramater has been called.)
//     */
//     system.createPresence = function(/** String */mesh, /** Function */ callback)
//     {
//       return createPresence.apply(this, arguments);
//     }
    
//     /** @deprecated  Use createEntity */
//     system.create_entity = function()
//     {
//       return create_entity.apply(this, arguments);
//     }
//     /** @function 
//         @description Creates a new entity on the current entity host. 

//         @throws {Exception} Calling create_entity in a sandbox without the capabilities to create entities throws an exception.

//         @see system.canCreateEntity
//         @param position (eg. new util.Vec3(0,0,0);). Corresponds to position to place new entity in world.
//         @param scriptOption Script option to pass in. Almost always pass "js"
//         @param initFile Name of file to import code for new entity from.
//         @param mesh Mesh uri corresponding to mesh you want to use for this entity.
//         @param scale Scale of new mesh. (Higher number means increase mesh's size.)
//         @param solidAngle Solid angle that entity's new presence queries with.

//     */
//     system.createEntity = function(/** util.Vec3 */ position, /** String */ scriptOption, /** String */ initFile, /** String */ mesh, /** Number */ scale, 
//                                   /** Number */ solidAngle)
//     {
//       return createEntity.apply(this, arguments);
//     }
//     /** @function 
//         @type Boolean
//         @return TRUE if system.eval() is invokable in the script
//         */

//     system.canEval = function()
//     {
//       return canEval.apply(this, arguments);
//     }

//     /** @function 
//         @description Returns the position of the default presence this script or sandbox is associated with
//         @type util.Vec3
//         @return vector corresponding to position of default presence sandbox is associated with.
//         @throws {Exception} Calling from root sandbox, or calling on a sandbox for which you do not have capabilities to query for position throws an exception.
//     */

//     system.getPosition = function()
//     {
//       return getPosition.apply(this, arguments);
//     }

//     /** @function 
//     @description Gives the version of Emerson run by the entity host
//     @type String
//     @return the version of the Emerson being run by the entity host*/
//     system.getVersion = function()
//     {
//       return getVersion.apply(this, arguments);
//     }

//     /** @function 
//         @description Registers a callback to be invoked when a presence created within this sandbox gets connected to the world
//         @return does not return anything
//         @param callback The function to be invoked. Function takes a single argument that corresponds to the presence that just connected to the world. 
//     */
//     system.onPresenceConnected = function(/** Function */ callback)
//     {
//       onPresenceConnected.apply(this, arguments);
//     }

//     /** @function 
//         @description Registers a callback to be invoked when a presence created within this sandbox gets disconnected from the world.
//         @return does not return anything
//         @param callback the function to be invoked. Function takes a single argument that corresponds to the presence that just got disconnected from the world. 
//         */
//     system.onPresenceDisconnected = function(/** Function */ callback)
//     {
//       onPresenceDisconnected.apply(this, arguments);
//     }

//     /** @function 
//         @description Library include mechanism. Calling require makes it so that system searches for file named by argument passed in. If system hasn't already executed this file, it reads file, and executes it.
//         @see system.import
//         @param filename The path to look for the file to include
//     */

//     system.require = function(/** String */ filename)
//     {
//       require.apply(this, arguments);
//     }

//     /** @function 
//         @description Destroys all created objects, except presences in the root context. Then executes script associated with root context. (Use system.setScript to set this script.)
//         @see system.setScript
//         @return Does not return any value 
//     */
//     system.reset = function()
//     {
//       reset.apply(this, arguments);
//     }

//     /** 
//         @deprecated  You should use setScript 
//     */
//     system.set_script = function()
//     {
//       set_script.apply(this, arguments);
//     }
    
//     /** @function 
//         @description Sets the script to be invoked when the system.reset is called
//         @see system.reset
//         @param script string representing the  emerson script to be invoked 
//         @return does not return any value
//     */
//     system.setScript = function(/** String */ script)
//     {
//       setScript.apply(this, arguments);
//     }

//     var presences = system.presences;
//     /** @field @type Array 
//         @description Used to store all the presences of the sandbox currrently connected to the world.
//     */
//     /** Array */ system.presences = presences;
//  }
// )();
/** @function 
    @description prints the argument in javascript/json style string format 
    @param a list of objects to print. Each of the objects are pretty-formatted
    concatenated before printing
*/
system.prettyprint = function(/** Object */ obj1, /** Object */ obj2) {
    res = '';
    for(var i = 0; i < arguments.length; i++)
        res += std.core.pretty(arguments[i]);
    system.print(res);
};

if (typeof(system.prototype) == "undefined") system.prototype = {};

/** @function 
    @type String
    @return the string representation of the system object
    @see system.print
    @see system.prettyprint*/
system.toString = function() {
    return "[object system]";
};
system.__prettyPrintString__ = "[object system]";
