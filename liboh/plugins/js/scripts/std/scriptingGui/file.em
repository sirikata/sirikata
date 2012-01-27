

(function()
{

    /**
     @param{string} text (optional) -- text to enter into file.

     @param{bool} useExistingFirst (optional) -- if set to true, then
     first try to read file from hd.  If that fails, then create a new
     file intead.
     */
    std.FileManager.File =
        function(name,text,filedir,useExistingFirst)
    {
        this.version = 0;
        this.name    = name;
        this.filedir = filedir;
        this.text    = text;
        this.remoteVersionText = '';

        if (useExistingFirst)
        {
            //if reading file throws an exception, it means that the
            //file does not exist. instead, try to create new file
            //through write operation in exception.
            try
            {
                this.readFile();
            }
            catch(excep)
            {
                this.writeFile();
            }
        }
        else
        {
            if (this.text == undefined)
                this.readFile();
            else
                this.writeFile();                
        }
    };


    std.FileManager.File.prototype.writeFile = function()
    {
        var toWriteTo = this.filedir + '/' + this.name;
        system.__debugFileWrite(this.text,toWriteTo);
    };

    std.FileManager.File.prototype.readFile = function()
    {
        var toReadFrom = this.filedir + '/' + this.name;
        this.text = system.__debugFileRead(toReadFrom);
    };

    std.FileManager.File.prototype.setRemoteVersionText = function(lastSentText)
    {
        this.remoteVersionText = lastSentText;
    };
    
    std.FileManager.File.prototype.updatetext = function (newText)
    {
        ++this.version;
        this.text = newText;
        this.writeFile();
    };

})();