
// Simple "dead" object that just connects to the space to fill it up with
// presences

var connectObject = function(pos, cb) {
    params = {
        'space' : '12345678-1111-1111-1111-DEFA01759ACE',
        'pos' : pos
    };
    if (cb) params['callback'] = cb;
    system.createPresence(params);
};

connectObject(<0,0,0>);