system.require('std/core/namespace.em');

Namespace('std.core').PresencePool = function(numPresences) {
    this.numPresences = numPresences;
    this._presences = [];
    this._allocated = [];
    this._connectCallback = null;
}

std.core.PresencePool.prototype.numConnected = function() {
    return this._presences.length;
}

std.core.PresencePool._clearPres = function(pres) {
    pres.setScale(0);
    pres.setMesh('');
    pres.setOrientation(new util.Quaternion(0, 0, 0, 1));
    pres.setModelOrientation(new util.Quaternion(0, 0, 0, 1));
    pres.setVelocity(<0, 0, 0>);
    pres.setOrientationVel(new util.Quaternion(0, 0, 0, 1));
}

std.core.PresencePool.prototype.connectCallback = function(pres) {
    this._presences.push(pres);
    this._clearPres(pres);
    system.__debugPrint('' + this._presences.length + '\n');
    if (this._presences.length == this.numPresences)
        this._connectCallback();
}

std.core.PresencePool.prototype.connectAll = function(callback) {
    if (this._presences.length > 0)
        return;

    this._connectCallback = callback;
    for (var i = 0; i < this.numPresences; i++) {
        system.createPresence({
            position: <0, 0, 0>,
            mesh: '',
            scale: 0,
            callback: std.core.bind(this.connectCallback, this)
        });
    }
}

std.core.PresencePool.prototype.allocate = function() {
    if (this._presences.length == 0)
        return null;
    var pres = this._presences.shift();
    this._allocated.push(pres);
    return pres;
}

std.core.PresencePool.prototype.free = function(presence) {
    var index = this._allocated.indexOf(presence);
    if (index == -1)
        return;
    this._presences.push(presence);
    this._allocated.splice(index, 1);
    this._clearPres(presence);
}

std.core.PresencePool.prototype.freeAll = function() {
    this._presences = this._presences.concat(this._allocated);
    this._allocated = [];
}

std.core.PresencePool.prototype.destroy = function() {
    for (var i = 0; i < this._presences.length; i++)
        this._presences[i].disconnect();
}
