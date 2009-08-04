import uuid
import traceback

import protocol.Sirikata_pb2 as pbSiri
import protocol.Persistence_pb2 as pbPer
import protocol.MessageHeader_pb2 as pbHead

from System import Array, Byte

def fromByteArray(b):
    return ''.join(chr(c) for c in b)
def toByteArray(p):
    return Array[Byte](tuple(Byte(ord(c)) for c in p))

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
    def processRPC(self,serialheader,name,serialarg):
        print "Got an RPC named",name
        header = pbHead.Header()
        header.ParseFromString(fromByteArray(serialheader))
        if name == "RetObj":
            print "sendprox1"
            self.spaceid = header.source_space
            print uuid.UUID(bytes=self.spaceid)
            self.sendNewProx()
        elif name == "ProxCall":
            proxcall = pbSiri.ProxCall()
            proxcall.ParseFromString(fromByteArray(serialarg))
            print "THIS IS PHYTHON AnD I SEE A OBJECT CALLED",uuid.UUID(bytes=proxcall.proximate_object)
    def sendNewProx(self):
        print "sendprox2"
        try:
            print "sendprox3"
            body = pbSiri.MessageBody()
            prox = pbSiri.NewProxQuery()
            prox.query_id = 123
            print "sendprox4"
            prox.max_radius = 1000.0
            body.message_names.append("NewProxQuery")
            body.message_arguments.append(prox.SerializeToString())
            header = pbHead.Header()
            print "sendprox5"
            header.destination_space = self.spaceid;
            header.destination_object = uuid.UUID(int=0).get_bytes()
            header.destination_port = 3 # libcore/src/util/KnownServices.hpp
            headerstr = header.SerializeToString()
            bodystr = body.SerializeToString()
            HostedObject.SendMessage(toByteArray(headerstr+bodystr))
        except:
            print "ERORR"
            traceback.print_exc()
            
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
if False:
    #
    hdr=ReadOnlyMessage()
    body=MessageBody()
#print "helloishdone"
    hdr.id=1
    hdr.destination_object="1234567890123456"
#print "helloishbaked"
    srlhdr=hdr.SerializeToString()
#print "helloishbakedflat"

    hdr1=ReadOnlyMessage()
    hdr1.MergeFromString(srlhdr)
    print hdr1.destination_object
    print hdr1.id
    print 5*6
