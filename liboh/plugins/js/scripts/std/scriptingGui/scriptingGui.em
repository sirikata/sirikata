std.ScriptingGui = {};
system.require('scriptingGuiUtil.em');
system.require('action.em');
system.require('console.em');


(function()
{
    var TIME_TO_WAIT_FOR_RESPONSE = 20;
    
    //actIDs to std.ScriptingGui.Actions
    var actionMap = {};

    //Next to maps are indexed by visible ids and have values of
    //visible objects.
    //which visibles are near me
    var nearbyVisMap = {};
    //which visibles I have scripted before.
    var scriptedVisMap = {};

    var scripter = null;
    
    std.ScriptingGui.Controller = function(presScripter)
    {
        this.console = new std.ScriptingGui.Console();

        scripter = presScripter;
        if (typeof(presScripter) === 'undefined')
            scripter = system.self;
    };



    //************* ACTIONS ***********************//
    function existsInActMap(id)
    {
        return (id in actionMap);
    }
    
    
    std.ScriptingGui.Controller.prototype.addAction =
        function(name,text)
    {
        var actId =  std.ScriptingGui.Util.uniqueId();
        actionMap[actId] =
            new std.ScriptingGui.Action(name,text,actId);

        return actId;
    };

    std.ScriptingGui.Controller.prototype.removeAction =
        function(id)
    {
        if (!existsInActMap(id))
        {
            throw new Error('Cannot remove action, because ' +
                            'have no record of it.');         
        }

        delete actionMap[id];
    };
    
    std.ScriptingGui.Controller.prototype.editAction =
        function(id,newText)
    {
        if (!existsInActMap(id))
        {
            throw new Error('Cannot modify action, because ' +
                            'have no record of it.');                
        }

        actionMap[id].edit(newText);
    };

    std.ScriptingGui.Controller.prototype.execAction =
        function(actId,visId)
    {
        if (!existsInActMap(actId))
        {
            throw new Error('Cannot execute action, because ' +
                            'have no record of it.');                
        }

        var vis = getVisAndChangeToUsed(visId);
        
        if (vis === null)
        {
            throw new Error ('Cannot execute action on visible ' +
                             visId + ' have no record of.');
        }

        var action = actionMap[actId];
        scripter # action.createScriptMsg() >> vis >>
            [std.core.bind(onActResp,undefined,this,action),
             TIME_TO_WAIT_FOR_RESPONSE,
             std.core.bind(onNoActResp,undefined,this,action,visId)];

        var consDescription = actionMap[actId].getConsoleDescription(visId);
        this.console.scriptSentEvent(consDescription);
    };


    
    /**
     If visId exists in nearbyVisMap, then removes it and adds it to
     scriptedVisMap.  Returns the associated visible.  

     If first condition isn't met, but visId exists in scriptedVisMap,
     returns visible.

     Otherwise, returns null.
     */
    function getVisAndChangeToUsed(visId)
    {
        var returner = null;
        if (visId in nearbyVisMap)
        {
            returner = nearbyVisMap[visId];
            scriptedVisMap[visId] = returner;
        }
        else if (visId in scriptedVisMap)
            returner = scriptedVisMap[visId];

        return returner;
    }
    

    /**
     @param {object} msg -- response sent from other side.

     @param {visible} sender -- visible that we tried to run action on.
     
     @param {std.ScriptingGui} scriptGui -- the scripting gui (can use
     it to access console object).

     @param {std.ScriptingGui.Action} action -- action that was
     requested of other side.

     */
    function onActResp(scriptGui,action,msg,sender)
    {
        var resp=
            action.getConsoleResponseDescription(msg,sender.toString());
        //tell the console about the change
        scriptGui.console.scriptRespEvent(resp);
    }

    /**
     @param {std.ScriptingGui} scriptGui -- the scripting gui (can use
     it to access the console object).

     @param {std.ScriptingGui.Action} action -- the action object that
     we tried to execute on the visible with id visId.

     @param {string} visId -- the id of the visible we tried to
     execute the action on.
     */
    function onNoActResp(scriptGui,action,visId)
    {
        var resp =
            action.getConsoleNoResponseDescription(visId);

        //tell the console about the change
        scriptGui.console.scriptRespEvent(resp);
    }

    
    
    
    //************** VISIBLES ***********************//
    if (typeof(system.self) == 'undefined')
    {
        system.onPresenceConnected(
            function(newPres,clearable)
            {
                instantiateProxHandlers();
                clearable.clear();
            });
    }
    else
    {
        instantiateProxHandlers();
    }

    function instantiateProxHandlers()
    {
        system.self.onProxAdded(addToNearby,true);
        system.self.onProxRemoved(removeFromNearby);
        function addToNearby(newVis)
        {
            nearbyVisMap[newVis.toString()] = newVis;
        }
        function removeFromNearby(oldVis)
        {
            delete nearbyVisMap[oldVis.toString()];
        }
    }
    



    //************** FILES    ***********************//


    //************** CONSOLES ***********************//

    
})();

