

system.require('scriptingGuiUtil.em');

(function ()
{

    std.ScriptingGui.Console = function(scriptingGui)
    {
        this.scriptingGui = scriptingGui;
    };

    std.ScriptingGui.Console.prototype.scriptSentEvent =
        function(sseObj)
    {
        std.ScriptingGui.Util.dPrint('\nGot a script sent event\n');
    };

    std.ScriptingGui.Console.prototype.scriptRespEvent =
        function(sreObj)
    {
        std.ScriptingGui.Util.dPrint('\nGot a script recevied event\n');
    };

    std.ScriptingGui.Console.prototype.fileEvent =
        function(fObj)
    {
        std.ScriptingGui.Util.dPrint('\nGot a fileEvent object\n');
    };

    std.ScriptingGui.Console.prototype.nearbyEvent =
        function(nearbyObj)
    {
        std.ScriptingGui.Util.dPrint('\nGot a nearbyEvent object\n');
    };

    std.ScriptingGui.Console.prototype.scriptedObjEvent =
        function(nearbyObj)
    {
        std.ScriptingGui.Util.dPrint('\nGot a scriptedObjEvent object\n');
    };


    std.ScriptingGui.Console.prototype.guiEvent =
        function(guiObj)
    {
        std.ScriptingGui.Util.dPrint('\nGot a gui event object\n');
    };
    
    
})();