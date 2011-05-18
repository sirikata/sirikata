
{
  /** @namespace visible */
  var handler = function()
  {


      /**
       Calling suspend prevents the handler from firing again until it is resumed.
       */
      handler.prototype.suspend = function()
      {
          
      };

      /**
       @return Object with the following fields: {boolean} isCleared , {uint32} contextId, {visible, optional} sender, {function, optional} callback, {array or null, optional} patterns, {boolean, optional} isSuspended.
       */
      handler.prototype.getAllData = function()
      {
      };


      
      /**
       Calling this function on a handler makes it so that the handler's associated
       callback can now fire in response to received messages.
       */
      handler.prototype.resume = function()
      {
          
      };

      /**
       @return Boolean corresponding to whether the handler is currently suspended.
       */
      handler.prototype.isSuspended = function()
      {
          
      };

      /**
       Calling clear prevents the handler from ever re-firing again.
       */
      handler.prototype.clear = function()
      {
          
      };


  };
}