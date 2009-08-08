import uuid
import traceback
import protocol.Sirikata_pb2 as pbSiri
import protocol.MessageHeader_pb2 as pbHead
from System import Array, Byte
from Sirikata.Runtime import HostedObject

def fromByteArray(b):
    return ''.join(chr(c) for c in b)

def toByteArray(p):
    return Array[Byte](tuple(Byte(ord(c)) for c in p))


class exampleclass:

    def reallyProcessRPC(self,serialheader,name,serialarg):
        print "PY:", "Got an RPC named-->" + name + "<--"
        header = pbHead.Header()
        header.ParseFromString(fromByteArray(serialheader))
        if name == "RetObj":
            retobj = pbSiri.RetObj()
            try:
                retobj.ParseFromString(fromByteArray(serialarg))
            except:
                pass
            self.objid = retobj.object_reference
            print "PY: my UUID is:", uuid.UUID(bytes=self.objid)
            self.spaceid = header.source_space
            print "PY: space UUID:", uuid.UUID(bytes=self.spaceid)
            print "PY: sending LocRequest"
            body = pbSiri.MessageBody()
            body.message_names.append("LocRequest")
            args = pbSiri.LocRequest()
            args.requested_fields=1
            body.message_arguments.append(args.SerializeToString())
            header = pbHead.Header()
            header.destination_space = self.spaceid
            header.destination_object = self.objid
            HostedObject.SendMessage(toByteArray(header.SerializeToString()+body.SerializeToString()))

    def processRPC(self,header,name,arg):
        try:
            self.reallyProcessRPC(header,name,arg)
        except:
            print "PY:", "Error processing RPC",name
            traceback.print_exc()
