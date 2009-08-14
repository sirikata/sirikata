from protocol import Persistence_pb2
from protocol import MessageHeader_pb2

import traceback
from System import Array, Byte

def fromByteArray(b):
    return tuple(b)
def toByteArray(p):
    return Array[Byte](tuple(Byte(c) for c in p))
def tupleToBigEndian(t):
    retval=0
    for i in t:
        retval*=256;
        retval+=i
    return retval
def tupleToLittleEndian(t):
    retval=0
    mult=1
    for i in t:
        adder=mult
        adder*=i;
        retval+=adder
        mult*=256;
    return retval
def bigEndianTuple16(i):
    return ((i/(256**15))%256,(i/(256**14))%256,(i/(256**13))%256,(i/(256**12))%256,(i/(256**11))%256,(i/(256**10))%256,(i/(256**9))%256,(i/(256**8))%256,(i/(256**7))%256,(i/(256**6))%256,(i/(256**5))%256,(i/(256**4))%256,(i/(256**3))%256,(i/(256**2))%256,(i/(256**1))%256,i%256)
def littleEndianTuple16(i):
    return (i%256,(i/(256**1))%256,(i/(256**2))%256,(i/(256**3))%256,(i/(256**4))%256,(i/(256**5))%256,(i/(256**6))%256,(i/(256**7))%256,(i/(256**8))%256,(i/(256**9))%256,(i/(256**10))%256,(i/(256**11))%256,(i/(256**12))%256,(i/(256**13))%256,(i/(256**14))%256,(i/(256**15))%256)

import uuid
def tupleToUUID(t):
    return uuid.UUID(int=tupleToBigEndian(t))
def tupleFromUUID(u):
    return bigEndianTuple16(int(u))

#Callback is of the format
#def callback(persistenceReadInstance, lastMessageHeader, persistenceError)

class PersistenceRead:
    def __init__(self, allcb):
        self.rws = Persistence_pb2.ReadWriteSet()
        self.reads = self.rws.reads
        self.writes = self.rws.writes
        self.allcb = allcb

    def send(self, sender, header):
        i = 0
        for r in self.reads:
            r.index = i
            i += 1
        i = 0
        for w in self.writes:
            w.index = i
            i += 1

        if not header.HasField('destination_port'):
            header.destination_port = 5
        print "SENDING SOMETHHHING!!!!",self.callback
        sender.CallFunction(toByteArray(header.SerializeToString() +
                                            self.rws.SerializeToString()),
                                self.callback)

    def callback(self, headerser, bodyser):
        try:
            self.docallback(headerser, bodyser)
        except:
            print "Error processing PersistenceRead callback"
            traceback.print_exc()
    def docallback(self, headerser, bodyser):
        hdr = MessageHeader_pb2.Header()
        hdr.ParseFromString(fromByteArray(headerser))
        if hdr.HasField('return_status'):
            self.allcb(self, hdr, None)
            return False
        response = Persistence_pb2.Response()
        response.ParseFromString(fromByteArray(bodyser))
        if response.HasField('return_status'):
            self.allcb(self, hdr, response.return_status)
            return False

        i = -1
        for r in response.reads:
            if r.HasField('index'):
                i = r.index
            else:
                i += 1
            if i >= 0 and i < len(self.reads):
                if r.HasField('data'):
                    self.reads[i].data = r.data
                if r.HasField('return_status'):
                    self.reads[i].return_status = r.return_status
##                if self.indivcb:
##                    self.indivcb(self, self.reads[i])

        for r in self.reads:
            if not r.HasField('data') and not r.HasField('return_status'):
                return True

        self.allcb(self, hdr, None)
        return False


