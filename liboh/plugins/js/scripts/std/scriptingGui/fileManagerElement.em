



(function()
 {
     var FILE_UPDATE_TIMEOUT = 10;
     
     std.FileManager = { };     
     system.require('file.em');
     
     std.FileManager.FileManagerElement = function(vis,errorFunction,filedir)
     {
         this.vis = vis;
         //filename to std.FileManger.File
         this.fileMap = {};
         this.errorFunction = errorFunction;
         this.filedir = filedir;
     };

     std.FileManager.FileManagerElement.prototype.addFile = function(name,text)
     {
         if (this.checkFileExists(name))
         {
             throw new Error('Error adding file to filemanagerelement.  ' +
                             'Already have file with name ' + name);
         }

         this.fileMap[name] = new std.FileManager.File(name,text,this.filedir);
     };


     std.FileManager.FileManagerElement.prototype.removeFile = function(name)
     {
         if (!this.checkFileExists(name))
         {
             throw new Error ('Error removing file from filemanagerelement.' +
                              '  Do not have file with name ' + name);
         }

         delete this.fileMap[name];
     };

     std.FileManager.FileManagerElement.prototype.checkFileExists = function(name)
     {
         return (name in this.fileMap);
     };

     
     //Force visible to read all files sent.
     std.FileManager.FileManagerElement.prototype.updateAll = function()
     {
         for (var s in this.fileMap)
             this.updateFile(s);
     };

     std.FileManager.FileManagerElement.prototype.getFileText = function(filename)
     {
         if (!this.checkFileExists(filename))
         {
             throw new Error ('Error getting file text.  Filename ' + filename +
                              ' does not exist');
         }

         return this.fileMap[filename].text;
     };

     std.FileManager.FileManagerElement.prototype.getRemoteVersionText = function(filename)
     {
         if (!this.checkFileExists(filename))
         {
             throw new Error ('Error getting remote text.  Filename ' + filename +
                              ' does not exist');
         }

         return this.fileMap[filename].remoteVersionText;         
     };


     /**
      @param {String} filename (optional) -- If undefined, just
      rereads every file from disk that is associated with this
      visible.
      */
     std.FileManager.FileManagerElement.prototype.rereadFile = function(filename)
     {
         if (typeof(filename) == 'undefined')
         {
             for (var s in this.fileMap)
                 this.fileMap[s].readFile();
             return;    
         }

         
         if (!this.checkFileExists(filename))
         {
             throw new Error('Error rereading file from disk.  Have no ' +
                             'record of file with name ' +
                             filename.toString());
         }

         this.fileMap[filename].readFile();

     };
     
     //sends the file to visible.
     std.FileManager.FileManagerElement.prototype.updateFile = function(filename)
     {
         if(!(filename in this.fileMap))
         {
             throw new Error('Error updating file in FileManagerElement: no ' +
                             'file with name ' + filename);            
         }
         
         var file = this.fileMap[filename];
         
         var fileUpdateObject =
             {
                 'fileManagerUpdate':           true,
                 'filename':               file.name,
                 'version':             file.version,
                 'text':                   file.text
             };

         fileUpdateObject >> this.vis >> 
             [ function()
               {
                   file.setRemoteVersionText(file.text);
               },
               FILE_UPDATE_TIMEOUT,
               function()
               {
                   errorFunction('No response to update.');
               }
             ];
     };

 })();



