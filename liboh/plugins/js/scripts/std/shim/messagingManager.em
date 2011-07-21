(function()
{
    function MessageReceiveManager()
    {
        //Every member of this array should be a
        //HandlerPatternSenderTriple.  
        this.allMessageHandlers = [];
        //Tracks the empty indices in allMessageHandlers.  Whenver we put
        //a new message handler into allMessageHandlers, we first try to
        //put it into a spot with an empty index.
        this.emptyIndices = [];
    }


    /**@ignore
     
     system should callback to this function whenever receive a
     message.  The function runs through all its handlers to see if
     one of their patterns matches msg.
     
     */
    MessageReceiveManager.prototype.handleMessage= function(msg,sender,receiver)
    {
        for (var s in this.allMessageHandlers)
        {
            if (this.allMessageHandlers[s] != null)
                this.allMessageHandlers[s].tryDispatch(msg,sender,receiver);
        }
    };


    /**@ignore
     Puts newTriple into allMessageHandlers.
     */
    MessageReceiveManager.prototype.insertIntoHandlers = function(newTriple)
    {
        //check if can reuse an empty slot: if cannot (if condition), then
        //just push onto array; if can (else condition), then insert into
        //empty slot.
        if (this.emptyIndices.length == 0)    
            this.allMessageHandlers.push(newTriple);
        else
            this.allMessageHandlers[ this.emptyIndices.pop()] = newTriple;
    };


    /**
     Creates a HandlerPatternSenderTriple from the three arguments
     provided, and appends this to SandboxMessageManager's array of
     message handlers.

     Returns a HandlerPatternSenderTriple from which we can deregister the
     handler, suspend it, re-register it, etc.
     */
    MessageReceiveManager.prototype.registerHandler = function(handlerOrCallback, pattern,sender)
    {
        if (typeof(sender) == 'undefined')
            sender = null;

        if (typeof(handlerOrCallback) ==='function')
            handlerOrCallback = new util.Handler(handlerOrCallback);
        
        var newTriple = new util.HandlerPatternSenderTriple(handlerOrCallback,pattern,sender);
        
        this.insertIntoHandlers (newTriple);
        return newTriple;
    };


    /**
     Run through all registered message handlers, and remove from
     allMessageHanders array if strictly equal to toDeregister.
     */
    MessageReceiveManager.prototype.deregister = function (toDeregister)
    {
        for (var s in this.allMessageHandlers)
        {
            if (this.allMessageHandlers[s] === toDeregister)
            {
                this.allMessageHandlers[s] = null;
                this.emptyIndices.push(s);
                return;
            }
        }
    };
    

    /**
     @param {HandlerPatternSenderTriple} toRegister
     
     If don't already have toRegister entered in allMessageHandlers, then
     enter it.
     */
    MessageReceiveManager.prototype.register = function (toRegister)
    {
        if ((typeof(toRegister) != 'object') ||
            (toRegister === null) ||
            (typeof(toRegister.__getType) != 'function') ||
            toRegister.__getType() !== util.HandlerPatternSenderTriple.type)
        {
            throw new TypeError('Invalid use of register in message manager.  Require a HandlerPatternSenderTriple as argument');
        }

        //check that we don't already have toRegister registered
        for (var s in this.allMessageHandlers)
        {
            if (toRegister === this.allMessageHandlers[s])
                return;
        }

        this.insertIntoHandlers(toRegister);
    };

    var sboxMessageManager = new MessageReceiveManager();
    var presMessageManager = new MessageReceiveManager();
    
    //register manager with system to receive any presence messages.
    system.__setPresenceMessageManager(presMessageManager);

    //register manager with system to receive any sandbox messages.
    system.__setSandboxMessageManager(sboxMessageManager);
    
 })();