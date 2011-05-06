system.import('std/core/bind.em');

//if do not already have std and std.core objects
//defined, define them.
if (typeof(std) === "undefined") /** @namespace */ std = {};
if (typeof(std.core) === "undefined") /** @namespace */ std.core = {};


/** @function
  * Constructor for an object that executes callback function
  * periodically every period seconds.  Can suspend the object (stops
  * firing the periodically), or reset the object (restarts the
  * periodic callbacks).  Calling reset when the object hasn't
  * already been suspended, sets the time until callback fires
  * back to period.  That is, if period were 3 seconds, 1 second
  * had elapsed, and scripter calls reset on object, instead of
  * callback firing 2 seconds from now, it will fire at 3 seconds
  * from now.
  *
  * @param {Number} period A number indicating the period (in seconds) to go between firing callback.
  * @param {function()} callback A callback taking no args that should be fired every period seconds.
  *
 */
std.core.RepeatingTimer = function(period,callback)
{
    var callbackHelper = function()
    {
        this.userCallback();
        this.timer.reset();
    };
    
    this.period       = period;
    this.userCallback = callback;
    this.callback     = std.core.bind(callbackHelper,this);
    this.timer        = system.timeout(this.period,this.callback);
    this.suspend      = function ()
    {
        return this.timer.suspend();
    };
    this.reset        = function ()
    {
         return this.timer.reset();  
    };
    this.isSuspended  = function ()
    {
        return this.timer.isSuspended();
    };
};
