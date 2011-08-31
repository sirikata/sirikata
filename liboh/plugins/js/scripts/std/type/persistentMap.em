if (typeof(std) === "undefined") /** @namespace */ std = {};

/* Map data structure
 * Constructor:
 *   e.g. X = new std.persistentMap('mapTest'); 
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
 *   keys()             : Return all keys in both memory and backend, as an array.
 *   size()             : Return number of elements in memory and backend
 *   empty()            : Return true if map is emty, false otherwise   
 *   contains(key)      : Return true if the map contains the key, false otherwise
 *   isResident(key)    : Return true if the key is resident in memory, false otherwise
 */

std.persistentMap = function(name) 
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

std.persistentMap.prototype._init = function(cb)
{
    system.storageRead(this._mapName, std.core.bind(this._initCommit, this, cb));
};

std.persistentMap.prototype._initCommit = function(cb, success, val)
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

std.persistentMap.prototype.set = function(key,value)
{
    this._data[key]=value;
    this._dirtyKeys[key] = 1;
    this._keys[key] = 1;
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
    delete this._keys[key];
    if (key in this._keys_list)
        this._dirtyKeys[key] = 0;
    else
        delete this._dirtyKeys[key];
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
    system.storageWrite(this._mapName, KeystoString(this._keys));
    system.storageCommit(std.core.bind(this._flushCommit, this, cb));
};

std.persistentMap.prototype._flushCommit = function(cb, success)
{
    if (!success)
        system.print('Flush fails');
    else {
        this._dirtyKeys={};
        this._keys_list = this._keys;
    }
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
    system.storageRead(this._mapName, std.core.bind(this._restore, this, cb));
};

std.persistentMap.prototype._restore = function(cb, success, val) 
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

std.persistentMap.prototype._restoreCommit = function(cb, success, val)
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

std.persistentMap.prototype.reset = function(cb)
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

std.persistentMap.prototype._resetCommit = function(cb, success) 
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

std.persistentMap.prototype.contains = function(key)
{
    if (key in this._keys)
        return true;
    else
        return false;
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

std.persistentMap.prototype.keys = function()
{
    return KeystoList(this._keys);
};

std.persistentMap.prototype.size = function()
{
    size = 0;
    for (key in this._keys)
        size++;
    return size;
};

std.persistentMap.prototype.empty = function()
{
    for (key in this._keys)
        return false;
    return true;
};

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
