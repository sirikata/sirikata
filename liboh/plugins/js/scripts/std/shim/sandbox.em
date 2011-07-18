
{
  /** @class A sandbox provides a safe execution environment so you
   *  can run untrusted code, e.g. code shipped to you by another
   *  object in the world. A sandbox can be suspended or destroyed at
   *  any time and can be given restricted functionality, e.g. by
   *  disabling message sending or proximity queries to limit its
   *  effects in the world. Sandboxes aren't constructed
   *  directly. Instead, use system.createSandbox to allocate a new
   *  one.
   */
  var sandbox = function()
  {

      /**
       Used as a constant for sending messages to your parent sandbox.
       */
      sandbox.PARENT = null;
      
      /**
       Calling suspends sandbox. (Until resume is called, no code within sandbox will execute.)
       */
      sandbox.prototype.suspend = function()
      {
          
      };

      /**
       Resumes suspended sandbox.
       */
      sandbox.prototype.resume = function()
      {
          
      };


      /**
       Destroys all objects that were created in this context + all of this
       context's subcontexts.
       */
      sandbox.prototype.clear = function()
      {
          
      };


      /**
       @param function object to execute within sandbox
       */
      sandbox.prototype.execute = function()
      {
          
      };

  };
}