
{
  /** @class A timer fires a callback after the specified number of
   * seconds. Create a new timer with system.timeout(duration, callback).
   *  @param {number} duration number of seconds to wait, may be fractional
   *  @param {function} callback callback to invoke after
   */
  var timer = function(duration, callback)
  {


      /**
       Calling suspend prevents the timer from firing again until it is resumed.
       */
      timer.prototype.suspend = function()
      {
          
      };

      /** Gets all data associated with this timer for serialization.
       *  @private
       *  @return Object with the following fields: {boolean} isCleared , {uint32} contextId, {double, optional} period, {function, optional} callback, {double, optional} timeRemaining, {boolean, optional} isSuspended.
       */
      timer.prototype.getAllData = function()
      {
      };
      
      /**
       Calling this function on a timer makes it so that the timer's associated
       callback will fire however many seconds from now the timer was initialized with.
       */
      timer.prototype.resume = function()
      {
          
      };

      /**
       @return Boolean corresponding to whether the timer is currently suspended.
       */
      timer.prototype.isSuspended = function()
      {
          
      };

      /**
       Calling clear prevents the timer from ever re-firing again.
       */
      timer.prototype.clear = function()
      {
          
      };



      /**
       @param numeric, specifying how long from now to go before re-firing the timer.
       @return boolean.

       This function takes in a single argument: when the callback should actually
       fire from now.
       */
      timer.prototype.resetTimer = function()
      {
          
      };

  };
}