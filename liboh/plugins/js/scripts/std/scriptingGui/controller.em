system.require('scriptingGuiUtil.em');

(function()
{
    var TIME_TO_WAIT_FOR_RESPONSE = 20;
    
    //actIDs to std.ScriptingGui.Actions
    var actionMap = {};


    var guiScriptActionId = std.ScriptingGui.Util.uniqueId();
    var scriptAction =
        new std.ScriptingGui.Action('script action','',guiScriptActionId);
    
    //map from visible id to visible object
    //which visibles are near me
    var nearbyVisMap   = {};
    //map from visible id to fileManagerElement
    //which visibles I have scripted before.
    var scriptedVisMap = {};

    var scripter    = null;
    var console     = null;
    var gui         = null;
    

    //basic constructor
    std.ScriptingGui.Controller = function(simulator,presScripter)
    {
        console = new std.ScriptingGui.Console(scriptedVisMap);

        scripter = presScripter;
        if (typeof(presScripter) === 'undefined')
            scripter = system.self;

        
        gui = new std.ScriptingGui(
            this,simulator,nearbyVisMap,
            scriptedVisMap,actionMap,console);
        
        instantiateProxHandlers();
        this.addVisible(scripter);
    };


    //************* ACTIONS ***********************//
    function existsInActMap(id)
    {
        return (id in actionMap);
    }
    

    std.ScriptingGui.Controller.prototype.getScriptActionId =
        function()
    {
        return guiScriptActionId;
    };

    std.ScriptingGui.Controller.prototype.execScriptAction =
        function(visId,text)
    {

        if (! this.scriptedVisExists(visId))
        {
            throw new Error('Error executing script action.  ' +
                            'Have no record of visId: ' + visId.toString());
        }

        scriptAction.edit(text);
        var vis = scriptedVisMap[visId].vis;
        return internalExecAction(this,scriptAction,vis);
    };

    
    std.ScriptingGui.Controller.prototype.addAction =
        function(name,text)
    {
        var actId =  std.ScriptingGui.Util.uniqueId();
        actionMap[actId] =
            new std.ScriptingGui.Action(name,text,actId);

        gui.redraw();
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
        internalExecAction(this,action,vis);
    };

    
    function internalExecAction(controller,action,vis)
    {
        var visId = vis.toString();

        scripter # action.createScriptMsg() >> vis >>
            [std.core.bind(onActResp,undefined,controller,action),
             TIME_TO_WAIT_FOR_RESPONSE,
             std.core.bind(onNoActResp,undefined,controller,action,visId)];

        var consDescription = action.getConsoleDescription(visId);
        console.scriptSentEvent(consDescription);
    }
    

    
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
    function getVisAndChangeToUsed(visId,controller)
    {
        var returner = null;
        if (visId in scriptedVisMap)
            returner = scriptedVisMap[visId].vis;
        else if (visId in nearbyVisMap)
        {
            returner = nearbyVisMap[visId];
            controller.addVisible(returner);
        }

        return returner;
    }
    

    /**
     @param {object} msg -- response sent from other side.

     @param {visible} sender -- visible that we tried to run action on.
     
     @param {std.ScriptingGui.Controller} controller -- the scripting
     gui (can use it to access console object).  (FIXME: not really
     needed anymore.)

     @param {std.ScriptingGui.Action} action -- action that was
     requested of other side.

     */
    function onActResp(controller,action,msg,sender)
    {
        var resp=
            action.getConsoleResponseDescription(msg,sender.toString());
        //tell the console about the change
        console.scriptRespEvent(resp);
    }

    /**
     @param {std.ScriptingGui.Controller} controller -- the scripting
     gui (can use it to access the console object).

     @param {std.ScriptingGui.Action} action -- the action object that
     we tried to execute on the visible with id visId.

     @param {string} visId -- the id of the visible we tried to
     execute the action on.
     */
    function onNoActResp(controller,action,visId)
    {
        var resp =
            action.getConsoleNoResponseDescription(visId);

        //tell the console about the change
        console.scriptRespEvent(resp);
    }
    
    
    //************** VISIBLES ***********************//

    function instantiateProxHandlers()
    {
        system.self.onProxAdded(addToNearby,true);
        system.self.onProxRemoved(removeFromNearby);
        function addToNearby(newVis)
        {            
            nearbyVisMap[newVis.toString()] = newVis;

            //trying to remove as much overhead as possible associated
            //with proximity handlers + this console messages do not
            //actually do anything.
            
            // var consMsg = {
            //         type: 'NEARBY_ADDED',
            //         addedVis: newVis
            //     };
            // console.nearbyEvent(consMsg);
        }
        function removeFromNearby(oldVis)
        {
            delete nearbyVisMap[oldVis.toString()];

            //trying to remove as much overhead as possible associated
            //with proximity handlers + this console messages do not
            //actually do anything.
            
            // var consMsg = {
            //         type: 'NEARBY_REMOVED',
            //         removedVis: oldVis
            //     };
            // console.nearbyEvent(consMsg);
        }
    }


    //************** FILES    ***********************//

    //returns true if visId is a key in scriptedVisMap (ie, have
    //already scripted a visible with id visId)
    std.ScriptingGui.Controller.prototype.scriptedVisExists =
        function(visId)
    {
        return (visId in scriptedVisMap);
    };
          
     
    std.ScriptingGui.Controller.prototype.addVisible =
        function(visToAdd)
    {
        if (this.scriptedVisExists(visToAdd))
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
        gui.redraw();
    };

    std.ScriptingGui.Controller.prototype.removeVisible =
        function(visToRemove)
    {
        if (!this.scriptedVisExists(visToRemove))
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
        if (!this.scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.removeFile.  ' +
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].removeFile(filename);
    };

    /**
     @param vis (can be string or visible)
     @param {String} filename (optional).  If undefined, will reread
     all files associated with this visible from disk.
     */
    std.ScriptingGui.Controller.prototype.rereadFile = function(vis,filename)
    {
        if (!this.scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.reareadFile.  '+
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].rereadFile(filename);
    };


    std.ScriptingGui.Controller.prototype.rereadAllFiles =
        function(visId)
    {
        if (!this.scriptedVisExists(visId))
        {
            throw new Error ('Error in FileManager.reareadAllFiles.  '+
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].rereadAllFiles();
    };
    

    /**
     @returns {object: <string (visibleId):
              object: <string(filename):string(filename)>>}
     
     keyed by visible id, elements are maps of files that each remote
     visible has on it.
     
     @see ishmaelRedraw in scriptingGui.em
     */
    std.ScriptingGui.Controller.prototype.htmlFileMap =function()
    {
        var returner = {};

        for (var s in scriptedVisMap)
            returner[s] = scriptedVisMap[s].getAllFilenames();
        return returner;
    };
    
    //if text is undefined, means just use the version on disk.
    std.ScriptingGui.Controller.prototype.addFile = function(vis,filename,text)
    {
        if (!this.scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.addFile.  ' +
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].addFile(filename,text);
    };


    //checks to see if file exists.  if does, then use the text from
    //that file.  if does not, then creates it with empty string of text.
    std.ScriptingGui.Controller.prototype.addExistingFileIfCan =
        function(vis,filename)
    {
        if (!this.scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.addExistingFileIfCan.  ' +
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].addExistingFileIfCan(filename);
    };
     

    std.ScriptingGui.Controller.prototype.checkFileExists = function(vis,filename)
    {
        if (!this.scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.checkFileExists.  ' +
                             'Do not have record for this visible.');
        }

        return scriptedVisMap[vis.toString()].checkFileExists(filename);
    };


    std.ScriptingGui.Controller.prototype.updateAll = function(vis)
    {
        if (!this.scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.updateAll.  ' +
                             'Do not have record for this visible.');
        }

        var updateFileId = std.ScriptingGui.Util.uniqueId();
        var consMsg = scriptedVisMap[vis.toString()].updateAll(
            updateFileId,
            std.core.bind(onFileUpdateSuccess,undefined,this,updateFileId,vis.toString()),
            std.core.bind(onFileUpdateFailure,undefined,this,updateFileId,vis.toString()));
        
        console.fileEvent(consMsg);
    };

    std.ScriptingGui.Controller.prototype.updateFile = function(vis,filename)
    {
        if (!this.scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.updateFilename.  ' +
                             'Do not have record for this visible.');
        }

        var updateFileId = std.ScriptingGui.Util.uniqueId();
        var consMsg = scriptedVisMap[vis.toString()].updateFile(
            filename,updateFileId,
            std.core.bind(onFileUpdateSuccess,undefined,this,updateFileId,vis.toString()),
            std.core.bind(onFileUpdateFailure,undefined,this,updateFileId,vis.toString()));

        console.fileEvent(consMsg);
    };

    function onFileUpdateSuccess(controller,updateId,visId,filename)
    {
        var consMsg = {
            type: 'FILE_UPDATE_SUCCESS',
            filename: filename,
            updateId: updateId,
            visId: visId
        };
        console.fileEvent(consMsg);
    }

    function onFileUpdateFailure(controller,updateId,visId,filename)
    {
        var consMsg = {
            type: 'FILE_UPDATE_FAIL',
            filename: filename,
            updateId: updateId,
            visId: visId
        };
        console.fileEvent(consMsg);        
    }
    
    
    std.ScriptingGui.Controller.prototype.getFileText = function(vis,filename)
    {
        if (!this.scriptedVisExists(vis))
        {
            throw new Error ('Error in FileManager.getFileText.  ' +
                             'Do not have record for this visible.');
        }
        
        return scriptedVisMap[vis.toString()].getFileText(filename);
    };

    std.ScriptingGui.Controller.prototype.getRemoteVersionText = function(vis,filename)
    {
        if (!this.scriptedVisExists(vis))
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


    //************ Handles print messages ***************//
    handlePrint << {'request':'print':};

    function handlePrint(msg,sender)
    {
        if (console === null)
            return;

        var printMsg = {
            type: 'PRINT_MESSAGE',
            visId: sender.toString(),
            value: msg.print
        };
        console.printEvent(printMsg);
        gui.redraw();
    }


    //************ External interface ***************//
    std.ScriptingGui.Controller.prototype.script = function (visOrPres)
    {
        var visibler = getVisAndChangeToUsed(visOrPres.toString(),this);
        if (visibler === null)
        {
            throw new Error('Error scripting: no record of visible with id ' +
                            visOrPres.toString() + ' to script.');
        }

        gui.redraw(visOrPres.toString(),true);
    };
    
})();

