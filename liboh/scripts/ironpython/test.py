print "helloish"
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
    def processRPC(self,header,name,arg):
        print "Got an RPC named "+name
    def processMessage(self,header,body):
        print "Got a message"
    def tick(self,tim):
        x=str(tim)
        print "Current time is "+x;
        #HostedObject.SendMessage(tuple(Byte(ord(c)) for c in x));# this seems to get into hosted object...but fails due to bad encoding
from Sirikata.Runtime import HostedObject
from System import Array, Byte

print dir(HostedObject)
x="hello worlds"

#
from Sirikata_pb2 import *
hdr=ReadOnlyMessage()
body=MessageBody()
print "helloishdone"
hdr.id=1
hdr.dest="1234567890123456"
print "helloishbaked"
#srlhdr=hdr.SerializeToString()
#print "helloishbakedflat"

#hdr1=ReadOnlyMessage()
#hdr1.MergeFromString(srlhdr)
#print hdr1.dest
#print hdr1.id
#print 5*6
