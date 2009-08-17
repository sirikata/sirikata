import uuid
import traceback

import protocol.Sirikata_pb2 as pbSiri
import protocol.Persistence_pb2 as pbPer
import protocol.MessageHeader_pb2 as pbHead

from Sirikata.Runtime import HostedObject
print dir(HostedObject)

import util

DEBUG_OUTPUT=False

class exampleclass:
    def __init__(self):
        self.val=0
    def func(self,otherval):
        self.val+=otherval
        print self.val
        return self.val;
    def reallyProcessRPC(self,serialheader,name,serialarg):
        print "Got an RPC named",name
        header = pbHead.Header()
        header.ParseFromString(util.fromByteArray(serialheader))
        if name == "RetObj":
            retobj = pbSiri.RetObj()
            #print repr(util.fromByteArray(serialarg))
            try:
                retobj.ParseFromString(util.fromByteArray(serialarg))
            except:
                pass
            self.objid = util.tupleToUUID(retobj.object_reference)
            print "sendprox1"
            self.spaceid = util.tupleToUUID(header.source_space)

            print self.spaceid
            self.sendNewProx()
            self.setPosition(angular_speed=0,axis=(0,1,0))
        elif name == "ProxCall":
            proxcall = pbSiri.ProxCall()
            proxcall.ParseFromString(util.fromByteArray(serialarg))
            objRef = util.tupleToUUID(proxcall.proximate_object)
            if proxcall.proximity_event == pbSiri.ProxCall.ENTERED_PROXIMITY:
                myhdr = pbHead.Header()
                myhdr.destination_space = util.tupleFromUUID(self.spaceid)
                myhdr.destination_object = proxcall.proximate_object
                dbQuery = util.PersistenceRead(self.sawAnotherObject)
                field = dbQuery.reads.add()
                field.field_name = 'Name'
                dbQuery.send(HostedObject, myhdr)
            if proxcall.proximity_event == pbSiri.ProxCall.EXITED_PROXIMITY:
                pass
    def sawAnotherObject(self,persistence,header,retstatus):
        if DEBUG_OUTPUT: print "PY: sawAnotherObject called"
        if header.HasField('return_status') or retstatus:
            return
        uuid = util.tupleToUUID(header.source_object)
        myName = ""
        for field in persistence.reads:
            if field.field_name == 'Name':
                if field.HasField('data'):
                    nameStruct=pbSiri.StringProperty()
                    nameStruct.ParseFromString(field.data)
                    myName = nameStruct.value
        if DEBUG_OUTPUT: print "PY: Object",uuid,"has name",myName
        if myName[:6]=="Avatar":
            rws=pbPer.ReadWriteSet()
            se=rws.writes.add()
            se.field_name="Parent"
            parentProperty=pbSiri.ParentProperty()
            parentProperty.value=util.tupleFromUUID(uuid);
            se.data=parentProperty.SerializeToString()
            header = pbHead.Header()
            header.destination_space=util.tupleFromUUID(self.spaceid);
            header.destination_object=util.tupleFromUUID(self.objid);
            header.destination_port=5#FIXME this should be PERSISTENCE_SERVICE_PORT
            HostedObject.SendMessage(header.SerializeToString()+rws.SerializeToString());
    def processRPC(self,header,name,arg):
        try:
            self.reallyProcessRPC(header,name,arg)
        except:
            print "Error processing RPC",name
            traceback.print_exc()
    def setPosition(self,position=None,orientation=None,velocity=None,angular_speed=None,axis=None,force=False):
        objloc = pbSiri.ObjLoc()
        if position is not None:
            for i in range(3):
                objloc.position.append(position[i])
        if velocity is not None:
            for i in range(3):
                objloc.velocity.append(velocity[i])
        if orientation is not None:
            total = 0
            for i in range(4):
                total += orientation[i]*orientation[i]
            total = total**.5
            for i in range(3):
                objloc.orientation.append(orientation[i]/total)
        if angular_speed is not None:
            objloc.angular_speed = angular_speed
        if axis is not None:
            total = 0
            for i in range(3):
                total += axis[i]*axis[i]
            total = total**.5
            for i in range(2):
                objloc.rotational_axis.append(axis[i]/total)
        if force:
            objloc.update_flags = pbSiri.ObjLoc.FORCE
        body = pbSiri.MessageBody()
        body.message_names.append("SetLoc")
        body.message_arguments.append(objloc.SerializeToString())
        header = pbHead.Header()
        header.destination_space = util.tupleFromUUID(self.spaceid)
        header.destination_object = util.tupleFromUUID(self.objid)
        HostedObject.SendMessage(util.toByteArray(header.SerializeToString()+body.SerializeToString()))
    def sendNewProx(self):
        print "sendprox2"
        try:
            print "sendprox3"
            body = pbSiri.MessageBody()
            prox = pbSiri.NewProxQuery()
            prox.query_id = 123
            print "sendprox4"
            prox.max_radius = 1.0e+30
            body.message_names.append("NewProxQuery")
            body.message_arguments.append(prox.SerializeToString())
            header = pbHead.Header()
            print "sendprox5"
            header.destination_space = util.tupleFromUUID(self.spaceid);
            header.destination_object = util.tupleFromUUID(uuid.UUID(int=0))
            header.destination_port = 3 # libcore/src/util/KnownServices.hpp
            headerstr = header.SerializeToString()
            bodystr = body.SerializeToString()
            HostedObject.SendMessage(util.toByteArray(headerstr+bodystr))
        except:
            print "ERORR"
            traceback.print_exc()

    def processMessage(self,header,body):
        print "Got a message"
    def tick(self,tim):
        x=str(tim)
        print "Current time is "+x;
        #HostedObject.SendMessage(tuple(Byte(ord(c)) for c in x));# this seems to get into hosted object...but fails due to bad encoding
