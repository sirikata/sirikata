

if (typeof (std) ===  'undefined')
    util = {  };


util.Capabilities = function()
{
    this.capPerms = 1;

    for (var s =0; s < arguments.length; ++s)
        this.capPerms +=arguments[s];            
};


/*Sister file with c++ decs is in JSCapabilitiesConsts.hpp */
util.Capabilities.SEND_MESSAGE     = 1;
util.Capabilities.RECEIVE_MESSAGE  = 2;
util.Capabilities.IMPORT           = 4;
util.Capabilities.CREATE_PRESENCE  = 8;
util.Capabilities.CREATE_ENTITY    = 16;
util.Capabilities.EVAL             = 32;
util.Capabilities.PROX_CALLBACKS   = 64;
util.Capabilities.PROX_QUERIES     = 128;
util.Capabilities.CREATE_SANDBOX   = 256;
util.Capabilities.GUI              = 512;
util.Capabilities.HTTP             = 1024;




util.Capabilities.prototype.__getType = function()
{
    return "capabilities";
};

util.Capabilities.prototype.createSandbox = function(presence,visible)
{
    var permProduct=this.capPerms;
    return system.__createSandbox(presence,
                                  visible,
                                  permProduct);
};

