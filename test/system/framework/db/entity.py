#!/usr/bin/python


def stringWrap(toWrap):
    '''
    @param {String} toWrap

    @return ""toWrap""

    Ie, if you call
    print(self.stringWrap('mesh'));
    > "\"mesh\""
    '''

    if not toWrap:
        return '""'
    return '"\\"' + toWrap + '\\""'

def numWrap(toWrap):
    return str(toWrap);

##Note: this vector is ordered to agree with the order in which
##arguments for the scene.db file read by csvobjectfactory
DEFAULT_VAL_VECTOR = [
    ['objtype', 'mesh', stringWrap],
    ['pos_x', 0, numWrap],
    ['pos_y', 0, numWrap],
    ['pos_z', 0, numWrap],
    ['orient_x', 0, numWrap],
    ['orient_y', 0, numWrap],
    ['orient_z', 1, numWrap],
    ['orient_w', 0, numWrap],
    ['vel_x', 0, numWrap],
    ['vel_y', 0, numWrap],
    ['vel_z', 0, numWrap],
    ["rot_axis_x", 0, numWrap],
    ["rot_axis_y", 0, numWrap],
    ["rot_axis_z", 0, numWrap],
    ["rot_speed", 0, numWrap],
    ["meshURI", "meerkat:///test/multimtl.dae/original/0/multimtl.dae", stringWrap],
    ["scale", 1, numWrap],
    ["script_type", '', stringWrap],
    ["script_contents", '', stringWrap],
    ["solid_angle", 12, numWrap],
    ["physics", '', stringWrap]
    ];

def getCSVHeaderAsCSVString():
    returner = '';
    for s in DEFAULT_VAL_VECTOR:
        returner += s[0] + ',';

    return returner;

class Entity:
    '''
    An Entity object contains all information about how to
    initialize an entity.  This eventually gets output to a scene.db file.
    '''

    def __init__ (self, *args, **kwargs):
        '''
        @see DEFAULT_VAL_VECTOR for a list of argument names that we can use.
        '''

        for s in DEFAULT_VAL_VECTOR:
            self.checkArgsLoadDefaults(s[0],kwargs,s[1],s[2]);


    def checkArgsLoadDefaults(self,toCheckFor,toCheckIn,default,wrapFn):
        toExec = 'self.' + toCheckFor + '=';
        if (toCheckFor in toCheckIn):
            toExec += wrapFn(toCheckIn[toCheckFor]);
        else:
            toExec += wrapFn(default);

        exec (toExec);


    '''
    @return {String} returns a comma-separated string with all the
    constructor info necessary for csvobjectfactory to construct an
    entity with specified characteristcs.
    '''
    def getConstructorRow(self):
        returner = '';
        #iterate through default val vector to get order to write
        #field names
        for s in DEFAULT_VAL_VECTOR:
            returner += str(self.getSelfData(s[0])) + ',';

        return returner;

    def getSelfData(self,fieldName):
        exec ('returner = self.' + fieldName);
        return returner;


#Only to test to ensure that this code is working correctly.
if __name__ == "__main__":
    cInfo1= Entity(pos_x=-20,pos_y=5,pos_z=1.25, scale=20, meshURI='meerkat:///kittyvision/hedgehog.dae/optimized/0/hedgehog.dae');
    print('\n\n');
    print(getCSVHeaderAsCSVString());
    print(cInfo1.getConstructorRow());
    print('\n\n');
