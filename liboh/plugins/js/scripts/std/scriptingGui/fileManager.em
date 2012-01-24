


(function()
 {
     /**
      FileManager keeps a copy of the files that are located on
      foreign visibles.  Can be used to send new versions of files to
      foreign files.
      */
     std.FileManager = function()
     {
         //map from visID to std.FileManager.FileElement
         this.visIDToElements = {};
     };
     
     system.require('fileManagerElement.em');
     
     std.FileManager.prototype.visExists = function (visToCheck)
     {
         return (visToCheck.toString() in this.visIDToElements);
     };


     
     std.FileManager.prototype.addVisible = function(visToAdd)
     {
         if (this.visExists(visToAdd))
         {
             throw new Error ('Error in FileManager.addVisible.  ' +
                              'Already had record for this visible.');            
         }

         this.visIDToElements[visToAdd.toString()] =
             new std.FileManager.FileManagerElement(
                 visToAdd,defaultErrorFunction,
                 generateScriptingDirectory(visToAdd));
     };

     std.FileManager.prototype.removeVisible = function(visToRemove)
     {
         if (!this.visExists(visToRemove))
         {
             throw new Error ('Error in FileManager.removeVisible.  ' +
                              'Do not have record for this visible.');
         }
         
         system.__debugPrint('\nNote: not removing previously-written files.\n');
         delete this.visIDToElements[visToRemove.toString()];
     };


     std.FileManager.prototype.removeFile = function(vis,filename)
     {
         if (!this.visExists(vis))
         {
             throw new Error ('Error in FileManager.removeFile.  ' +
                              'Do not have record for this visible.');
         }

         return this.visIDToElements[vis.toString()].removeFile(filename);
     };

     /**
      @param vis
      @param {String} filename (optional).  If undefined, will reread
      all files associated with this visible from disk.
      */
     std.FileManager.prototype.rereadFile = function(vis,filename)
     {
         if (!this.visExists(vis))
         {
             throw new Error ('Error in FileManager.reareadFile.  '+
                              'Do not have record for this visible.');
         }

         return this.visIDToElements[vis.toString()].rereadFile(filename);
     };
     
     
     //if text is undefined, means just use the version on disk.
     std.FileManager.prototype.addFile = function(vis,filename,text)
     {
         if (!this.visExists(vis))
         {
             throw new Error ('Error in FileManager.addFile.  ' +
                              'Do not have record for this visible.');
         }

         return this.visIDToElements[vis.toString()].addFile(filename,text);
     };
     
     

     std.FileManager.prototype.checkFileExists = function(vis,filename)
     {
         if (!this.visExists(vis))
         {
             throw new Error ('Error in FileManager.checkFileExists.  ' +
                              'Do not have record for this visible.');
         }

         return this.visIDToElements[vis.toString()].checkFileExists(filename);
     };


     std.FileManager.prototype.updateAll = function(vis)
     {
         if (!this.visExists(vis))
         {
             throw new Error ('Error in FileManager.updateAll.  ' +
                              'Do not have record for this visible.');
         }

         return this.visIDToElements[vis.toString()].updateAll();
     };

     std.FileManager.prototype.updateFile = function(vis,filename)
     {
         if (!this.visExists(vis))
         {
             throw new Error ('Error in FileManager.updateFilename.  ' +
                              'Do not have record for this visible.');
         }

         return this.visIDToElements[vis.toString()].updateFilename(filename);
     };

     std.FileManager.prototype.getFileText = function(vis,filename)
     {
         if (!this.visExists(vis))
         {
             throw new Error ('Error in FileManager.getFileText.  ' +
                              'Do not have record for this visible.');
         }
         
         return this.visIDToElements[vis.toString()].getFileText(filename);
     };

     std.FileManager.prototype.getRemoteVersionText = function(vis,filename)
     {
         if (!this.visExists(vis))
         {
             throw new Error ('Error in FileManager.getRemoteVersionText.  ' +
                              'Do not have record for this visible.');
         }
         
         return this.visIDToElements[vis.toString()].getRemoteVersionText(filename);
     };
     
     

     
     function defaultErrorFunction(errorString)
     {
         system.__debugPrint('\n');
         system.__debugPrint(errorString);
         system.__debugPrint('\n');
     }

     /**
      Returns the directory that all scripts associated with remove
      visible, visible, will be stored.
      */
     function generateScriptingDirectory(visible)
     {
         return 'ishmael_scripting/' + visible.toString();
     }
     
 })();



