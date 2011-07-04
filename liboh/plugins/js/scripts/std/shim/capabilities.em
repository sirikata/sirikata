

if (typeof (std) ===  'undefined')
    util = {  };


util.Capabilities = function()
{
    this. capObj = 1;

    for (var s =0; s < arguments.length; ++s)
        this.capObj*=arguments[s];            
};

util.Capabilities.SEND_MESSAGE     = 2;
util.Capabilities.GUI              = 3;
util.Capabilities.PROX             = 5;
util.Capabilities.RECEIVE_MESSAGE  = 7;
util.Capabilities.CREATE_PRESENCE  = 11;
util.Capabilities.CREATE_ENTITY    = 13;
util.Capabilities.CREATE_SANDBOX   = 17;
util.Capabilities.HTTP             = 19;
util.Capabilities.IMPORT           = 23;
uill.Capabilities.EVAL             = 29;
