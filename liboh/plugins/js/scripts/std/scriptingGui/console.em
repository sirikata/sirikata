

system.require('scriptingGuiUtil.em');

(function ()
{

    std.ScriptingGui.Console = function()
    {
        
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
    
})();