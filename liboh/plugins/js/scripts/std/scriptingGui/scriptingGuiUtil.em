

(function()
{
    var uID =0;
    
    std.ScriptingGui.Util =
        {
            uniqueId: function()
            {
                return ++uID; 
            },
            dPrint: function(toPrint)
            {
                system.__debugPrint(toPrint);
            }
        };
})();
