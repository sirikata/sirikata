if (typeof(std) === "undefined") /** @namespace */ std = {};

/* Map data structure
 * Constructor:
 *   e.g. X = new std.persistentMap('mapTest', function(success,size){...}); 
 *
 * APIs:
 *   set(key,value)     : Write element to memory
 *   copy(map)          : Write all elements in given associative array to this map (in memory)
 *                          e.g. M = {'a':'x','b':'y'};  X.copy(M); 
 *   get(key)           : Read element with given key from memory, return the value
 *   get(key,cb)        : Read element with given key from memory first, if not exists, read
 *                        from backend storage then. Callback: cb(success, value)   
 *   erase(key)         : Erase element from memory
 *   clear()            : Clear content in memory
 *   flush(cb)          : Flush changes to backend storage. Callback: cb(success)
 *   retrieve(keys,cb)  : Retrieve elements with given list of keys from backend to memory
 *                        Callback: cb(success)
 *   restore(cb)        : Restore all elements from backend to memory. Callback: cb(success)
 *   reset(cb)          : Clear content in both memory and backend. Callback: cb(success)
 *   name()             : Return name of the map
 *   data()             : Return all elements in memory, as an associative array
 *   size(cb)           : Get number of elements in backend. Callback: cb(success, count)
 *   contains(key,cb)   : Check if map contains the key. Callback: cb(success)
 *   isResident(key)    : Return true if the key is resident in memory, false otherwise
 */

std.persistentMap = function(name,cb) 
{
    this._type = 'map';
    this._name = name;
    this._mapName = keyName(this._type,this._name);
    this._data = {};
    this._dirtyKeys = {};
    this.size(cb);
};

std.persistentMap.prototype.set = function(key, value)
{
    this._data[key]=value;
    this._dirtyKeys[key] = 1;
};

std.persistentMap.prototype.copy = function(map)
{
    for(key in map)
        this.set(key, map[key]);
}

std.persistentMap.prototype.get = function()
{
    if (arguments.length == 1) {
        key = arguments[0];
        return this._data[key];
    }
    else if (arguments.length == 2) {
        key = arguments[0];
        cb = arguments[1];
        if(key in this._data)
            cb(true, this._data[key]);
        else
            system.storageRead(keyName(this._mapName, key), std.core.bind(this._getCommit, this, key, cb));
    }
    else {
        system.print('API *get* usage: get(key) or get(key, cb)');
        return false;
    }
};

std.persistentMap.prototype._getCommit = function(key, cb, success, val)
{
    if(!success)
        system.print('key does not exist');
    else 
        this._data[key]=val[keyName(this._mapName, key)];
    cb(success, this._data[key]);
};

std.persistentMap.prototype.erase = function(key)
{
    delete this._data[key];
    this._dirtyKeys[key] = 0;
};

std.persistentMap.prototype.clear = function()
{
    for (key in this._data)
        this.erase(key);
};

std.persistentMap.prototype.flush = function(cb)
{
    system.storageBeginTransaction();
    for (key in this._dirtyKeys) {
        if(this._dirtyKeys[key]==1)
            system.storageWrite(keyName(this._mapName, key), this._data[key]);
        if(this._dirtyKeys[key]==0)
            system.storageErase(keyName(this._mapName, key));
    }
    system.storageCommit(std.core.bind(this._flushCommit, this, cb));
};

std.persistentMap.prototype._flushCommit = function(cb, success)
{
    if (!success)
        system.print('Flush fails');
    else
        this._dirtyKeys={};
 
    cb(success);
};

std.persistentMap.prototype.retrieve = function(keys, cb)
{
    system.storageBeginTransaction();
    for(i=0; i<keys.length; i++)
        system.storageRead(keyName(this._mapName, keys[i]));
    system.storageCommit(std.core.bind(this._retrieveCommit, this, keys, cb));
};

std.persistentMap.prototype._retrieveCommit = function(keys, cb, success, val)
{
    if(!success)
        system.print('Retrieve fails');
    else
        for (i=0; i<keys.length; i++)
            this._data[keys[i]] = val[keyName(this._mapName, keys[i])];
    cb(success);
};

std.persistentMap.prototype.restore = function(cb)
{
    start = this._mapName;
    finish = this._mapName+'@';
    system.storageRangeRead(start, finish,  std.core.bind(this._restoreCommit, this, cb));
};

std.persistentMap.prototype._restoreCommit = function(cb, success, val)
{
    if (!success)
        system.print('Restore fails');
    else {
        for (key in val)
            this._data[key.split(':')[2]]=val[key];
        this._dirtyKeys = {};
    }
    cb(success);
};

std.persistentMap.prototype.reset = function(cb)
{
    start = this._mapName;
    finish = this._mapName+'@';
    system.storageRangeErase(start, finish,  std.core.bind(this._resetCommit, this, cb));
};

std.persistentMap.prototype._resetCommit = function(cb, success) 
{
    if (!success)
        system.print('reset fails');
    else {
        this._data = {};
        this._dirtyKeys = {};
        system.print('map reset');
    }
    cb(success);
};

std.persistentMap.prototype.contains = function(key, cb)
{
    if(key in this._data)
        cb(true);
    else
        system.storageRead(keyName(this._mapName, key), function(success){cb(success);});

};

std.persistentMap.prototype.isResident = function(key)
{
    if (key in this._data)
        return true;
    else
        return false;
};

std.persistentMap.prototype.type = function()
{
    return this._type;
};

std.persistentMap.prototype.name = function()
{
    return this._name;
};

std.persistentMap.prototype.data = function()
{
    return this._data;
};

std.persistentMap.prototype.dirtyKeys = function()
{
    return KeystoList(this._dirtyKeys);
};

std.persistentMap.prototype.size = function(cb)
{
    start = this._mapName;
    finish = this._mapName+'@';
    system.storageCount(start, finish,  function(success, count){cb(success,count);});
};

var keyName = function(name, subname) {
    return name + ':' + subname;
};

var KeystoList = function(keys) {
    list = [];
    for (key in keys)
        list.push(key);
    return list;
};
