/*  Sirikata
 *  shim.em
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



__system.require('std/core/bind.em');

(function()
  {
      var sysConstructor = function (baseSystem)
      {
        //self declarations
        this.addToSelfMap= function(toAdd)
        {
            var selfKey = (toAdd == null)? this.__NULL_TOKEN__: toAdd.toString();
            this._selfMap[selfKey] = toAdd;
        };

        this.__NULL_TOKEN__ = 'null';
        
        //data
        this._selfMap = { };

        //lkjs;
        //FIXME: should not default to presences[0], because may not exist yet.
        this.__behindSelf = baseSystem;


          this.__setBehindSelf = function(toSetTo)
          {
              this.__behindSelf = toSetTo;
          };
        
          this.__defineGetter__("self", function(){
                                    return this.__behindSelf;
                                });
        
          this.__defineSetter__("self", function(val){
                                });            


        //rest of functions
          var printhandler = undefined;
          
        // var realprint = baseSystem.print;

        // var sendHome = baseSystem.sendHome;
        // var import = baseSystem.import;
        // var canSendMessage = baseSystem.canSendMessage;
        // var canRecvMessage = baseSystem.canRecvMessage;
        // var canProx = baseSystem.canProx;
        // var canImport = baseSystem.canImport;
        // var canCreatePresence = baseSystem.canCreatePresence;
        // var canCreateEntity = baseSystem.canCreateEntity;
        // var canEval = baseSystem.canEval;
        // var getPosition = baseSystem.getPosition;
        // var getVersion = baseSystem.getVersion;
        // var eval = baseSystem.eval;
        // var createSandbox = baseSystem.create_context;
        // var createEntity = baseSystem.create_entity;
        
        // var create_context = baseSystem.create_context;
        // var create_presence = baseSystem.create_presence;
        // var create_entity = baseSystem.create_entity;

        // var require = baseSystem.require;
     
        // var reset = baseSystem.reset;
        // var set_script = baseSystem.set_script;
        // var setScript = baseSystem.set_script;
             

    
        this.onPrint = function(cb) {
            printhandler = cb;
        };

        /** @function 
         @description Prints the argument 
         @see system.prettyPrint
         */

        this.print = function(/** Object */ obj) {
            if (printhandler !== undefined && printhandler !== null)
            {
                printhandler.apply(this, arguments);                                  
            }
            else
            {
                baseSystem.print.apply(baseSystem, arguments);                                  
            }
        };
        
        this.__debugPrint = function()
        {
            baseSystem.print.apply(baseSystem,arguments);
        };
    
        /** I an not defining any callback handlers. They can be, if required */
        /** Since these functions are not getting added to the prototype of the system object, it will generate static functions */
        /** If added to prototype, it generates the member functions in the documentation */
        /** @function 

         This function sends the msg object back to the visible associated with this sandbox
         @return  void
         */
        this.sendHome = function(){
            baseSystem.sendHome.apply(baseSystem, arguments);
        };

          /** @function
           @description This function evaluates the emerson string that is passed in as its single argument.

           @param String to eval.
           */
          this.eval = function()
          {
              baseSystem.eval.apply(baseSystem,arguments);
          };
          
        // Not exposing this
        /** @ignore */
        this.registerHandler = function (callback,arg2,arg3,arg4)
        {
            var wrappedCallback = this.__wrapRegHandler(callback);
            //baseSystem.registerHandler.apply(baseSystem,wrappedCallback,arg2,arg3,arg4);
            //lkjs;
            baseSystem.registerHandler(wrappedCallback,arg2,arg3,arg4);
        };

        // Not exposing this
        /** @ignore */
        this.__wrapRegHandler = function (toCallback)
        {
            var returner = function (msg,sender,receiver)
            {
                this.__setBehindSelf(this._selfMap[receiver]);
                toCallback(msg,sender);
            };
            return std.core.bind(returner,this);
        };


        
        /** @function 
         @param time number of seconds to wait before executing the callback
         @param callback The function to invoke once "time" number of seconds have passed
         @return a object representing a handle for this timer. This handle can be used in future to suspend and resume the timer
         */
        this.timeout = function (/**Number*/timeUntil, /**function*/callback)
        {
            var selfKey = (this.self == null )? this.__NULL_TOKEN__ : this.self.toString();
            var wrappedFunction = this.__wrapTimeout(callback,selfKey);
            return baseSystem.timeout(timeUntil,wrappedFunction);
        };


        /** @ignore */
        this.__wrapTimeout= function(callback,toStringSelf)
        {
            var returner = function()
            {
                this.__setBehindSelf(this._selfMap[toStringSelf]);
                callback();
            };
            
            return std.core.bind(returner,this);
        };

        
        /** @function
         @param scriptFile The Emerson file to import and execute in the current script
         */
        this.import = function(/** String */ scriptFile)
        {
            baseSystem.import.apply(baseSystem, arguments);
        };

        /** @function 
         @description Tells whether the script is allowed to send messages
         @type Boolean
         @return TRUE if the script is allowed to send a message, FALSE otherwise
         */
        this.canSendMessage = function()
        {
            return baseSystem.canSendMessage.apply(baseSystem, arguments);
        };
    
        /** @function 
         @description Tells whether the script is allowed to register to receive messages
         @type Boolean
         @return TRUE if the script is allowed to receive a message, FALSE otherwise
         */
        this.canRecvMessage = function()
        {
            return baseSystem.canRecvMessage.apply(baseSystem, arguments);
        };

        /** @function 
         @description Tells whether the script is allowed to register for proximity queries
         @type Boolean
         @return TRUE if the script is allowed to register for proximity queries, FALSE otherwise
         */
        this.canProx = function()
        {
            return baseSystem.canProx.apply(baseSystem, arguments);
        };
    
        /** @function 
         @description Tells whether the script is allowed to import another emerson script
         @type Boolean
         @return TRUE if the script is allowed to import an emerson script using system.import, FALSE otherwise 
         */
        this.canImport = function()
        {
            return baseSystem.canImport.apply(baseSystem, arguments);
        };

    
        /** @function 
         @description Tells whether the script is allowed to create a new entity
         @type Boolean
         @return TRUE if the script is allowed to create a new entity on the current host, FALSE otherwise
         */
        this.canCreateEntity = function()
        {
            return baseSystem.canCreateEntity.apply(baseSystem, arguments);
        };
    
        /** @function
         @description Tells whether the script is allowed to create a new presence
         @type Boolean
         @return TRUE if the script is allowed to create a new presence for the entity, FALSE otherwise 
         */    
        this.canCreatePresence = function()
        {
            return baseSystem.canCreatePresence.apply(baseSystem, arguments);
        };

        
        /** @deprecated Use createSandbox */
        this.create_context = function()
        {
            return this.createSandbox.apply(this,arguments);//baseSystem.create_context.apply(baseSystem, arguments);
        };

        
        /** @function 
         @description Creates a new sandbox with associated capabilities
         @type context  
         @param presence The presence that the context is associated with. (will use this as sender of messages). If this arg is null, then just passes through the parent context's presence
         @param visible The visible object that can always send messages to. if null, will use same spaceobjectreference as one passed in for arg0.
         @param canSendMsg can I send messages to everyone?
         @param canRecvMsg can I receive messages from everyone?
         @param canProx can I make my own prox queries argument
         @param canImport can I import argument
         @param canCreatePresene can I create presences.
         @param canCreateEntity can I create entites
         @param canEval can I call eval directly through system object.

         @see system.canSendMessage
         @see system.canRecvMessage
         @see system.canCreatePresence
         @see system.canCreateEntity 
         */
        this.createSandbox = function(/** Presence */ presence, /** Visible */ visible, /** Boolean */ canSendMsg, /** Boolean */ canRecvMsg,
                                    /** Boolean */ canProx, /** Boolean */ canImport, /** Boolean */ canCreatePresence, /** Boolean */ canCreateEntity,
                                    /** Boolean */ canEval)
        {
            return baseSystem.createSandbox.apply(baseSystem, arguments);
        };

        
        /** @deprecated Use createPresence */
        this.create_presence = function()
        {
            return this.createPresence.apply(this,arguments);
        };

        /** @function 
         @description This function call creates a new presence for the entity running this script. 
         Note: Presence's initial position is the same as the presence that created it. Scale is set to 1.
         
         @throws {Exception}  if sandbox does not have capability to create presences. 
         
         @see system.canCreatePresence

         @param mesh  a uri for a mesh for the new presence.
         @param callback function to be called when presence gets connected to the world. (Function has form func (pres), where pres contains the presence just connected.)

         @return Presence object. Presence is not connected to world until receive notification. (Ie, don't call setVelocity, setPosition, etc. until the second paramater has been called.)
         */
        this.createPresence = function (/** String */mesh, /** Function */ callback)
        {
            //must be this way.
            //baseSystem.create_presence.apply(baseSystem,arguments);
            //baseSystem.create_presence(mesh,callback);
            //baseSystem.create_presence.apply(baseSystem,[mesh,this.__wrapPresConnCB(callback)]);
            baseSystem.create_presence(mesh,this.__wrapPresConnCB(callback));
        };


        
        /** @deprecated  Use createEntity */
        this.create_entity = function()
        {
            return this.createEntity.apply(this,arguments);//baseSystem.create_entity.apply(baseSystem, arguments);
        };
        
        /** @function 
         @description Creates a new entity on the current entity host. 

         @throws {Exception} Calling create_entity in a sandbox without the capabilities to create entities throws an exception.

         @see system.canCreateEntity
         @param position (eg. new util.Vec3(0,0,0);). Corresponds to position to place new entity in world.
         @param scriptOption Script option to pass in. Almost always pass "js"
         @param initFile Name of file to import code for new entity from.
         @param mesh Mesh uri corresponding to mesh you want to use for this entity.
         @param scale Scale of new mesh. (Higher number means increase mesh's size.)
         @param solidAngle Solid angle that entity's new presence queries with.
         */
        this.createEntity = function(/** util.Vec3 */ position, /** String */ scriptOption, /** String */ initFile, /** String */ mesh, /** Number */ scale, /** Number */ solidAngle)
        {
            return baseSystem.create_entity.apply(baseSystem, arguments);
        };
        
        /** @function 
         @type Boolean
         @return TRUE if system.eval() is invokable in the script
         */
        this.canEval = function()
        {
            return baseSystem.canEval.apply(baseSystem, arguments);
        };
        
        /** @function 
         @description Returns the position of the default presence this script or sandbox is associated with
         @type util.Vec3
         @return vector corresponding to position of default presence sandbox is associated with.
         @throws {Exception} Calling from root sandbox, or calling on a sandbox for which you do not have capabilities to query for position throws an exception.
         */
        this.getPosition = function()
        {
            return baseSystem.getPosition.apply(baseSystem, arguments);
        };

        /** @function 
         @description Gives the version of Emerson run by the entity host
         @type String
         @return the version of the Emerson being run by the entity host*/
        this.getVersion = function()
        {
            return baseSystem.getVersion.apply(baseSystem, arguments);
        };


        /** @function 
         @description Registers a callback to be invoked when a presence created within this sandbox gets connected to the world
         @return does not return anything
         @param callback The function to be invoked. Function takes a single argument that corresponds to the presence that just connected to the world. 
         */        
        this.onPresenceConnected = function(/**Function */callback)
        {
            //baseSystem.onPresenceConnected.apply(baseSystem,[this.__wrapPresConnCB(callback)]);
            baseSystem.onPresenceConnected(this.__wrapPresConnCB(callback));
        };

                
        /** @function 
         @description Registers a callback to be invoked when a presence created within this sandbox gets disconnected from the world.
         @return does not return anything
         @param callback the function to be invoked. Function takes a single argument that corresponds to the presence that just got disconnected from the world. 
         */
        this.onPresenceDisconnected = function (/**Function*/callback)
        {
            //baseSystem.onPresenceDisconnected.apply(baseSystem,[this.__wrapPresConnCB(callback)]);
            baseSystem.onPresenceDisconnected(this.__wrapPresConnCB(callback));
        };

        //not exposing
        /** @ignore */
        this.__wrapPresConnCB = function(callback)
        {
            var returner = function(presConn)
            {
                this.addToSelfMap(presConn);
                this.__setBehindSelf(presConn);
                callback(presConn);
            };

            return std.core.bind(returner,this);
        };

          
          this.__sys_onProxAdded= function (presCalling, funcToCall)
          {
              var wrappedCallback = this.__wrapOnProx(presCalling,funcToCall);
              presCalling.__hidden_onProxAdded(wrappedCallback);
          };

          this.__sys_onProxRemoved = function (presCalling, funcToCall)
          {
              var wrappedCallback = this.__wrapOnProx(presCalling,funcToCall);
              presCalling.__hidden_onProxRemoved(wrappedCallback);
          };
        
          this.__wrapOnProx = function (presCalling,funcToCall)
          {
              var returner = function(newVis)
              {
                  this.__setBehindSelf(presCalling);
                  funcToCall(newVis);
              };
              
              return std.core.bind(returner,this);
          };


        

        /** @function 
         @description Library include mechanism. Calling require makes it so that system searches for file named by argument passed in. If system hasn't already executed this file, it reads file, and executes it.
         @see system.import
         @param filename The path to look for the file to include
         */
        this.require = function(/** String */ filename)
        {
            baseSystem.require.apply(baseSystem, arguments);
        };

        /** @function 
         @description Destroys all created objects, except presences in the root context. Then executes script associated with root context. (Use system.setScript to set this script.)
         @see system.setScript
         @return Does not return any value 
         */
        this.reset = function()
        {
            baseSystem.reset.apply(baseSystem, arguments);
        };

        /** 
         @deprecated  You should use setScript 
         */
        this.set_script = function()
        {
            this.setScript.apply(this,arguments);
        };
    
        /** @function 
         @description Sets the script to be invoked when the system.reset is called
         @see system.reset
         @param script string representing the  emerson script to be invoked 
         @return does not return any value
         */
        this.setScript = function(/** String */ script)
        {
            baseSystem.setScript.apply(baseSystem, arguments);
        };

        var presences = baseSystem.presences;
        /** @field @type Array 
         @description Used to store all the presences of the sandbox currrently connected to the world.
         */
        /** Array */ this.presences = presences;
      };

      system = new sysConstructor(__system);
      system.__presence_constructor__ = __system.__presence_constructor__;
      
  })();







/*
  This is the main shim layer created between Emerson and C++. This is
  because we want some basic functionality to be in C++ while creating
  a layer of other essential functionalities directly in Emerson. We
  save dev time with this and we are not too much bothered about
  performance. IF performance degrades beyond tolerance, we will
  implement those functionalities in C++.
*/

system.shim = new Object();

system.require('std/shim/system.em');
system.require('std/shim/class.em');
system.require('std/shim/presence.em');
system.require('std/shim/vec3.em');
system.require('std/shim/quaternion.em');
