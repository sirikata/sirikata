""" csv_converter -- utility for converting csv to sqlite """
"""  Sirikata utility scripts
 *  csv_converter.py
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""
import math, sys
noisy=False
try:
    try:
        import sqlite3
    except:
        from pysqlite2 import dbapi2 as sqlite3
    import os
    import csv
    import math
    import random
    import time
    import uuid
    from urllib import unquote_plus
    import shutil
except:
    print "Missing library: ", sys.exc_info()[0],", not generating scene.db" 
    sys.exit(0);
basepath=''
if len(sys.argv):
    where1=sys.argv[0].rfind("/tools/oh/")
    where2=sys.argv[0].rfind("\\tools\\oh")
    if where1!=-1:
        if where2!=-1 and where2>where1:
            where1=where2
    else:
        where1=where2
    if where1!=-1:
        basepath=sys.argv[0][0:where1]+'/'
print basepath
sys.path.append(basepath+'liboh/scripts/ironpython')
sys.path.append(basepath+'liboh/scripts/ironpython/site-packages')

from protocol import Sirikata_pb2 as Sirikata
from protocol import Persistence_pb2 as Persistence
from protocol.MessageHeader_pb2 import Header

PHYSICAL_MODES = {
    'graphiconly': Sirikata.PhysicalParameters.NONPHYSICAL,
    'staticmesh': Sirikata.PhysicalParameters.STATIC,
#    'dynamicmesh': Sirikata.PhysicalParameters.DYNAMIC,
    'dynamicbox': Sirikata.PhysicalParameters.DYNAMICBOX,
    'dynamicsphere': Sirikata.PhysicalParameters.DYNAMICSPHERE,
    'dynamiccylinder': Sirikata.PhysicalParameters.DYNAMICCYLINDER,
    'character': Sirikata.PhysicalParameters.CHARACTER,
    '': Sirikata.PhysicalParameters.NONPHYSICAL
}

LIGHT_TYPES = {
    'point': Sirikata.LightInfoProperty.POINT,
    'spotlight': Sirikata.LightInfoProperty.SPOTLIGHT,
    'directional': Sirikata.LightInfoProperty.DIRECTIONAL,
    '': Sirikata.LightInfoProperty.POINT
}

ALLOWED_TYPES = {'mesh':1, 'camera':1, 'light':1}

def randomUUID():
    return uuid.uuid4()

def veclen(fltarray):
    return (fltarray[0]*fltarray[0] +
            fltarray[1]*fltarray[1] +
            fltarray[2]*fltarray[2])

DEG2RAD = 0.0174532925

def Euler2Quat(yaw, pitch, roll):
    k = DEG2RAD*.5
    yawcos = math.cos(yaw*k)
    yawsin = math.sin(yaw*k)
    pitchcos = math.cos(pitch*k)
    pitchsin = math.sin(pitch*k)
    rollcos = math.cos(roll*k)
    rollsin = math.sin(roll*k)

    return (rollcos * pitchsin * yawcos + rollsin * pitchcos * yawsin,
            rollcos * pitchcos * yawsin - rollsin * pitchsin * yawcos,
            rollsin * pitchcos * yawcos - rollcos * pitchsin * yawsin,
            rollcos * pitchcos * yawcos + rollsin * pitchsin * yawsin)

class CsvToSql:
    def __init__(self, conn):
        self.conn = conn
        self.name_to_uuid = {}
        self.uuid_objects = {}
        self.table_name = 'persistence'
        self.camera_count=0
        self.max_cameras=1

    def getTableName(self, uuid):
        return uuid.get_bytes()

    def getKeyName(self, name, index=0):
        return "%s_%d" % (name, index)

    def getUUID(self, name):
        if not name:
            return randomUUID()
        if name not in self.name_to_uuid:
            self.name_to_uuid[name] = randomUUID()
        return self.name_to_uuid[name]

    def addUUID(self, row):
        myid = self.getUUID(row['name'])
        if myid in self.uuid_objects:
            myid = randomUUID()
        self.uuid_objects[myid] = row
        return myid

    def addTable(self, curs):
        table_create = "CREATE TABLE IF NOT EXISTS "
        table_create += "\""+self.table_name+"\""
        table_create += "(object TEXT, key TEXT, value TEXT, PRIMARY KEY(object, key))"
        curs.execute(table_create)

    def protovec(self, listval, csvrow, fieldname):
        listval.append(float(csvrow[fieldname+'_x']))
        listval.append(float(csvrow[fieldname+'_y']))
        listval.append(float(csvrow[fieldname+'_z']))
    def charconvert(self, c):
        if type(c)==type(''):
            return c;
        return chr(c)
        
    def set(self, curs, uuid, key, value, which=0):
        object_name = self.getTableName(uuid)
        key_name = self.getKeyName(key, which)
        table_insert = "INSERT INTO \"" + self.table_name + "\""
        table_insert += " VALUES (?, ?, ?)"
        value = "".join(self.charconvert(c) for c in value);
        curs.execute(table_insert, (buffer(object_name), key_name, buffer(value)))

    def go(self, filelist, **csvargs):
        cursor = self.conn.cursor()
        self.addTable(cursor)

        uuidlist = Sirikata.UUIDListProperty()

        for openfile in filelist:
            reader = csv.DictReader(openfile, **csvargs)
            for row in reader:
                try:
                    if row['objtype'] not in ALLOWED_TYPES:
                        continue # blank or bad row

                    u = self.addUUID(row)
                    self.processRow(u, row, cursor)
                    uuidlist.value.append(u.get_bytes())
                except:
                    print row
                    raise
        nulluuid = uuid.UUID(int=0)
        self.set(cursor, nulluuid, 'ObjectList', uuidlist.SerializeToString())
        self.conn.commit()
        cursor.close()

    def processRow(self, uuid, row, cursor):
        if row['objtype']=='camera':
            self.camera_count += 1
            if self.camera_count > self.max_cameras:
                print "not a good idea to have too many cameras -- not adding this one"
                return
        location = Sirikata.ObjLoc()
        self.protovec(location.position, row, 'pos')
        if (row.get('orient_w','')):
            self.protovec(location.orientation, row, 'orient')
        else:
            """
            # Convert from angle
            phi = float(row['orient_x'])*math.pi/180.
            theta = float(row['orient_y'])*math.pi/180.
            psi = float(row['orient_z'])*math.pi/180.
            location.orientation.append(math.sin(phi/2)*math.cos(theta/2)*math.cos(psi/2)
                                      + math.cos(phi/2)*math.sin(theta/2)*math.sin(psi/2)) #x
            location.orientation.append(math.cos(phi/2)*math.sin(theta/2)*math.cos(psi/2)
                                      + math.sin(phi/2)*math.cos(theta/2)*math.sin(psi/2)) #y
            location.orientation.append(math.cos(phi/2)*math.cos(theta/2)*math.sin(psi/2)
                                      + math.sin(phi/2)*math.sin(theta/2)*math.cos(psi/2)) #z
            # w is not stored to save space, since quaternions are normalized
            location.orientation.append(math.cos(phi/2)*math.cos(theta/2)*math.cos(psi/2)
                                      + math.sin(phi/2)*math.sin(theta/2)*math.sin(psi/2)) #w
            """
            qx, qy, qz, qw = Euler2Quat(float(row['orient_y']), float(row['orient_x']), float(row['orient_z']))
            location.orientation.append(qx)
            location.orientation.append(qy)
            location.orientation.append(qz)
            location.orientation.append(qw)
        if row.get('vel_x',''):
            self.protovec(location.velocity, row, 'vel')
        if row.get('rot_axis_x',''):
            self.protovec(location.rotational_axis, row, 'rot_axis')
        if row.get('rot_speed',''):
            location.angular_speed = float(row['rot_speed'])
        self.set(cursor, uuid, 'Loc', location.SerializeToString())

        if row.get('script',''):
            scrprop = Sirikata.StringProperty()
            scrprop.value = row['script']
            self.set(cursor, uuid, '_Script', scrprop.SerializeToString())
            if noisy:
                print row['script']
            scrprop = Sirikata.StringMapProperty()
            for kv in row['scriptparams'].split('&'):
                key, value = kv.split('=',1)
                scrprop.keys.append(unquote_plus(key))
                scrprop.values.append(unquote_plus(value))
                if noisy:
                    print 'param',key,'=',value
            self.set(cursor, uuid, '_ScriptParams', scrprop.SerializeToString())

        if row.get('parent',''):
            parentprop = Sirikata.ParentProperty()
            parentprop.value = self.getUUID(row['parent']).get_bytes()
            self.set(cursor, uuid, 'Parent', parentprop.SerializeToString())
        if row['objtype']=='mesh':
            scale = Sirikata.Vector3fProperty()
            self.protovec(scale.value, row, 'scale')
            self.set(cursor, uuid, 'MeshScale', scale.SerializeToString())
            physical = Sirikata.PhysicalParameters()
            physical.mode = PHYSICAL_MODES['']
            if row.get('subtype',''):
                physical.mode = PHYSICAL_MODES[row['subtype']]
            if physical.mode != PHYSICAL_MODES['']:
                physical.density = float(row['density'])
                physical.friction = float(row['friction'])
                physical.bounce = float(row['bounce'])
                physical.collide_mask = int(row['colMask'])
                physical.collide_msg = int(row['colMsg'])
                if row.get('gravity',''):
                    physical.gravity = float(row['gravity'])
                else:
                    physical.gravity = 1.0
                if row.get('hull_x',''):
                    self.protovec(physical.hull, row, 'hull')
                self.set(cursor, uuid, 'PhysicalParameters', physical.SerializeToString())
            meshuri = Sirikata.StringProperty()
            meshuri.value = row['meshURI']
            self.set(cursor, uuid, 'MeshURI', meshuri.SerializeToString())
            if row.get('webpage', ''):
                weburi = Sirikata.StringProperty()
                weburi.value = row['webpage']
                self.set(cursor, uuid, 'WebViewURL', weburi.SerializeToString())
            if (row.get('name','')):
                meshuri = Sirikata.StringProperty()
                meshuri.value = row['name']
                self.set(cursor, uuid, 'Name', meshuri.SerializeToString())
            if noisy:
                print "** Adding a Mesh ",uuid,"named",row.get('name',''),"with",row['meshURI']
        elif row['objtype']=='light':
            if noisy:
                print "** Adding a Light ",uuid
            lightinfo = Sirikata.LightInfoProperty()
            if row.get('subtype',''):
                lightinfo.type = LIGHT_TYPES[row['subtype']]
            self.protovec(lightinfo.diffuse_color, row, 'diffuse')
            self.protovec(lightinfo.specular_color, row, 'specular')
            lightinfo.power = float(row['power'])
            if row.get('ambient_x',''):
                self.protovec(lightinfo.ambient_color, row, 'ambient')
            elif row.get('ambient',''):
                ambientPower = float(row['ambient'])
                diffuseColorLength = veclen(lightinfo.diffuse_color)
                for i in range(3):
                    if diffuseColorLength:
                        lightinfo.ambient_color.append(lightinfo.diffuse_color[i]*
                                                       (ambientPower/diffuseColorLength)/lightinfo.power)
                    else:
                        lightinfo.ambient_color.append(0.0)
            if row.get('shadow_x',''):
                self.protovec(lightinfo.shadow_color, row, 'shadow')
            elif row.get('shadowpower',''):
                shadowPower = float(row['shadowpower'])
                specularColorLength = veclen(lightinfo.specular_color)
                for i in range(3):
                    if specularColorLength:
                        lightinfo.shadow_color.append(lightinfo.specular_color[i]*(shadowPower/specularColorLength)/lightinfo.power)
                    else:
                        lightinfo.shadow_color.append(0.0)
            lightinfo.light_range = float(row['range'])
            if 'constantfall' in row:
                lightinfo.constant_falloff = float(row['constantfall'])
            elif 'constfall' in row:
                lightinfo.constant_falloff = float(row['constfall'])
                if lightinfo.constant_falloff < 0.01:
                    lightinfo.constant_falloff = 1.0                #bug in artist exporter -- 0 makes no sense, make it 1
            lightinfo.linear_falloff = float(row['linearfall'])
            lightinfo.quadratic_falloff = float(row['quadfall'])
            lightinfo.cone_inner_radians = float(row['cone_in'])
            lightinfo.cone_outer_radians = float(row['cone_out'])
            lightinfo.cone_falloff = float(row['cone_fall'])
            lightinfo.casts_shadow = bool(row['shadow'])
            if noisy:
                print lightinfo
            self.set(cursor, uuid, 'LightInfo', lightinfo.SerializeToString())
        elif row['objtype']=='camera':
            if noisy:
                print "** Adding a Camera ",uuid
            self.set(cursor, uuid, 'IsCamera', '')

if __name__=='__main__':
    sqlfile = 'scene.db'
    if len(sys.argv) > 1:
        if len(sys.argv) == 2:
            csvfiles = [sys.argv[1]]
        else:
            csvfiles = []
            for i in sys.argv[1:-1]:
                csvfiles.append(i)
            sqlfile = sys.argv[-1]
    else:
        csvfiles = ['scene.csv']

    try:
        os.rename(sqlfile, sqlfile+'.bak')
    except OSError:
        pass

    csvfile = csvfiles[0]
    print "converting:", csvfile, "to:", sqlfile
    shutil.copy(csvfile, sqlfile)

    # disabled in favor of using CSV directly
    if False:
        # Old SQL format
        conn = sqlite3.connect(sqlfile)
        converter = CsvToSql(conn)
        handles = []
        for fname in csvfiles:
            handles.append(open(fname))
        converter.go(handles)
        for han in handles:
            han.close()
        print "Generating scene: SUCCESS!"
