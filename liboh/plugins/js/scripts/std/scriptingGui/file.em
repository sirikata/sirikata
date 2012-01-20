

(function()
{
    std.FileManager.File = function(name,text,filedir)
    {
        this.version = 0;
        this.name    = name;
        this.filedir = filedir;
        this.text    = text;
        this.remoteVersionText = '';

        if (this.text == undefined)
            this.readFile();
        else
            this.writeFile();
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