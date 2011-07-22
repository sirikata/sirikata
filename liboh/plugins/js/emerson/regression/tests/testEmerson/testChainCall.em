var f = {};
f.fun = function(x) {
    return this;
};
f.fun().fun().fun();
