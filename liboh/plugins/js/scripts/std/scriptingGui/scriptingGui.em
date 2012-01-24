std.ScriptingGui = {};
system.require('scriptingGuiUtil.em');
system.require('action.em');
system.require('console.em');
system.require('fileManagerElement.em');

(function()
{
    var TIME_TO_WAIT_FOR_RESPONSE = 20;
    
    //actIDs to std.ScriptingGui.Actions
    var actionMap = {};


    //map from visible id to visible object
    //which visibles are near me
    var nearbyVisMap   = {};
    //map from visible id to fileManagerElement
    //which visibles I have scripted before.
    var scriptedVisMap = {};

    var scripter    = null;
    var console     = new std.ScriptingGui.Console();


    //basic constructor
    std.ScriptingGui.Controller = function(presScripter)
    {
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

        var vis = getVisAndChangeToUsed(visId,this);
        
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
        console.scriptSentEvent(consDescription);
    };


    
    /**
     If we've alread scripted this visible before (visId key exists in
     nearbyVisMap), then returns the visible object associated with
     it.

     If we have not already scripted this object before, then check if
     object is in nearbyVisMap.  If it is, then we create a new
     filemanagerelement object associated with visible in
     scriptedVisMap, and return visible associated with it.

     Otherwise, returns null.
     */
    function getVisAndChangeToUsed(visId,scriptingGui)
    {
        var returner = null;
        if (visId in scriptedVisMap)
            returner = scriptedVisMap[visId].vis;
        else if (visId in nearbyVisMap)
        {
            returner = nearbyVisMap[visId];
            scriptingGui.addVisible(returner);
        }

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
        console.scriptRespEvent(resp);
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
        console.scriptRespEvent(resp);
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

            var consMsg = {
                    type: 'NEARBY_ADDED',
                    addedVis: newVis
                };
            console.nearbyEvent(consMsg);
        }
        function removeFromNearby(oldVis)
        {
            delete nearbyVisMap[oldVis.toString()];

            var consMsg = {
                    type: 'NEARBY_REMOVED',
                    removedVis: oldVis
                };
            console.nearbyEvent(consMsg);
        }
    }


    //************** FILES    ***********************//

    //returns true if visId is a key in scriptedVisMap (ie, have
    //already scripted a visible with id visId)
    function scriptedVisExists(visId)
    {
        return (visId in scriptedVisMap);
    }
          
     
    std.ScriptingGui.Controller.prototype.addVisible =
        function(visToAdd)
    {
        if (scriptedVisExists(visToAdd))
        {
            throw new Error ('Error in FileManager.addVisible.  ' +
                             'Already had record for this visible.');            
        }

        scriptedVisMap[visToAdd.toString()] =
            new std.FileManager.FileManagerElement(
                visToAdd,defaultErrorFunction,
                generateScriptingDirectory(visToAdd));

        var consMsg = {
                type: 'ADD_VISIBLE',
                visible: visToAdd
            };

        console.scriptedObjEvent(consMsg);
    };

    std.ScriptingGui.Controller.prototype.removeVisible =
        function(visToRemove)
    {
        if (!scriptedVisExists(visToRemove))
        {
            throw new Error ('Error in FileManager.removeVisible.  ' +
                             'Do not have record for this visible.');
        }
        system.__debugPrint('\nNote: not removing previously-written files.\n');
        delete scriptedVisMap[visToRemove.toString()];

        var consMsg = {
                type: 'REMOVE_VISIBLE',
                visible: visToRemove
            };
        console.scriptedObjEvent(consMsg);
    };


    std.ScriptingGui.Controller.prototype.removeFile =
        function(vis,filename)
    {
        if (!scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.removeFile.  ' +
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].removeFile(filename);
    };

    /**
     @param vis
     @param {String} filename (optional).  If undefined, will reread
     all files associated with this visible from disk.
     */
    std.ScriptingGui.Controller.prototype.rereadFile = function(vis,filename)
    {
        if (!scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.reareadFile.  '+
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].rereadFile(filename);
    };
     
     
    //if text is undefined, means just use the version on disk.
    std.ScriptingGui.Controller.prototype.addFile = function(vis,filename,text)
    {
        if (!scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.addFile.  ' +
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].addFile(filename,text);
    };
     
     

    std.ScriptingGui.Controller.prototype.checkFileExists = function(vis,filename)
    {
        if (!scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.checkFileExists.  ' +
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].checkFileExists(filename);
    };


    std.ScriptingGui.Controller.prototype.updateAll = function(vis)
    {
        if (!scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.updateAll.  ' +
                             'Do not have record for this visible.');
        }

        var updateFileId = std.ScriptingGui.Util.uniqueId();
        var consMsg = scriptedVisMap[vis.toString()].updateAll(
            updateFileId,
            std.core.bind(onFileUpdateSuccess,undefined,this,updateFileId),
            std.core.bind(onFileUpdateFailure,undefined,this,updateFileId));
        
        console.fileEvent(consMsg);
    };

    std.ScriptingGui.Controller.prototype.updateFile = function(vis,filename)
    {
        if (!scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.updateFilename.  ' +
                             'Do not have record for this visible.');
        }

        var updateFileId = std.ScriptingGui.Util.uniqueId();
        var consMsg = scriptedVisMap[vis.toString()].updateFile(
            filename,updateFileId,
            std.core.bind(onFileUpdateSuccess,undefined,this,updateFileId),
            std.core.bind(onFileUpdateFailure,undefined,this,updateFileId));

        console.fileEvent(consMsg);
    };

    function onFileUpdateSuccess(scriptingGui,updateId,filename)
    {
        var consMsg = {
            type: 'FILE_UPDATE_SUCCESS',
            filename: filename,
            updateId: updateId
        };
        console.fileEvent(consMsg);
    }

    function onFileUpdateFailure(scriptingGui,updateId,filename)
    {
        var consMsg = {
            type: 'FILE_UPDATE_FAIL',
            filename: filename,
            updateId: updateId
        };
        console.fileEvent(consMsg);        
    }
    
    
    std.ScriptingGui.Controller.prototype.getFileText = function(vis,filename)
    {
        if (!scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.getFileText.  ' +
                             'Do not have record for this visible.');
        }
        
        return scriptedVisMap[vis.toString()].getFileText(filename);
    };

    std.ScriptingGui.Controller.prototype.getRemoteVersionText = function(vis,filename)
    {
        if (!scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.getRemoteVersionText.  ' +
                             'Do not have record for this visible.');
        }
         
        return scriptedVisMap[vis.toString()].getRemoteVersionText(filename);
    };
     
    
    function defaultErrorFunction(errorString)
    {
        system.__debugPrint('\n');
        system.__debugPrint(errorString);
        system.__debugPrint('\n');
    }

    /**
     Returns the directory that all scripts associated with remove
     visible, visible, will be stored.
     */
    function generateScriptingDirectory(visible)
    {
        return 'ishmael_scripting/' + visible.toString();
    }

    
    //************** CONSOLES ***********************//

    
})();

