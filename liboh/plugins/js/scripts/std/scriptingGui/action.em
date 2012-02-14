

std.ScriptingGui.Action = function(actionName, actionText,actionId)
{
    this.name = actionName;
    this.text = actionText;
    this.id   = actionId;
};


std.ScriptingGui.Action.prototype.edit = function(newText)
{
    this.text = newText;
};


/**
 Returns a scripting request message.  This scripting request message
 can be sent to targets, causing them to execute text associated with
 action on themselves.
 */
std.ScriptingGui.Action.prototype.createScriptMsg =
    function()
{
    var returner = {
        request: 'script',
        script: this.text
    };

    return returner;
};

/**
 When request an action to run, that action gets displayed in the
 console.  The object this returns is used by the console to display
 that the action has run.
 */
std.ScriptingGui.Action.prototype.getConsoleDescription =
    function(visId)
{
    return {
        type: 'ACTION_REQUEST',
        name: this.name,
        text: this.text,
        visId: visId
    };
};


std.ScriptingGui.Action.prototype.getConsoleResponseDescription =
    function(respMsg, responderId)
{
    return {
        type: 'ACTION_RESPONSE',
        name: this.name,
        visId: responderId,
        value: respMsg.value,
        exception: respMsg.exception
    };
};

std.ScriptingGui.Action.prototype.getConsoleNoResponseDescription =
    function(responderId)
{
    return{
        type: 'ACTION_NO_RESPONSE',
        visId:responderId,
        name: this.name
    };
};