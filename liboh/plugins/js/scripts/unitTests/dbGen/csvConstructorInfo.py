#!/usr/bin/python

'''
A ConstructorInfo object contains all information about how to
initialize an entity.  This eventually gets output to a scene.db file.
'''

'''
@param {String} toWrap

@return ""toWrap""

Ie, if you call
print(self.stringWrap('mesh'));
> "\"mesh\""
'''
def stringWrap(toWrap):
    return '"\\"' + toWrap + '\\""'

'''
For empty fields in the scene.db file
'''
def emptyWrap():
    return '""';

def numWrap(toWrap):
    return str(toWrap);

##Note: this vector is ordered to agree with the order in which
##arguments for the scene.db file read by csvobjectfactory
DEFAULT_VAL_VECTOR = [
    ['objtype',stringWrap('mesh')],
    ['subtype',stringWrap('graphiconly')],
    ['name',stringWrap('tetrahedron')],
    ['parent',emptyWrap()],
    ['script',emptyWrap()],
    ['scriptparams',emptyWrap()],
    ['pos_x',numWrap(0)],
    ['pos_y',numWrap(0)],
    ['pos_z',numWrap(0)],
    ['orient_x',numWrap(0)],
    ['orient_y',numWrap(0)],
    ['orient_z',numWrap(1)],
    ['orient_w',numWrap(0)],
    ['vel_x',numWrap(0)],
    ['vel_y',numWrap(0)],
    ['vel_z',numWrap(0)],
    ["rot_axis_x",numWrap(0)],
    ["rot_axis_y",numWrap(0)],
    ["rot_axis_z",numWrap(0)],
    ["rot_speed",numWrap(0)],
    ["scale_x",numWrap(1)],
    ["scale_y",numWrap(1)],
    ["scale_z",numWrap(1)],
    ["hull_x",numWrap(1)],
    ["hull_y",numWrap(1)],
    ["hull_z",numWrap(1)],
    ["density",numWrap(1)],
    ["friction",numWrap(.3)],
    ["bounce",numWrap(.1)],
    ["colMask",numWrap(0)],
    ["colMsg",numWrap(0)],
    ["gravity",numWrap(1)],
    ["meshURI",stringWrap("meerkat:///test/multimtl.dae/original/0/multimtl.dae")],
    ["diffuse_x",emptyWrap()],
    ["diffuse_y",emptyWrap()],
    ["diffuse_z",emptyWrap()],
    ["ambient",emptyWrap()],
    ["specular_x",emptyWrap()],
    ["specular_y",emptyWrap()],
    ["specular_z",emptyWrap()],
    ["shadowpower",emptyWrap()],
    ["range",emptyWrap()],
    ["constantfall",emptyWrap()],
    ["linearfall",emptyWrap()],
    ["quadfall",emptyWrap()],
    ["cone_in",emptyWrap()],
    ["cone_out",emptyWrap()],
    ["power",emptyWrap()],
    ["cone_fall",emptyWrap()],
    ["shadow",emptyWrap()],
    ["scale",emptyWrap()],
    ["script_type",emptyWrap()],
    ["script_contents",emptyWrap()],
    ["solid_angle",emptyWrap()],
    ["physics",emptyWrap()]
    ];



def getCSVHeaderAsCSVString():
    returner = '';
    for s in DEFAULT_VAL_VECTOR:
        returner += s[0] + ',';

    return returner;

class CSVConstructorInfo:
    '''
    @see DEFAULT_VAL_VECTOR for a list of argument names that we can use.
    '''
    def __init__ (self, *args, **kwargs):

        for s in DEFAULT_VAL_VECTOR:
            self.checkArgsLoadDefaults(s[0],kwargs,s[1]);

        
    def checkArgsLoadDefaults(self,toCheckFor,toCheckIn,default):
        toExec = 'self.' + toCheckFor + '=';
        if (toCheckFor in toCheckIn):
            toExec += toCheckIn[toCheckFor];
        else:
            toExec += default;

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
    cInfo1= CSVConstructorInfo(pos_x=numWrap(-20),pos_y=numWrap(5),pos_z=numWrap(1.25), scale=numWrap(20), meshURI=stringWrap('meerkat:///kittyvision/hedgehog.dae/optimized/0/hedgehog.dae'));
    print('\n\n');
    print(getCSVHeaderAsCSVString());
    print(cInfo1.getConstructorRow());
    print('\n\n');

