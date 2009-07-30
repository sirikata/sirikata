print "helloish"
import sys
print sys.path
print 5*6
def fact(i):
    if (i==0):
        return 1;
    return i*fact(i-1);
print fact(9)
class exampleclass:
    def __init__(self):
        self.val=0
    def func(self,otherval):
        self.val+=otherval
        print self.val
        return self.val;
