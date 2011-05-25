/*  Sirikata
 *  system.em
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


(function()
 {
      var baseSystem = __system;
      var isResetting = false;
     
      system = {};

      //self declarations
      system.addToSelfMap= function(toAdd)
      {
          var selfKey = (toAdd == null)? this.__NULL_TOKEN__: toAdd.toString();
          this._selfMap[selfKey] = toAdd;
      };

     system.printSelfMap = function()
     {
         __system.print('\nPrinting self map\n');
         for (var s in this._selfMap)
         {
             __system.print(s);
             __system.print('\n');
             __system.print(this._selfMap[s]);
             __system.print('\n');
             
         }
         __system.print('\n\n');
     };
     
      system.__NULL_TOKEN__ = 'null';

      //data
      system._selfMap = { };

      system.__behindSelf = system;

      system.__setBehindSelf = function(toSetTo)
      {
          this.__behindSelf = toSetTo;
      };

      system.__defineGetter__("self", function(){
                                return system.__behindSelf;
                            });

      system.__defineSetter__("self", function(val){
                            });


      //rest of functions
      var printhandler = undefined;


      system.onPrint = function(cb) {
          printhandler = cb;
      };

      /** @function
       @description Prints the argument
       @see system.prettyPrint
       */

      system.print = function(/** Object */ obj) {
          if (printhandler !== undefined && printhandler !== null)
          {
              printhandler.apply(this, arguments);
          }
          else
          {
              baseSystem.print.apply(baseSystem, arguments);
          }
      };

      system.__debugPrint = function()
      {
          baseSystem.print.apply(baseSystem,arguments);
      };


     /** @function
      @param string space and object id of a visible object.  
      
      @return a visible object with the space and object id contained argument.
      
      Throws an exception if string is incorrectly formatted, otherwise returns vis object.
      */
      system.createVisible = function(/**String**/strToCreateFrom)
      {
          return baseSystem.createVisible.apply(baseSystem,arguments);
      };

      
      /** I an not defining any callback handlers. They can be, if required */
      /** Since these functions are not getting added to the prototype of the system object, it will generate static functions */
      /** If added to prototype, it generates the member functions in the documentation */
      /** @function

       This function sends the msg object back to the visible associated with this sandbox
       @return  void
       */
      system.sendHome = function(){
          baseSystem.sendHome.apply(baseSystem, arguments);
      };


      /** @function
       @param string space and object id of a visible object.

       @return a visible object with the space and object id contained argument.

       Throws an exception if string is incorrectly formatted, otherwise returns vis object.
       */
      system.createVisible = function(/**String**/strToCreateFrom){
          return baseSystem.createVisible.apply(baseSystem,arguments);
      };


      /** @function
       @description This function evaluates the emerson string that is passed in as its single argument.

       @param String to eval.
       */
      system.eval = function()
      {
          baseSystem.eval.apply(baseSystem,arguments);
      };

      // Not exposing this
      /** @ignore */
      system.registerHandler = function (callback,pattern,sender,issusp)
      {
          var wrappedCallback = this.__wrapRegHandler(callback);
          if (typeof(issusp) =='undefined')
              return baseSystem.registerHandler(wrappedCallback,pattern,sender);

          return baseSystem.registerHandler(wrappedCallback,pattern,sender,issusp);
      };

      // Not exposing this
      /** @ignore */
      system.__wrapRegHandler = function (toCallback)
      {
          var returner = function (msg,sender,receiver)
          {
              system.__setBehindSelf(system._selfMap[receiver]);
              toCallback(msg,sender,receiver);

          };
          return std.core.bind(returner,this);
      };

      /** @function
       @return returns the script that was set by setScript, and that is associated with this sandbox.

       */
      system.getScript = function()
      {
          return baseSystem.getScript.apply(baseSystem,arguments);
      };


      /** @function
       @param time number of seconds to wait before executing the callback
       @param callback The function to invoke once "time" number of seconds have passed

       @param {Reserved} uint32 contextId
       @param {Reserved} double timeRemaining
       @param {Reserved} bool   isSuspended
       @param {Reserved} bool   isCleared
       
       @return a object representing a handle for this timer. This handle can be used in future to suspend and resume the timer
       */
      system.timeout = function (/**Number*/period, /**function*/callback, /**uint32*/contextId, /**double*/timeRemaining,/**bool*/isSuspended, /**bool*/isCleared)
      {
          var selfKey = (this.self == null )? this.__NULL_TOKEN__ : this.self.toString();
          var wrappedFunction = this.__wrapTimeout(callback,selfKey);
          if (typeof(isCleared) == 'undefined')
              return baseSystem.timeout(period,wrappedFunction);

          return baseSystem.timeout(period,wrappedFunction,contextId,timeRemaining,isSuspended,isCleared);
      };


      /** @ignore */
      system.__wrapTimeout= function(callback,toStringSelf)
      {
          var returner = function()
          {
              this.__setBehindSelf(this._selfMap[toStringSelf]);
              callback();
          };

          return std.core.bind(returner,this);
      };


      /** @function

       @param Which presence to send from.
       @param Message object to send.
       @param Visible to send to.
       @param (Optional) Error handler function.
       */
      system.sendMessage = function()
      {
          baseSystem.sendMessage.apply(baseSystem, arguments);
      };

      /** @function
       @param scriptFile The Emerson file to import and execute in the current script
       */
      system.import = function(/** String */ scriptFile)
      {
          baseSystem.import.apply(baseSystem, arguments);
      };

      /** @function
       @description Tells whether the script is allowed to send messages
       @type Boolean
       @return TRUE if the script is allowed to send a message, FALSE otherwise
       */
      system.canSendMessage = function()
      {
          return baseSystem.canSendMessage.apply(baseSystem, arguments);
      };

      /** @function
       @description Tells whether the script is allowed to register to receive messages
       @type Boolean
       @return TRUE if the script is allowed to receive a message, FALSE otherwise
       */
      system.canRecvMessage = function()
      {
          return baseSystem.canRecvMessage.apply(baseSystem, arguments);
      };

      /** @function
       @description Tells whether the script is allowed to register for proximity queries
       @type Boolean
       @return TRUE if the script is allowed to register for proximity queries, FALSE otherwise
       */
      system.canProx = function()
      {
          return baseSystem.canProx.apply(baseSystem, arguments);
      };

      /** @function
       @description Tells whether the script is allowed to import another emerson script
       @type Boolean
       @return TRUE if the script is allowed to import an emerson script using system.import, FALSE otherwise
       */
      system.canImport = function()
      {
          return baseSystem.canImport.apply(baseSystem, arguments);
      };


      /** @function
       @description Tells whether the script is allowed to create a new entity
       @type Boolean
       @return TRUE if the script is allowed to create a new entity on the current host, FALSE otherwise
       */
      system.canCreateEntity = function()
      {
          return baseSystem.canCreateEntity.apply(baseSystem, arguments);
      };

      /** @function
       @description Tells whether the script is allowed to create a new presence
       @type Boolean
       @return TRUE if the script is allowed to create a new presence for the entity, FALSE otherwise
       */
      system.canCreatePresence = function()
      {
          return baseSystem.canCreatePresence.apply(baseSystem, arguments);
      };


      /** @deprecated Use createSandbox */
      system.create_context = function()
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
      system.createSandbox = function(/** Presence */ presence, /** Visible */ visible, /** Boolean */ canSendMsg, /** Boolean */ canRecvMsg,
          /** Boolean */ canProx, /** Boolean */ canImport, /** Boolean */ canCreatePresence, /** Boolean */ canCreateEntity,
          /** Boolean */ canEval)
      {
          return baseSystem.create_context.apply(baseSystem, arguments);
      };


      //not exposing
      /** @ignore */
      system.__wrapPresConnCB = function(callback)
      {
          var returner = function(presConn)
          {
              this.addToSelfMap(presConn);
              this.__setBehindSelf(presConn);
              if (typeof(callback) === 'function')
                  callback(presConn);
          };

          return std.core.bind(returner,this);
      };


          /** @function
           @description Creates a new entity based on the position and space of presence passed in.
           
           @param presence or visible.
           @param vec3.  Position of new entity's first presence relative to first argument's position
           @param script. Can either be a string, which will be eval-ed on new entity as soon as it's created or can be a non-closure capturing function that will be executed with next parameter as its argument.
           @param Object.  Passed as argument to script argument if script argument is a function.
           @param solidAngle Solid angle that entity's new presence queries with.
           @param {optional} Mesh uri corresponding to mesh you want to use for this entity.  If undefined, defaults to self's mesh.
           @param {optional} scale Scale of new mesh. (Higher number means increase mesh's size.)  If undefined, default to self's scale.
           */
          system.createEntityFromPres = function(pres, pos, script,arg, solidAngle,mesh,scale)
          {
              var newSpace = pres.getSpaceID();
              var newPos = pres.getPosition() + pos;
              this.createEntityScript(newPos,script,arg,mesh,scale,solidAngle,newSpace);
          };

          /** @function
           @deprecated Use createEntityScript instead.
           
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
          system.createEntity = function(/** util.Vec3 */ position, /** String */ scriptOption, /** String */ initFile, /** String */ mesh, /** Number */ scale, /** Number */ solidAngle)
          {
              var wrapImport = "system.import('" + initFile + "');";
              return this.createEntityScript(position, wrapImport,null, solidAngle,mesh,scale);
          };


      

          /** @function
           @description Creates a new entity on the current entity host.
           
           @throws {Exception} Calling create_entity in a sandbox without the capabilities to create entities throws an exception.

           @see system.canCreateEntity
           @param position (eg. new util.Vec3(0,0,0);). Corresponds to position to place new entity in world.
           @param Script.  Can either be a string, which will be eval-ed on new entity as soon as it's created or can be a non-closure capturing function that will be executed with next parameter as its argument.
           @param Object.  Passed as argument to script argument if script argument is a function.  Null if takes no argument.
           @param solidAngle Solid angle that entity's new presence queries with.
           @param {optional} Mesh uri corresponding to mesh you want to use for this entity.  If undefined, defaults to self's mesh.
           @param {optional} scale Scale of new mesh. (Higher number means increase mesh's size.)  If undefined, default to self's scale.
           @param {optional} Space to create the entity in.  If undefined, defaults to self.
           */
          system.createEntityScript = function (position, script,arg,solidAngle,mesh,scale,space)
          {
              if (typeof(mesh) === 'undefined')
                  mesh = this.self.getMesh();
              if (typeof (scale) === 'undefined')
                  scale = this.self.getScale();
              if (typeof (space) === 'undefined')
                  space = this.self.getSpaceID();

                          
              var emersonSyntaxError = function (str)
              {
                  var toThrow = '20';
                  var tmp = 'throw ' + toThrow + ';' + str;
                  try
                  {
                      eval(tmp);
                  }
                  catch (excep)
                  {
                      if (excep === toThrow)
                          return false;  //there is no emerson syntax error
                      else
                          return excep;   //there is an emerson syntax error
                  }
              };

              var quoteEscaper = function (str)
              {
                  for (var s=0; s < str.length; ++s)
                  {
                      if (str.charAt(s) == '"')
                      {
                          str = str.substring(0,s) + '\\' + str.substring(s);
                          s+=1;
                      }
                  }
                  return str;    
              };
              
              //handle script is function
              if (typeof(script) == 'function')
              {
                  var funcString = quoteEscaper(script.toString());
                  if (arg !== null)
                  {
                      var serializedArg = quoteEscaper(this.serialize(arg));
                      funcString = '"' + '(' + funcString + ") ( system.deserialize(@" + serializedArg + "@));" + '"';
                  }
                  else
                      funcString = '"' + '(' + funcString + ") ( );" + '"';


                  this.__hidden_createEntity(position, 'js',funcString,mesh,scale,solidAngle,space);
              }
              else if (typeof(script) == 'string')
              {
                  var emSynError = emersonSyntaxError(script);
                  if (emSynError !== false)
                  {
                      throw "Error calling createEntity.  String passed in must be valid Emerson.  " + emSynError;
                  }

                  this.__hidden_createEntity(position,'js','"' +  quoteEscaper(script) + '"',mesh,scale,solidAngle,space);
              }
              else
              {
                  throw "Error.  Second argument to createEntity must contain a string or a function to execute on new entity.";
              }
              
          };


      
      /** @ignore */
      system.__hidden_createEntity = function(/** util.Vec3 */ position, /** String */ scriptOption, /** String */ scriptString, /** String */ mesh, /** Number */ scale, /** Number */ solidAngle)
      {
          return baseSystem.create_entity.apply(baseSystem, arguments);
      };

      
      /** @function
       @param Object to be serialized.

       @return Returns a string representing the serialized object.
       Takes an object and serializes it to be sent over the network, producing a string.
       */
      system.serialize = std.core.bind(baseSystem.serialize,baseSystem);

      /** @function
       @param String to deserialize into an object.
       @return Returns an object representing the deserialized string. 
       */
      system.deserialize = std.core.bind(baseSystem.deserialize,baseSystem);
          

      
        /** @function
         @description This function call creates a new presence for the entity running this script.
         Note: Presence's initial position is the same as the presence that created it. Scale is set to 1.

         @throws {Exception}  if sandbox does not have capability to create presences.

         @see system.canCreatePresence

         @param mesh  a uri for a mesh for the new presence.
         @param callback function to be called when presence gets connected to the world. (Function has form func (pres), where pres contains the presence just connected.)
         @param {optional} A position for the new presence.  Unspecified defaults to same position as self.
         @param {optional} A space to create the new presence in.  Unspecified defaults to same space as self.
         @return Presence object. Presence is not connected to world until receive notification. (Ie, don't call setVelocity, setPosition, etc. until the second paramater has been called.)
         */
        system.createPresence = function (/** String */mesh, /** Function */ callback, /**Vec3*/position, space)
        {
            if ((typeof(space) == 'undefined') || (space === null))
                space = this.self.getSpaceID();
            if ((typeof(position) == 'undefined') || (position === null))
                position = this.self.getPosition();
            
            baseSystem.create_presence(mesh,this.__wrapPresConnCB(callback),position,space);
        };


     
     /**
      @param {string} sporef,
      @param {vec3} pos,
      @param {vec3} vel,
      @param {string} posTime,
      @param {quaternion} orient,
      @param {quaternion} orientVel,
      @param {string} orientTime,
      @param {string} mesh,
      @param {number} scale,
      @param {boolean} isCleared ,
      @param {uint32} contextId,
      @param {boolean} isConnected,
      @param {function,null} connectedCallback,
      @param {boolean} isSuspended,
      @param {vec3} suspendedVelocity,
      @param {quaternion} suspendedOrientationVelocity,
      */
     system.restorePresence = function()
     {
         return baseSystem.restorePresence.apply(baseSystem,arguments);
     };
      
      /** @deprecated Use createPresence */
      system.create_presence = function()
      {
          return this.createPresence.apply(this,arguments);
      };



      /** @deprecated  Use createEntity */
      system.create_entity = function()
      {
          return this.createEntity.apply(this,arguments);
      };


      /** @function
       @type Boolean
       @return TRUE if system.eval() is invokable in the script
       */
      system.canEval = function()
      {
          return baseSystem.canEval.apply(baseSystem, arguments);
      };

      /** @function
       @description Returns the position of the default presence this script or sandbox is associated with
       @type util.Vec3
       @return vector corresponding to position of default presence sandbox is associated with.
       @throws {Exception} Calling from root sandbox, or calling on a sandbox for which you do not have capabilities to query for position throws an exception.
       */
      system.getPosition = function()
      {
          return baseSystem.getPosition.apply(baseSystem, arguments);
      };

      /** @function
       @description Gives the version of Emerson run by the entity host
       @type String
       @return the version of the Emerson being run by the entity host*/
      system.getVersion = function()
      {
          return baseSystem.getVersion.apply(baseSystem, arguments);
      };


      /** @function
       @description Registers a callback to be invoked when a presence created within this sandbox gets connected to the world
       @return does not return anything
       @param callback The function to be invoked. Function takes a single argument that corresponds to the presence that just connected to the world.
       */
      system.onPresenceConnected = function(/**Function */callback)
      {
          baseSystem.onPresenceConnected(this.__wrapPresConnCB(callback));
      };

      /** @function
       @description Registers a callback to be invoked when a presence created within this sandbox gets disconnected from the world.
       @return does not return anything
       @param callback the function to be invoked. Function takes a single argument that corresponds to the presence that just got disconnected from the world.
       */
      system.onPresenceDisconnected = function (/**Function*/callback)
      {
          baseSystem.onPresenceDisconnected(this.__wrapPresConnCB(callback));
      };


      system.__sys_onProxAdded= function (presCalling, funcToCall)
      {
          var wrappedCallback = this.__wrapOnProx(presCalling,funcToCall);
          presCalling.__hidden_onProxAdded(wrappedCallback);
      };

      system.__sys_onProxRemoved = function (presCalling, funcToCall)
      {
          var wrappedCallback = this.__wrapOnProx(presCalling,funcToCall);
          presCalling.__hidden_onProxRemoved(wrappedCallback);
      };

      system.__wrapOnProx = function (presCalling,funcToCall)
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
      system.require = function(/** String */ filename)
      {
          baseSystem.require.apply(baseSystem, arguments);
      };

      /** @function
       @description Destroys all created objects, except presences in the root context. Then executes script associated with root context. (Use system.setScript to set this script.)
       @see system.setScript
       @return Does not return any value
       */
      system.reset = function()
      {
          try
          {
              baseSystem.reset.apply(baseSystem, arguments);
              isResetting = true;
              throw '__resetting__';
          }
          catch (exception)
          {
              throw exception;
          }
      };

     system.__isResetting = function()
     {
         return isResetting;
     };
     
      /**
       @deprecated  You should use setScript
       */
      system.set_script = function()
      {
          this.setScript.apply(this,arguments);
      };

      /** @function
       @description Sets the script to be invoked when the system.reset is called
       @see system.reset
       @param script string representing the  emerson script to be invoked
       @return does not return any value
       */
      system.setScript = function(/** String */ script)
      {
          baseSystem.set_script.apply(baseSystem, arguments);
      };

      /** @field @type Array
       @description Used to store all the presences of the sandbox currrently connected to the world.
       */
      /** Array */ system.presences = baseSystem.presences;

      system.__presence_constructor__ = __system.__presence_constructor__;
      system.__visible_constructor__ = __system.__visible_constructor__;


      // Invoking these force the callbacks to be registered, making
      // system.self work in all cases, even though the callbacks we
      // pass are undefined.
      system.onPresenceConnected(undefined);
      system.onPresenceDisconnected(undefined);

  })();


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
