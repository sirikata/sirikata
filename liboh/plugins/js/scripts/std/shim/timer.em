
{
  /** @namespace visible */
  var timer = function()
  {


      /**
       Calling suspend prevents the timer from firing again until it is resumed.
       */
      timer.prototype.suspend = function()
      {
          
      };

      /**
       @return Object with the following fields: {boolean} isCleared , {uint32} contextId, {double, optional} period, {function, optional} callback, {double, optional} timeRemaining, {boolean, optional} isSuspended.
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