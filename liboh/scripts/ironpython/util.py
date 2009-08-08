from protocol import Persistence_pb2
from protocol import MessageHeader_pb2

import traceback
from System import Array, Byte

def fromByteArray(b):
    return tuple(b)
def toByteArray(p):
    return Array[Byte](tuple(Byte(c) for c in p))

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
        #print repr(str(fromByteArray(bodyser)))
        response.ParseFromString(fromByteArray(bodyser))
        if response.HasField('return_status'):
            self.allcb(self, hdr, response.return_status)
            return False

        i = 0
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
                if self.indivcb:
                    self.indivcb(self, self.reads[i])

        for r in self.reads:
            if not r.HasField('data') and not r.HasField('return_status'):
                return True

        self.allcb(self)
        return False


