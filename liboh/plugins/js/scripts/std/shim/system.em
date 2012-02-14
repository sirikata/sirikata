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
system.require('std/escape.em');


// This is just a document hack.
// System should not be in the if as there is something wrong if it
// does.
if(system == undefined)
{
  /** @namespace */
  system = new Object();
}

/**
 Should only be instantiated from featureObject.em

 Registers self as proximity manager.
 */

/**@ignore
 Each presence entry object manages the proximity result set for a presence.
 @param {string} sporef The space object reference for the presence, presObj.
 @param {presence} presObj The actual js presence object.
 */
function PresenceEntry(sporef, presObj,proxMan)
{
    this.sporef  = sporef;
    this.presObj = presObj;
    this.proxMan = proxMan;
}

PresenceEntry.prototype.getProxResultSet = function()
{
    if (typeof(this.proxMan) == 'undefined')
        throw new Error ('\n\nError proxMan is not defined\n\n');
    
    return this.proxMan.getProxResultSet(this.sporef);
};

PresenceEntry.prototype.__getType = function()
{
    return 'presenceEntry';
};

PresenceEntry.prototype.setProxAddCB = function (proxAddCB)
{
    return this.proxMan.setProxAddCB(this.presObj,proxAddCB);
};

PresenceEntry.prototype.setProxRemCB = function (proxRemCB)
{
    return this.proxMan.setProxRemCB(this.presObj,proxRemCB);
};

PresenceEntry.prototype.delProxAddCB = function(addID)
{
    return this.proxMan.delProxAddCB(this.presObj,addID);
};

PresenceEntry.prototype.delProxRemCB = function(remID)
{
    return this.proxMan.delProxRemCB(this.presObj,remID);
};

PresenceEntry.prototype.proxAddedEvent = function(visibleObj,visTo)
{
    return this.proxMan.proxAddedEvent(visTo,visibleObj);
};

PresenceEntry.prototype.proxRemovedEvent = function (visibleObj,visTo)
{
    return this.proxMan.proxRemovedEvent(visTo,visibleObj);
};




(function()
 {
     var baseSystem = __system;
     var isResetting = false;
     var isKilling   = false;

     var proxManager = null;
     var sboxMessageManager = null;
     var presMessageManager = null;


     var entityToken = baseSystem.getUniqueToken();
     
     system = {};

     system.__registerProxManager = function(proxMan)
     {
         proxManager = proxMan;
     };
     
     system.__getType = function()
     {
       return 'system';
     };
     

      //self declarations
      system.addToSelfMapAndPresencesArray = function(toAdd)
      {
          var selfKey = (toAdd == null) ? this.__NULL_TOKEN__ : toAdd.toString();
          if (selfKey in this._selfMap)
              return;

          this._selfMap[selfKey] = new PresenceEntry(selfKey,toAdd,proxManager);
          if (toAdd !== null)
              system.presences.push(toAdd);
      };

     system.printSelfMap = function()
     {
         __system.print('\nPrinting self map\n');
         for (var s in this._selfMap)
         {
             __system.print(s);
             __system.print('\n');
             this._selfMap[s].debugPrint(__system.print);
             __system.print('\n');
             
         }
         __system.print('\n\n');
     };
     
     system.__NULL_TOKEN__ = 'null';

     system.getAllData = function()
     {
         return system._selfMap;
     };

     /**
      @param {string} toChangeTo  The sporef of the presence that we want to change self to.
      */
     system.changeSelf  = function(toChangeTo)
     {
         if (toChangeTo === undefined) {
             system.__setBehindSelf(undefined);
         }
         else {
             var p = system._selfMap[toChangeTo];
             if (p === undefined) throw new Error("Can't change self to " + toChangeTo + " because that presence doesn't exist.");
             system.__setBehindSelf(p.presObj);
         }
     };

     /** Wrap a callback in a function which restores system.self to
      *  the value when the callback was setup.
      */
     system.wrapCallbackForSelf = function(cb) {
         var oldSelf = system.self;

         return function() {
             system.__setBehindSelf(oldSelf);
             cb.apply(undefined, arguments);
         };
     };     


     /**
      Changes the directory from which we look for scripts.
      
      @param {String} newDir Directory we check first when resolving
      imports/requires.
      */
     system.__pushEvalContextScopeDirectory = function (newDir)
     {
         return baseSystem.__pushEvalContextScopeDirectory.apply(
             baseSystem,arguments);
     };

     system.__popEvalContextScopeDirectory = function()
     {
         return baseSystem.__popEvalContextScopeDirectory.apply(
             baseSystem,arguments);
     };
     
     /**
      @ignore

      Takes in a string and runs it thorugh the emerson
      compiler.  Returns the string that the emerson compiler spits
      out, or throws an error if there is a syntax error, etc.

      Used by eval over-writing in compiler.  (If we just directly
      called eval on a string, then that string goes straight to JS.
      We instead turn a call such as:

      eval (someString) into

      eval (system.__emersonCompileString(someString) );
      
      */
     system.__emersonCompileString = function(toCompile)
     {
         return baseSystem.__emersonCompileString.apply(baseSystem,arguments);
     };


     /**
      @param {String} type (GET or POST) are only two supported for now.
      @param {String} url or ip address.
      @param {String} headers formatted as a string (with \r\n's ending each line)
      @param {function} callback to execute on success or failure (first arg of
      function is bool.  If success, bool is true, if fail, bool is false).
      Success callbacks have a second arg that takes in an object with the
      following fields:
        respHeaders (string map).
        contentLength (number).
        status code (number).
        data (string).
      */
     system.basicHttpGet = function(type, url, headers, cb)
     {
         return baseSystem.http(type,url, headers, system.wrapCallbackForSelf(cb));
     };

     /**
      @ignore
      Necessary so that script messages will be evaluated in global object rather than from within scriptable.
      */
     system.__evalInGlobal = function(toEval)
     {
         return baseSystem.__evalInGlobal.apply(baseSystem, arguments);
     };
     
     
     //storage manipulations
     system.storageBeginTransaction = function()
     {
         return baseSystem.storageBeginTransaction.apply(baseSystem, arguments);
     };
     
     /**
      
        @param {function} (optional) callback to execute after commit is
        complete.  This function takes two arguments: 1) bool (whether the
        commit succeeded or failed; 2) an object containing all the objects that
        we requested to read as fields (if we requested no reads, then this is
        undefined.
      */
     system.storageCommit = function()
     {
         if (arguments.length == 1 && typeof(arguments[0] === 'function'))
             return baseSystem.storageCommit.apply(baseSystem, [ system.wrapCallbackForSelf(arguments[0]) ]);
         else
             return baseSystem.storageCommit.apply(baseSystem, arguments);
     };

     /**
      @param {String} writeKey The field to write these data into.  Later, can
      use writeKey to read from stored data.
      @param {String} data serialized data to put in storage.
      @param {function} (optional) cb Callback to execute when write has completed. Takes a
      single argument: bool as to whether the write succeeded (true) or failed
      (false).
      */
     system.storageWrite = function()
     {
         if (arguments.length == 3 && typeof(arguments[2] === 'function'))
             return baseSystem.storageWrite.apply(baseSystem, [ arguments[0], arguments[1], system.wrapCallbackForSelf(arguments[2]) ]);
         else
             return baseSystem.storageWrite.apply(baseSystem, arguments);
     };

     /**
      @param {String} readKey (same as writeKey from storageCommit).  Specifies
      the key in the backend storage system to read data from.

      @param {function} cb Callback to execute when read finishes.  Takes two
      arguments: 1) bool (true if read succeeded, false if read failed); 2) If
      read succeeded, the value of the field read in (if read failed, undefined).

      */
     system.storageRead = function()
     {
         if (arguments.length == 2 && typeof(arguments[1] === 'function'))
             return baseSystem.storageRead.apply(baseSystem, [ arguments[0], system.wrapCallbackForSelf(arguments[1]) ]);
         else
             return baseSystem.storageRead.apply(baseSystem, arguments);
     };


     

     /**
      @param {String} eraseKey (same as writeKey from storageCommit and readKey
      from storageRead).  Specifies the element in the backend storage system to
      remove.

      @param {function} cb Callback to execute when remove finishes.  Takes a
      single argument: bool (true if remov is successful, false otherwise).
      */
     system.storageErase = function()
     {
         if (arguments.length == 2 && typeof(arguments[1] === 'function'))
             return baseSystem.storageErase.apply(baseSystem, [ arguments[0], system.wrapCallbackForSelf(arguments[1]) ]);
         else
             return baseSystem.storageErase.apply(baseSystem, arguments);
     };


     /**
      @param {String} readKeyStart. Specifies start of the range of keys in 
      the backend storage system to read data from.
      @param {String} readKeyFinish. Specifies end of the range of keys in 
      the backend storage system to read data from.

      @param {function} cb Callback to execute when read finishes.  Takes two
      arguments: 1) bool (true if read succeeded, false if read failed); 2) If
      read succeeded, the value of the field read in (if read failed, undefined).

      */
     system.storageRangeRead = function()
     {
         if (arguments.length == 3 && typeof(arguments[2] === 'function'))
             return baseSystem.storageRangeRead.apply(baseSystem, [ arguments[0], arguments[1], system.wrapCallbackForSelf(arguments[2]) ]);
         else
             return baseSystem.storageRangeRead.apply(baseSystem, arguments);
     };


     /**
      @param {String} eraseKeyStart. Specifies start of the range of keys in 
      the backend storage system to remove.
      @param {String} eraseKeyFinish. Specifies end of the range of keys in 
      the backend storage system to remove.

      @param {function} cb Callback to execute when remove finishes.  Takes a
      single argument: bool (true if remov is successful, false otherwise).

      */
     system.storageRangeErase = function()
     {
         if (arguments.length == 3 && typeof(arguments[2] === 'function'))
             return baseSystem.storageRangeErase.apply(baseSystem, [ arguments[0], arguments[1], system.wrapCallbackForSelf(arguments[2]) ]);
         else
             return baseSystem.storageRangeErase.apply(baseSystem, arguments);
     };


     /**
      @param {String} countKeyStart. Specifies start of the range of keys in 
      the backend storage system to count.
      @param {String} countKeyFinish. Specifies end of the range of keys in 
      the backend storage system to count.

      @param {function} cb Callback to execute when count finishes.  Takes two
      arguments: 1) bool (true if read succeeded, false if read failed); 2) If
      count succeeded, the int32_t value of the field read in (if count failed, undefined).

      */
     system.storageCount = function()
     {
         if (arguments.length == 3 && typeof(arguments[2] === 'function'))
             return baseSystem.storageCount.apply(baseSystem, [ arguments[0], arguments[1], system.wrapCallbackForSelf(arguments[2]) ]);
         else
             return baseSystem.storageCount.apply(baseSystem, arguments);
     };

     
     /**
      @ignore
      */
     system.__setSandboxMessageManager = function(sbox_message_manager)
     {
         sboxMessageManager = sbox_message_manager;
         var sboxMessageCallback = function(msg,sender)
         {
             sboxMessageManager.handleMessage(msg,sender);
         };
         baseSystem.setSandboxMessageCallback(sboxMessageCallback);
     };

     /**
      @ignore
      */
     system.__setPresenceMessageManager = function(pres_message_manager)
     {
         presMessageManager = pres_message_manager;
         var presMessageCallback = function(msg,sender,receiver)
         {
             presMessageManager.handleMessage(msg,sender,receiver);
         };
         baseSystem.setPresenceMessageCallback(presMessageCallback);
     };

          
      /** @ignore
       Call to register a message handler for a message to a presence.
       */
      system.registerHandler = function (callback,pattern,sender)
      {
          if (presMessageManager === null)
              throw new Error('Error: presence message manager is not yet initialized.  Cannot register a handler yet.');

          var wrappedCallback = this.__wrapRegHandler(callback);
          return presMessageManager.registerHandler(wrappedCallback,pattern,sender);
      };


     /**@ignore 
      */
     system.registerSandboxMessageHandler = function (callback,pattern,sender)
     {
          if (sboxMessageManager === null)
              throw new Error('Error: sandbox message manager is not yet initialized.  Cannot register a handler yet.');
          
          return sboxMessageManager.registerHandler(callback,pattern,sender);
      };
     
     
     
     
     /**
      @ignore
      */
     system.__setPresenceMessageCallback = function()
     {
         return baseSystem.setPresenceMessageCallback.apply(baseSystem,arguments);
     };
     
     
     //restore manipulations
     /** Set the script to execute when this object is restored after a crash.
      *  @param{String,Function} script the script to execute when the object is restored
      *  @param{Function} cb callback to invoke when the update finishes, taking
      *                   one boolean parameter indicating success
      */
     system.setRestoreScript = function(script, cb)
     {
         if (typeof(script) == 'function')
             script = '(' + script.toString() + ')()';
         if (typeof(script) !== 'string')
             throw new TypeError('Invalid restore script type: expected String or Function.');

         if (typeof(cb) !== 'undefined' && typeof(cb) !== 'function')
             throw new TypeError('Invalid callback parameter: expected function.');

         if (typeof(cb) === 'function')
             return baseSystem.setRestoreScript.apply(baseSystem, [script, system.wrapCallbackForSelf(cb)]);                 
         else
             return baseSystem.setRestoreScript.apply(baseSystem, [script]);

     };
     /** Disable restoration from storage after a crash.
      *  @param{Function} cb callback to invoke when the update finishes, taking
      *                   one boolean parameter indicating success
      */
     system.disableRestoreScript = function(cb)
     {
         if (typeof(cb) !== 'undefined' && typeof(cb) !== 'function')
             throw new TypeError('Invalid callback parameter: expected function.');

         if (typeof(cb) === 'function')
             return baseSystem.setRestoreScript.apply(baseSystem, ['', system.wrapCallbackForSelf(cb)]);
         else
             return baseSystem.setRestoreScript.apply(baseSystem, ['']);
     };


      //data
      system._selfMap = { };

      system.__behindSelf = undefined;

     
      system.__setBehindSelf = function(toSetTo)
      {
          this.__behindSelf = toSetTo;
      };

      system.__defineGetter__("self", function(){
                                return system.__behindSelf;
                            });

      system.__defineSetter__("self", function(val){
                            });



     
     /** @function
      @throws __killEntity__  (If kill entity command is successful)
      @throws Exception (If do not execute kill entity from root context).
      @description Destroys the entity and all the presences and state
      associated with it if run from the root context.  If not run
      from root context, throws error.
      */
     system.killEntity = function()
     {
         isKilling = true;
         baseSystem.killEntity.apply(baseSystem,arguments);
         throw '__killEntity__';
     };

     system.__isKilling = function()
     {
         return isKilling;
     };
     
     
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

      /** @function
      @description Prints the argument, followed by a newline
      @see system.prettyPrint
      */
      system.println = function(/** Object */ obj) {
          system.print.apply(this, Array.prototype.slice.call(arguments, "\n"));
      };
      
      system.__debugPrint = function()
      {
          baseSystem.print.apply(baseSystem,arguments);
      };


     /**
      @return Returns true if you are running in headless mode.  Returns false otherwise.
      */
     system.isHeadless = function()
     {
         return baseSystem.headless();
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



     /**
      Used internally by the compiler.  The compiler calls this check
      before actually callling eval.
      */
     system.__checkAndThrowCanEval = function()
     {
         if (!system.canEval())
             throw new Error ('ERROR: You do not have the capability to eval.');
     };

      // Not exposing this
      /** @ignore */
      system.__wrapRegHandler = function (toCallback)
      {
          var returner = function (msg,sender,receiver)
          {
              system.__setBehindSelf(system._selfMap[receiver].presObj);
              std.messaging.seqNumManager.updateSeqNumberOnReceivedMessage(system.self,sender,msg);
              // We may invoke this multiple times because we might
              // have multiple handlers. To ensure the reply code can
              // properly throw an exception if we try to reply twice,
              // keep an existing makeReply if we have it.
              if (msg.makeReply === undefined) msg.makeReply = std.messaging.makeReply(msg,sender);
              
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


     /** Trigger an event handler. The handler will be added to the event queue
      *  and invoked later, i.e. on a different stack than the current one.
      * 
      *  @param {function} handler the event handler to invoke.
      *  @returns {boolean} true on success, false if the system is shutting down
      *  @throws {TypeError} if anything besides a function is provided
      */
     system.event = function(handler) {
         if (typeof(handler) !== 'function')
             throw new TypeError('system.event only supports functions');

         return baseSystem.event.apply(baseSystem, [ system.wrapCallbackForSelf(handler) ]);
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
          var wrappedFunction = system.wrapCallbackForSelf(callback);
          if (typeof(isCleared) == 'undefined')
              return baseSystem.timeout(period,wrappedFunction);

          return baseSystem.timeout(period,wrappedFunction,contextId,timeRemaining,isSuspended,isCleared);
      };

     
     /** @ignore*/
     system.__debugFileWrite = function(strToWrite,filename)
     {
         baseSystem.__debugFileWrite(strToWrite,filename);
     };

     /** @ignore */
     system.__debugFileRead = function(filename)
     {
         return baseSystem.__debugFileRead(filename);
     };

     

      /** @function

       @param Which presence to send from.
       @param Message object to send.
       @param Visible to send to.
       */
      system.sendMessage = function()
      {
          baseSystem.sendMessage.apply(baseSystem, arguments);
      };


     /**
      @function
      @param Which presence to send from.
      @param Message object to send.
      @param Visible to send to.
      */
     system.sendMessageUnreliable = function()
     {
         baseSystem.sendMessageUnreliable.apply(baseSystem,arguments);
     };
     


     
      /** @function
       *  @description Loads a file and evaluates its contents. Note
       *  that this version *always* imports the file, even if it was
       *  previously imported.
       *  @param scriptFile The Emerson file to import and execute in the current script
       *  @throws {Exception} If the import leads to circular
       *  dependencies, it will throw an exception. Use system.require
       *  to include each file only once.
       *  @see system.require
       */
      system.import = function(/** String */ scriptFile)
      {
          baseSystem.import(scriptFile);
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
          return this.__createSandbox.apply(this,arguments);//baseSystem.create_context.apply(baseSystem, arguments);
      };

     
      /** @function
       @description Creates a new sandbox with associated capabilities
       @param {presence} If given capabilities, sends/receives
       messages, sets prox queries, etc. for this presence.
       @param {visible} or null The sporef for the presence on whose
       behalf we are creating the sandbox
       
       @param {int} a permission number indicating the level of
       permissions granted to the new sandbox.  NOTE: permission
       numbers instead of being composed of bit-operators are
       constructed from multiplying primes and interpretted from
       modding them.
       
       @return {sandbox} A sandbox object.  Can call execute on this
       sandbox to execute isolated code inside a sandbox.
       */
      system.__createSandbox = function()
      {
          return baseSystem.create_context.apply(baseSystem, arguments);
      };
     
      //not exposing
      /** @ignore */
      system.__wrapPresConnCB = function(callback)
      {
          var returner = function(presConn, /**only for entity-wide onPresenceConnected call*/clearable)
          {
              this.addToSelfMapAndPresencesArray(presConn);
              this.__setBehindSelf(presConn);
              if (typeof(callback) === 'function')
                  callback(presConn,clearable);
          };
          return std.core.bind(returner,this);
      };


          /** @function
           @description Creates a new entity based on the position and space of presence passed in.
           
           @param presence or visible.
           @param pos.  Position of new entity's first presence relative to first argument's position
           @param script Can either be a string, which will be eval-ed on new entity as soon as it's created or can be a non-closure capturing function that will be executed with next parameter as its argument.
           @param arg Object  Passed as argument to script argument if script argument is a function.
           @param solidAngle Solid angle that entity's new presence queries with.
           @param {optional} Mesh uri corresponding to mesh you want to use for this entity.  If undefined, defaults to self's mesh.
           @param {optional} scale Scale of new mesh. (Higher number means increase mesh's size.)  If undefined, default to self's scale.
           */
          system.createEntityFromPres = function(pres, pos, script,arg, solidAngle,mesh,scale)
          {
              var newSpace = pres.getSpaceID();
              var newPos = pres.getPosition() + pos;
              this.createEntityScript(newPos,script,arg,solidAngle,mesh,scale,newSpace);
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
           @param Script Can either be a string, which will be eval-ed on new entity as soon as it's created or can be a non-closure capturing function that will be executed with next parameter as its argument.
           @param Object Passed as argument to script argument if script argument is a function.  Null if takes no argument.
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
                  var toThrow = 20;
                  var tmp = 'throw ' + toThrow.toString() + ';' + str;
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
                  return false;  //should never get here;
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
                  var funcString = script.toString();
                  
                  if (arg !== null)
                  {
                      var serializedArg = Escape.escapeString(this.serialize(arg), '@');
                      funcString = '(' + funcString + ") ( system.deserialize(" + serializedArg + "));" ;
                  }
                  else
                      funcString = '(' + funcString + ") ( );";
                  
                  this.__hidden_createEntity(position, 'js',funcString,mesh,scale,solidAngle,space);
              }
              else if (typeof(script) == 'string')
              {
                  var emSynError = emersonSyntaxError(script);
                  if (emSynError !== false)
                  {
                      throw new Error("Error calling createEntity.  String passed in must be valid Emerson.  " + emSynError);
                  }

                  this.__hidden_createEntity(position,'js',script,mesh,scale,solidAngle,space);
              }
              else
              {
                  throw new Error("Error.  Second argument to createEntity must contain a string or a function to execute on new entity.");
              }
              
          };


     /**
      Tries to send a message, msg, from the sandbox that system is
      instantiated in to sbox.

      @param {object} msg The message to deliver to sbox.
      
      @param {sandbox|util.sandbox.PARENT} (optional) sbox Should either be
      a sandbox object or should be the parent sandbox identifier.
      Marks the intended destination of msg.  If undefined, just sends
      to parent.

      */
     system.sendSandbox = function(msg,sbox)
     {
         if (typeof(sbox) == 'undefined')
             return baseSystem.sendSandbox(msg,sandbox.Parent);
             
         return baseSystem.sendSandbox(msg,sbox);
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
         @description Creates a new presence for the entity. There are
         two ways to invoke this method. You can pass it an object
         containing properties to set (e.g. with fields 'position',
         'orientation', etc) or pass it a fixed set of arguments
         (mesh, callback, position, space). Only the first method
         allows setting all properties of the presence and is
         preferred. In both cases, the second argument can be a
         callback to be invoked when the presence is connected.

         By default, the presence's intial position is the same as the
         presence that created it and its scale is 1.

         @throws {Exception} if sandbox does not have capability to
         create presences.

         @see system.canCreatePresence

         
         @param {string or object} firstArg If string, will try to decode
         string as a mesh a uri for the new presence and callback
         argument is required.  If it's an object, we read the fields
         of the object for presence initialization information.  The
         fields of the object we read are as follows: space (string),
         pos(vec3), orient (quat), mesh (string), physics (string),
         scale (float), callback (function), solidAngleQuery(float).
         Any of these parameters that are undefined default to taking
         the equivalent value of system.self.

         @param {function} [callback] function to be called when
         presence gets connected to the world. (Function has form func
         (pres), where pres contains the presence just connected.)
         @param {optional} A position for the new presence.
         Unspecified defaults to same position as self.
         @param {optional} A space to create the new presence in.
         Unspecified defaults to same space as self.

         @param {Vec3} [position] requested initial position for the
         presence. Ignored when using an object to specify settings.

         @param {string} [space] the space to connect to.

         @return Nothing.
         */
        system.createPresence = function (firstArg, callback, position, space)
        {
            var sporef  = util.identifier(system.self.getSpaceID());
            var pos = system.self.getPosition();
            var connectedCallback = this.__wrapPresConnCB(function(){  });
            var mesh = this.self.getMesh();
            
            if ((typeof(space) != 'undefined') && (space !== null))
                sporef = util.identifier(space);
            if ((typeof(position) != 'undefined') && (position !== null))
                pos = position;

            if ((typeof(callback) != 'undefined'))
                connectedCallback = this.__wrapPresConnCB(callback);

            if (typeof(firstArg) == 'string')
                mesh = firstArg;

            var orient = system.self.getOrientation();
            var vel = new util.Vec3(0,0,0);
            var posTime = null;
            var orientVel = new util.Quaternion (0,0,0,1); //identity.
            var orientTime = null;
            var physics = '';
            var scale = 1.0;
            var isCleared = false;
            var contextID = null;
            var isConnected = true;
            

            var isSuspended = false;
            var suspendedVelocity = new util.Vec3(0,0,0);
            var suspendedOrientationVelocity = new util.Quaternion(0,0,0,1);
            var query = "";

            
            if (typeof(firstArg) == 'object')
            {
                if ('space' in firstArg)
                    sporef = util.identifier(firstArg['space']);

                if ('pos' in firstArg)
                    pos = firstArg['pos'];

                if ('position' in firstArg)
                    pos = firstArg['position'];

                if ('vel' in firstArg)
                    vel = firstArg['vel'];

                if ('velocity' in firstArg)
                    vel = firstArg['velocity'];

                if ('orientVel' in firstArg)
                    orientVel = firstArg['orientVel'];

                if ('orientationVel' in firstArg)
                    orientVel = firstArg['orientationVel'];

                if ('orient' in firstArg)
                    orient = firstArg['orient'];

                if ('orientation' in firstArg)
                    orient = firstArg['orientation'];

                if ('mesh' in firstArg)
                    mesh = firstArg['mesh'];

                if ('physics' in firstArg)
                    physics = firstArg['physics'];

                if ('scale' in firstArg)
                    scale = firstArg['scale'];

                if ('callback' in firstArg)
                    connectedCallback = this.__wrapPresConnCB(firstArg['callback']);

                if ('solidAngleQuery' in firstArg)
                    query = system.__presence_constructor__.__encodeDeprecatedQuery(firstArg['solidAngleQuery']);

                if ('query' in firstArg)
                    query = firstArg['query'];
            }
            return system.restorePresence(sporef,pos,vel,posTime,orient,orientVel,orientTime,mesh,physics,scale,isCleared,contextID,isConnected,connectedCallback,isSuspended,suspendedVelocity,suspendedOrientationVelocity,query);

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
      @param {string} physics,
      @param {number} scale,
      @param {boolean} isCleared ,
      @param {uint32} contextId,
      @param {boolean} isConnected,
      @param {function,null} connectedCallback,
      @param {boolean} isSuspended,
      @param {vec3} suspendedVelocity,
      @param {quaternion} suspendedOrientationVelocity,
      @param {string} query
      */
     system.restorePresence = function(sporef,pos,vel,posTime,orient,orientVel,orientTime,mesh,physics,scale,isCleared,contextId,isConnected,connCB,isSuspended,suspendedVelocity,suspendedOrientationVelocity,query)
     {
         if (connCB != null)
             connCB = this.__wrapPresConnCB(connCB);

         return baseSystem.restorePresence(sporef,pos,vel,posTime,orient,orientVel,orientTime,mesh,physics,scale,isCleared,contextId,isConnected,connCB,isSuspended,suspendedVelocity,suspendedOrientationVelocity,query);
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
       @return TRUE if eval() is invokable in the script
       */
      system.canEval = function()
      {
          return baseSystem.canEval.apply(baseSystem, arguments);
      };


      /** @function
       @description Gives the version of Emerson run by the entity host
       @type String
       @return the version of the Emerson being run by the entity host*/
      system.getVersion = function()
      {
          return baseSystem.getVersion.apply(baseSystem, arguments);
      };


     var presConnectedManager ={  };
     var lastConnectionManagerId = 0;
     function ClearablePresConnected(idToClear)
     {
         this.idToClear = idToClear;
     }
     ClearablePresConnected.prototype.clear=function()
     {
         if (this.idToClear in presConnectedManager)
             delete presConnectedManager[this.idToClear];
     };

     var onPresConnFunc = function(pres)
     {
         
         for (var s in presConnectedManager)
             presConnectedManager[s](pres, new ClearablePresConnected(s));                 
     };

     baseSystem.onPresenceConnected(onPresConnFunc);
     
      /** @function
       @description Registers a callback to be invoked when a presence created within this sandbox gets connected to the world
       @return does not return anything
       @param callback The function to be invoked. Function takes a single argument that corresponds to the presence that just connected to the world.
       */
      system.onPresenceConnected = function(/**Function */callback)
      {
          ++lastConnectionManagerId;
          var cpc = new ClearablePresConnected(lastConnectionManagerId);
          presConnectedManager[lastConnectionManagerId] = this.__wrapPresConnCB(callback);
          return cpc;
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


     /** @function
      @param {presence} presToGetSetFor The presence object whose prox result set you want this function to return.  If the presence object does not
      @return {object} Each index of object is the identifier for a separate visible object.  The value of that index is the visible itself.
      */
     system.getProxSet = function (presToGetSetFor)
     {
         var key = presToGetSetFor.toString();
         if (!(key in system._selfMap))
             throw new Error('Error in system.getProxSet.  Do not have a presence with the identifier specified.');

         return system._selfMap[key].getProxResultSet();
     };


     
     /**
      @param visObj is a visible object that has now moved into presence's result set
      @param presVisTo is a string that is the identifier for the presence the visible is visible to.

      Resets self as well
      */
     var proxAddedManager = function(visObj, presVisTo)
     {
         if (presVisTo.toString() in system._selfMap)
         {
             //reset self;
             system.__setBehindSelf( system._selfMap[presVisTo.toString()].presObj);
             //fire proxAddedEvent.
             system._selfMap[presVisTo.toString()].proxAddedEvent(visObj,system.self);
         }
         else
             throw new Error('Error: received prox added message for presence not controlling');
     };

     /**
      @param visObj is a visible object that has now moved into presence's result set
      @param presVisTo is a string that is the identifier for the presence the visible is visible to.

      Resets self as well
      */
     var proxRemovedManager = function(visObj, presVisTo)
     {
         if (presVisTo.toString() in system._selfMap)
         {
             //reset self;
             system.__setBehindSelf( system._selfMap[presVisTo.toString()].presObj);
             //fire proxRemovedEvent.
             system._selfMap[presVisTo.toString()].proxRemovedEvent(visObj,system.self);
         }
         else
             throw new Error('Error: received prox added message for presence not controlling');
     };

     baseSystem.registerProxAddedHandler(proxAddedManager);
     baseSystem.registerProxRemovedHandler(proxRemovedManager);

     /**
      @presCalling this is the presence that want to register
      onProxAdded function for
      @funcToCall this is the function to call when a new presence
      joins presCalling's result set.
      @onExisting {bool} (Optional) If true, runs the prox added callback
      funcToCall over all presences already existing in result set.
      */
      system.__sys_register_onProxAdded= function (presCalling, funcToCall,onExisting)
      {
          if (typeof(onExisting) == 'undefined')
              onExisting = false;
          
          if (presCalling.toString()  in this._selfMap)
          {
              var prevSelf = system.self;
              var returner = this._selfMap[presCalling.toString()].setProxAddCB(funcToCall);                  
              if (onExisting)
              {
                  system.__setBehindSelf( system._selfMap[presCalling.toString()].presObj);
                  var existing = system.getProxSet(presCalling);
                  for (var s in existing)
                      funcToCall(existing[s]);
                  
                  system.__setBehindSelf( prevSelf );
              }
              
              return returner;
          }
          else
              throw new Error('Error: do not have a presence in map matching ' + presCalling.toString());
      };
     
     /**
      @presCalling this is the presence that want to register onProxRemoved function fro
      @funcToCall this is the function to call when a new presence exits presCalling's result set.
      */
      system.__sys_register_onProxRemoved = function (presCalling, funcToCall)
      {
          if (presCalling.toString()  in this._selfMap)
              return this._selfMap[presCalling.toString()].setProxRemCB(funcToCall);
          else
              throw new Error('Error: do not have a presence in map matching ' + presCalling.toString());
      };

	 /**
	  @presCalling this is the presence that wants to unregister onProxAdded function
	  @addID this is the ID number of the onProxAdded function to delete
	  */
	  system.__sys_register_delProxAdded = function (presCalling, addID)
	  {
		  this._selfMap[presCalling.toString()].delProxAddCB(addID);
	  };

	 /**
	  @presCalling this is the presence that wants to unregister onProxRemoved function
	  @remID this is the ID number of the onProxRemoved function to delete
	  */
	  system.__sys_register_delProxRemoved = function (presCalling, remID)
	  {
		  this._selfMap[presCalling.toString()].delProxRemCB(remID);
	  };

      /** @function
       *  @description Loads and evaluates a file if it has not
       *  already been loaded.  Unlike system.import, this ensures
       *  that each file is imported at most one time. This is usually
       *  what you want to use.
       *  @param filename The path to look for the file to include
       *  @see system.import
       */
      system.require = function(/** String */ filename)
      {
          baseSystem.require(filename);
      };

     
     system.__entityToken = function()
     {
         return entityToken;
     };

     
      /** @function
       @description Destroys all created objects, except presences in the root context. Then executes script associated with root context. (Use system.setScript to set this script.)
       @see system.setScript
       @return Does not return any value
       */
      system.reset = function()
      {
          //Indices of presProxSetArg are presence's sporefs.  Values are
          //arrays containing the sporefs of all visibles within these
          //presences' prox sets.
          var presProxSetArg = { };
          for (var s in this._selfMap)
          {
              if (s == this.__NULL_TOKEN__)
                  continue;
              
              var pSet = this._selfMap[s].getProxResultSet();
              var pSetArray = [];
              for (var t in pSet)
                  pSetArray.push(t);

              presProxSetArg[s] = pSetArray;
          }
          
          try
          {
              baseSystem.reset(presProxSetArg);
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
      /** Array */  system.presences = [];
     
      system.__presence_constructor__ = __system.__presence_constructor__;
      system.__visible_constructor__ = __system.__visible_constructor__;


      // Invoking these force the callbacks to be registered, making
      // system.self work in all cases, even though the callbacks we
      // pass are undefined.
      system.onPresenceConnected(undefined);
      system.onPresenceDisconnected(undefined);

     //populates self with basic system object.


     //if this sandbox is associated with a given presence, then loads that
     //presence by default into self.
     if (typeof(baseSystem.getAssociatedPresence()) !== 'undefined')
     {
             system.addToSelfMapAndPresencesArray(baseSystem.getAssociatedPresence());
             system.__setBehindSelf(baseSystem.getAssociatedPresence());             
     }
     else
         system.addToSelfMapAndPresencesArray(null);


     // FIXME this shouldn't be in system, but its the only place we
     // can put functions with callbacks right now because we need to
     // get at the context they are running in
     system.__loadPresenceMesh = function(pres, cb) {
         pres.__origLoadMesh.apply(pres, [baseSystem, cb]);
     };
     system.__loadVisibleMesh = function(vis, cb) {
         vis.__origLoadMesh.apply(vis, [baseSystem, cb]);
     };
     
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
