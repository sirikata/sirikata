if (typeof(std) === "undefined") /** @namespace */ std = {};

/* Map data structure
 * Constructor:
 *   e.g. X = new std.map('mapTest'); 
 *
 * APIs:
 *   set(key,value)      : Write element to memory
 *   copy(map)           : Write all elements in a given associate array to this map (in memory)
 *                           e.g. M = {'a':'x','b':'y'};  X.copy(M); 
 *   get(key)            : Read element with given key from memory, return the value
 *   get(key,cb)         : Read element with given key from memory first, if not exists, read
 *                         from backend storage then. Callback: cb(success, value)   
 *   erase(key)          : Erase element from memory
 *   clear()             : Clear content in memory
 *   flush(cb)           : Flush changes to backend storage. Callback: cb(success)
 *   retrieve(keys,cb)   : Retrieve elements with given list of keys from backend to memory
 *                         Callback: cb(success)
 *   restore(cb)         : Restore all elements from backend to memory. Callback: cb(success)
 *   reset(cb)           : Clear content in both memory and backend. Callback: cb(success)
 *   name()              : Return name of the map
 *   data()              : Return all elements in memory, as an associate array
 *   keys()              : Return all keys in both memory and backend, as an array.
 *   size()              : Return number of elements in memory and backend
 *   empty()             : Return true if map is emty   
 */

std.map = function(name) 
{
    this._type = 'map';
    this._name = name;
    this._mapName = keyName(this._type,this._name);
    this._dirtyKeys = {};
    this._data = {};
    this._keys = {};
    this._keys_list = {};
    this._init(function(success){});
};

std.map.prototype._init = function(cb)
{
    system.storageRead(this._mapName, std.core.bind(this._initCommit, this, cb));
};

std.map.prototype._initCommit = function(cb, success, val)
{
    if (!success)
        system.print('This is a new map')
    else {
        this._keys = StringtoKeys(val[this._mapName]);
        this._keys_list = this._keys;
        system.print('Map exist, restore keys')
    }
    cb(success);
};

std.map.prototype.set = function(key,value)
{
    this._data[key]=value;
    this._dirtyKeys[key] = 1;
    this._keys[key] = 1;
};

std.map.prototype.copy = function(map)
{
    for(key in map)
        this.set(key, map[key]);
}

std.map.prototype.get = function()
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

std.map.prototype._getCommit = function(key, cb, success, val)
{
    if(!success)
        system.print('key does not exist');
    else 
        this._data[key]=val[keyName(this._mapName, key)];
    cb(success, this._data[key]);
};

std.map.prototype.erase = function(key)
{
    delete this._data[key];
    delete this._keys[key];
    if (key in this._keys_list)
        this._dirtyKeys[key] = 0;
    else
        delete this._dirtyKeys[key];
};

std.map.prototype.clear = function()
{
    for (key in this._data)
        this.erase(key);
};

std.map.prototype.flush = function(cb)
{
    system.storageBeginTransaction();
    for (key in this._dirtyKeys) {
        if(this._dirtyKeys[key]==1)
            system.storageWrite(keyName(this._mapName, key), this._data[key]);
        if(this._dirtyKeys[key]==0)
            system.storageErase(keyName(this._mapName, key));
    }
    system.storageWrite(this._mapName, KeystoString(this._keys));
    system.storageCommit(std.core.bind(this._flushCommit, this, cb));
};

std.map.prototype._flushCommit = function(cb, success)
{
    if (!success)
        system.print('Flush fails');
    else {
        this._dirtyKeys={};
        this._keys_list = this._keys;
    }
    cb(success);
};

std.map.prototype.retrieve = function(keys, cb)
{
    system.storageBeginTransaction();
    for(i=0; i<keys.length; i++)
        system.storageRead(keyName(this._mapName, keys[i]));
    system.storageCommit(std.core.bind(this._retrieveCommit, this, keys, cb));
};

std.map.prototype._retrieveCommit = function(keys, cb, success, val)
{
    if(!success)
        system.print('Retrieve fails');
    else
        for (i=0; i<keys.length; i++)
            this._data[keys[i]] = val[keyName(this._mapName, keys[i])];
    cb(success);
};

std.map.prototype.restore = function(cb)
{
    system.storageRead(this._mapName, std.core.bind(this._restore, this, cb));
};

std.map.prototype._restore = function(cb, success, val) 
{
    if (!success) {
        system.print('Map does not exist');
        cb(success);
    }
    else {
        this._keys = {};
        list = val[this._mapName].split(',');
        if(list.length>0) {
            system.storageBeginTransaction();
            for (i=0; i<list.length; i++) {
                system.storageRead(keyName(this._mapName, list[i]));
                this._keys[list[i]] = 1;
            }
            system.storageCommit(std.core.bind(this._restoreCommit, this, cb));
        }
        else {
            system.print('Map is empty');
            cb(success)
        }
    }
};

std.map.prototype._restoreCommit = function(cb, success, val)
{
    if (!success)
        system.print('Restore fails');
    else {
        for (key in val)
            this._data[key.split(':')[2]]=val[key];
        this._dirtyKeys = {};
        this._keys_list = this._keys;
    }
    cb(success);
};

std.map.prototype.reset = function(cb)
{
    system.storageBeginTransaction();
    system.storageErase(this._mapName);
    for(key in this._keys)
        system.storageErase(keyName(this._mapName, key));
    for(key in this._dirtyKeys)
        if(this._dirtyKeys[key]==0)
            system.storageErase(keyName(this._mapName, key));
    system.storageCommit(std.core.bind(this._resetCommit, this, cb));
};

std.map.prototype._resetCommit = function(cb, success) 
{
    if (!success)
        system.print('reset fails');
    else {
        this._data = {};
        this._keys = {};
        this._dirtyKeys = {};
        this._keys_list = {};
        system.print('map reset');
    }
    cb(success);
};

std.map.prototype.type = function ()
{
    return this._type;
};

std.map.prototype.name = function()
{
    return this._name;
};

std.map.prototype.data = function()
{
    return this._data;
};

std.map.prototype.dirtyKeys = function()
{
    return KeystoList(this._dirtyKeys);
};

std.map.prototype.keys = function()
{
    return KeystoList(this._keys);
};

std.map.prototype.size = function()
{
    size = 0;
    for (key in this._keys)
        size++;
    return size;
};

std.map.prototype.empty = function()
{
    for (key in this._keys)
        return false;
    return true;
}

var keyName = function(name, subname) {
    return name + ':' + subname;
};

var KeystoString = function(keys) {
    str='';
    for (key in keys)
        str = str+','+key;
    return str.substr(1);
};

var KeystoList = function(keys) {
    list = [];
    for (key in keys)
        list.push(key);
    return list;
};

var StringtoKeys = function(str) {
    list = str.split(',');
    keys = {};
    for (i=0; i<list.length; i++)
        keys[list[i]] = 1;
    return keys;
};
