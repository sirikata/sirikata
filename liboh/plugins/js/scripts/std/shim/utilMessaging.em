
/**
 @namespace
 @param name to match
 @param value to match (can be null)
 @param proto to match (can be null)

 @return pattern object
 */
util.Pattern = function(name,value, prototype)
{
    if (typeof(name) == 'undefined')
        name = null;
    if (typeof(value) == 'undefined')
        value = null;
    if (typeof(prototype) == 'undefined')
        prototype = null;

    this.name  = name;
    this.value = value;
    this.prototype = prototype;
};

util.Pattern.type = 'pattern';


util.Pattern.prototype.matchesPattern = function(msgObj)
{
    var returner = true;
    if (this.name != null)
    {
        returner = returner && (this.name in msgObj);
        
        if (this.value != null)
            returner = returner && (msgObj[this.name] === this.value);

        //FIXME: need to actually do something with prototype here.
    }
    
    return returner;
};


util.Pattern.prototype.__getType = function()
{
    return util.Pattern.type;
};
    

/**
 @namespace
 */
util.Handler = function(callback)
{
    this.isSusp = false;
    this.callback    = callback;
};

util.Handler.type = 'handler';

util.Handler.prototype.suspend = function ()
{
    this.isSusp = true;
};

util.Handler.prototype.resume = function()
{
    this.isSusp = false;
};

util.Handler.prototype.isSuspended = function()
{
    return this.isSusp;
};

util.Handler.prototype.dispatch = function(msgObj,msgSender)
{
    if (this.isSuspended())
        return;
    
    this.callback.apply(undefined, arguments);
};

util.Handler.prototype.getAllData = function()
{
    var returner = {'isSuspended' : this.isSuspended(),
                    'callback': this.callback
                   };
    return returner;
};

util.Handler.prototype.__getType = function ()
{
    return util.Handler.type;
};




/**
 @namespace
 */
util.HandlerPatternSenderTriple = function (handler,pattern,sender,messageManager)
{
    this.pattern = [];
    
    //check first argument is a handler
    if ((typeof(handler) != 'object') ||
        (handler == null) ||
        (typeof(handler.__getType) != 'function') ||
        handler.__getType() !== util.Handler.type)
    {
        throw new TypeError('Invalid use of HandlerPatternSenderTriple, require Handler object as first argument'); 
    }

    //check second argument is a pattern, an array of patterns, or null: can have null pattern; implies match everything.
    if (pattern != null)
    {
        if (typeof(pattern) != 'object')
             throw new TypeError('Invalid use of HandlerPatternSender, require Pattern object, null, or array of Pattern objects as second argument');         

        //check if array of patterns
        if (typeof (pattern.length) == 'number')
        {
            for (var s in pattern)
            {
                if  ((typeof(pattern[s].__getType) != 'function') ||
                    (pattern[s].__getType() !== util.Pattern.type))
                {
                    throw new TypeError('Invalid use of HandlerPatternSender, require Pattern object, null, or array of Pattern objects as second argument');                           
                }

                this.pattern.push(pattern[s]);
            }
        }
        else
        {
            //check if just got a single pattern
            if  ((typeof(pattern.__getType) != 'function') ||
                (pattern.__getType() !== util.Pattern.type))
            {
                throw new TypeError('Invalid use of HandlerPatternSender, require Pattern object, null, or array of Pattern objects as second argument');                           
            }

            this.pattern.push(pattern);
        }
    }

    //check third argument is a visible.
    if (typeof(sender) != 'object') //note that sender can be null, meaning "match any sender".
    {
        //FIXME: need to do more thorough type-checking for visible
        //objects.
        throw new TypeError('Invalid use of HandlerPatternSenderTriple, require Visible object as third argument'); 
    }

    this.handler = handler;
    //note: pattern array has already been taken care of.
    this.sender  = sender;
    this.messageManager = messageManager;
};

util.HandlerPatternSenderTriple.type = 'hpst';

util.HandlerPatternSenderTriple.prototype.suspend = function()
{
    return this.handler.suspend();
};

util.HandlerPatternSenderTriple.prototype.deRegister = function()
{
    return this.messageManager.deRegister(this);
};


util.HandlerPatternSenderTriple.prototype.clear = function()
{
    system.__debugPrint  ('\nWARN: Clear is not defined in util.HandlerPatternSenderTriple yet\n');
};


/**
 Note: if this object has already been registered with message
 manager, has no effect.
 */
util.HandlerPatternSenderTriple.prototype.reRegister = function()
{
    return this.messageManager.register(this);
};


/**
 @return Returns true if msg matches the patterns stored in this's
 pattern array.
 */
util.HandlerPatternSenderTriple.prototype.matchesPattern = function(msg)
{
    if (this.pattern == null)
        return true;
    
    var returner = true;
    for (var s in this.pattern)
        returner = returner && this.pattern[s].matchesPattern(msg);

    return returner;
};


util.HandlerPatternSenderTriple.prototype.tryDispatch = function(msg,sender,receiver)
{
    if ((! this.handler.isSuspended()) && (this.matchesPattern(msg)))
        this.handler.dispatch(msg,sender,receiver);
};

