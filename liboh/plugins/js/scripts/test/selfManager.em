system.require('std/shim/class.em');
system.require('std/core/bind.em');



std.sysExtender = system.Class.extend(
    {
        init: function(curSys)
        {
            //constants
            this.__NULL_TOKEN__ = 'null';


            //data
            this._curSys = curSys;
            this._selfMap = { };

            //initializers
            this.initializeSelf();
            this.addToSelfMap(this.self);
            
        },

        initializeSelf: function()
        {
            //lkjs;
            //FIXME: should not default to presences[0], because may not exist yet.
            this.__behindSelf = this._curSys.presences[0];
            this.__setBehindSelf = function(toSetTo)
            {
                this.__behindSelf = toSetTo;   
            };

            this.__defineGetter__("self", function(){
                                      return this.__behindSelf;
                                  });
    
            this.__defineSetter__("self", function(val){
                                  });            
        },
        
        addToSelfMap: function(toAdd)
        {
            var selfKey = (toAdd == null)? this.__NULL_TOKEN__: toAdd.toString();
            this._selfMap[selfKey] = toAdd;
        },

        basicPrint: function(toPrint)
        {
            this._curSys.print(toPrint);  
        },
        
        timeout: function (timeUntil, callback)
        {
            var selfKey = (this.self == null )? this.__NULL_TOKEN__ : this.self.toString();
            var wrappedFunction = this.__wrapTimeout(callback,selfKey);
            this._curSys.timeout(timeUntil,wrappedFunction);
        },
        
        __wrapTimeout: function(callback,toStringSelf)
        {
            var returner = function()
            {
                this.__setBehindSelf(this._selfMap[toStringSelf]);
                callback();
            };
            
            return std.core.bind(returner,this);
        },


        createPresence : function (uri, funcToCall)
        {
            this._curSys.create_presence(uri,this.__wrapPresConnCB(funcToCall));
        },

        __wrapPresConnCB : function(callback)
        {
            var returner = function(presConn)
            {
                this.addToSelfMap(presConn);
                this.__setBehindSelf(presConn);
                callback(presConn);
            };

            return std.core.bind(returner,this);
        },


        onPresenceConnected : function(funcToCall)
        {
            this._curSys.onPresenceConnected(this.__wrapPressConnCB(funcToCall));
        },


        
        onPresenceDisconnected : function (funcToCall)
        {
            this._curSys.onPresenceDisconnected(this.__wrapPressConnCB(funcToCall));
        },

        
        __onProxAdded : function (presCalling, funcToCall)
        {
            var wrappedCallback = this.__wrapOnProx(presCalling,funcToCall);
            presCalling.onProxAdded(wrappedCallback);
        },

        __onProxRemoved : function (presCalling, funcToCall)
        {
            var wrappedCallback = this.__wrapOnProx(presCalling,funcToCall);
            presCalling.onProxRemoved(wrappedCallback);
        },
        
        __wrapOnProx : function (presCalling,funcToCall)
        {
            var returner = function(newVis)
            {
                this.__setBehindSelf(presCalling);
                funcToCall(newVis);
            };

            return std.core.bind(returner,this);
        },

        //registerHandlers
        registerHandler : function (callback,arg2,arg3,arg4)
        {
            var wrappedCallback = this.__wrapRegHandler(callback);
            this._curSys.registerHandler(wrappedCallback,arg2,arg3,arg4);
        },

        __wrapRegHandler : function (toCallback)
        {
            var returner = function (msg,sender,receiver)
            {
                this.__setBehindSelf(receiver);
                toCallback(msg,sender);
            };
            return std.core.bind(returner,this);
        }
    }
    );


std.sys = new std.sysExtender(system);




function toPrint()
{
    system.print('\n\n');
    system.print(std.sys.self.toString());
    system.print('\n\n');
}

function toPrint2(pres)
{
    system.print('\n\nInside func');
    system.print(std.sys.self.toString());
    system.print(pres.toString());
    system.print('\n\n');
}


//std.sys.timeout(5,toPrint);


std.sys.createPresence(std.sys.self.getMesh(),toPrint2);

std.sys.registerHandler(test_msg_handler,null, [{"m":"random":}, {"msg":"msg":}]  ,null);

function test_msg_handler(msg,sender)
{
    system.print("\n");
    system.print("Got into msg handler");
    system.print(std.sys.self.toString());
    system.print("\n");
}

