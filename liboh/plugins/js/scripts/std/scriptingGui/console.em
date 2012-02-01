

system.require('scriptingGuiUtil.em');
system.require('std/core/pretty.em');

(function ()
{
    //@param {std.ScriptingGui} scriptGui
    var sGui = null;

    /**
     @param{Object: <String(visId):std.FileManager.FileManagerElement>
     visMap.  Mostly going to deal with the consoleHistory value of
     each element in here.
     */
    std.ScriptingGui.Console = function(visMap)
    {
        this.visMap = visMap;
    };

    /**
     @param {std.ScriptingGui} scriptGui
     */
    std.ScriptingGui.Console.prototype.setScriptingGui =
        function(scriptGui)
    {
        sGui = scriptGui;
    };

    std.ScriptingGui.Console.prototype.checkVisExists =
        function (visId)
    {
        return (visId in this.visMap);
    };
    
    std.ScriptingGui.Console.prototype.scriptSentEvent =
        function(sseObj)
    {
        if (!this.checkVisExists(sseObj.visId))
        {
            throw new Error('Error processing script sent event.  ' +
                            'No matching visible with id: ' + sseObj.visId);                
        }

        //message
        var msg =  sseObj.type + ':  ' + sseObj.name + '\n';
        msg += sseObj.text;
        this.visMap[sseObj.visId].consoleHistory.push(msg);

        if (sGui !== null)
            sGui.redraw();
    };

    std.ScriptingGui.Console.prototype.scriptRespEvent =
        function(sreObj)
    {
        if (!this.checkVisExists(sreObj.visId))
        {
            throw new Error('Error processing script resp event.  ' +
                            'No matching visible with id: ' + sreObj.visId);                
        }

                
        var msg = '';
        msg += sreObj.type + ':  ' + sreObj.name + '';
        var someResp = false;
        if (sreObj.type =='ACTION_RESPONSE')
        {
            if (typeof(sreObj.value) != 'undefined')
            {
                someResp = true;
                msg += std.core.pretty(sreObj.value);                    
            }
            if (typeof(sreObj.exception) != 'undefined')
            {
                someResp = true;
                msg += '\nException: ' + std.core.pretty(sreObj.exception);                    
            }

        }
        else if (sreObj.type == 'ACTION_NO_RESPONSE')
        {
            //do nothing
        }
        else
        {
            throw new Error('Unrecognized script');
        }

        if (!someResp)
            return;

        
        this.visMap[sreObj.visId].consoleHistory.push(msg);
        if (sGui !== null)
            sGui.redraw();
    };

    std.ScriptingGui.Console.prototype.fileEvent =
        function(fObj)
    {
        if (!this.checkVisExists(fObj.visId))
        {
            throw new Error('Error processing script file event.  ' +
                            'No matching visible with id: ' +
                            fObj.visId + 'for message of type ' + fObj.type);                
        }

        var msg = '';
        if ((fObj.type == 'FILE_UPDATE_SUCCESS') ||
            (fObj.type == 'FILE_UPDATE_FAIL'))
        {
            msg = fObj.type + ':  ' + fObj.filename;
        }
        else if (fObj.type =='FILE_MANAGER_UPDATE_ALL')
        {
            msg = fObj.type + ':  ';
            for (var s in fObj.files)
                msg += fObj.files[s].filename + ', ';
        }
        else if (fObj.type =='FILE_MANAGER_UPDATE')
            msg = fObj.type + ':  ' + fObj.filename ;
        else
        {
            throw new Error('Error processing file event ' + fObj.type);                
        }

        
        this.visMap[fObj.visId].consoleHistory.push(msg);
        if (sGui !== null)
            sGui.redraw();
    };


    std.ScriptingGui.Console.prototype.printEvent =
        function(printObj)
    {
        if (!this.checkVisExists(printObj.visId))
        {
            throw new Error('Error processing print event.  ' +
                            'No matching visible with id: ' + printObj.visId);
        }
        this.visMap[printObj.visId].consoleHistory.push(
            std.core.pretty(printObj.value));
    };
    
    std.ScriptingGui.Console.prototype.nearbyEvent =
        function(nearbyObj)
    {
        //std.ScriptingGui.Util.dPrint('\nGot a nearbyEvent object\n');
    };

    std.ScriptingGui.Console.prototype.scriptedObjEvent =
        function(nearbyObj)
    {
        //std.ScriptingGui.Util.dPrint('\nGot a scriptedObjEvent object\n');
    };


    std.ScriptingGui.Console.prototype.guiEvent =
        function(guiObj)
    {
        //std.ScriptingGui.Util.dPrint('\nGot a gui event object\n');
    };
    
    std.ScriptingGui.Console.prototype.toHtmlMap =
        function()
    {
        var returner = {};
        for (var s in this.visMap)
            returner[s] = this.visMap[s].consoleHistory;
        
        return returner;
    };

    
})();