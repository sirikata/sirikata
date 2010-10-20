#!/usr/bin/python

import feedparser
import urllib
import os.path
import os
import time
import pickle
import zipfile
import xml.dom.minidom
import math
import euclid

def extract_entity_id(entity_url):
    id_url_prefix = 'http://sketchup.google.com/3dwarehouse/data/entities/'
    assert(entity_url.startswith(id_url_prefix))
    return entity_url[len(id_url_prefix):]

def fetch_model_list(place):
    url = 'http://sketchup.google.com/3dwarehouse/data/entities?q=is:model+is:best-of-geo+is:geo+is:downloadable'
    url += '+near:%22' + place + '%22'
    url += '&scoring=d&max-results=1000'

    url = url.replace(' ', '+')

    d = feedparser.parse(url)
    objects = set()
    for x in d.entries:
        objects.add(extract_entity_id(x.id))

    return objects

def build_download_url(model_id):
    return 'http://sketchup.google.com/3dwarehouse/download?mid=' + model_id + '&rtyp=zs&fn=Untitled&ctyp=other' + '&ts=1262100548000'

def model_filename(model_id, work_dir):
    return os.path.join(work_dir, str(model_id) + '.zip')

def have_model(model_id, work_dir):
    fname = model_filename(model_id, work_dir)
    if (os.path.exists(fname)): return True
    return False

def download_model(model_id, work_dir):
    if have_model(model_id, work_dir): return

    fname = model_filename(model_id, work_dir)

    dl_url = build_download_url(model_id)
    filename, headers = urllib.urlretrieve(dl_url, fname)


class ColladaZip:
    def __init__(self, zf):
        self.zf = zipfile.ZipFile(zf)

    def find_kmls(self):
        return [x for x in self.zf.namelist() if x.endswith('.kml')]

    def get_file(self, fname):
        return self.zf.open(fname)

class ModelPosition:
    def __init__(self, lat, lng, alt):
        self.lat, self.long, self.alt = lat, lng, alt

class ModelOrientation:
    def __init__(self, heading, tilt, roll):
        self.heading, self.tilt, self.roll = heading, tilt, roll

class ModelScale:
    def __init__(self, x, y, z):
        self.x, self.y, self.z = x, y, z

class ModelLocation:
    def __init__(self, pos, orient, scale):
        self.pos = pos
        self.orientation = orient
        self.scale = scale


def getXmlChildByName(xml, tag):
    node = xml.getElementsByTagName(tag)
    assert(len(node) <= 1)
    if (len(node) == 0): return None
    (node,) = node
    return node

def getXmlContents(xml):
    return xml.childNodes[0].data

def extract_kml_info(model_id, work_dir):
    assert(have_model(model_id, work_dir))

    fname = model_filename(model_id, work_dir)
    cz = ColladaZip(fname)

    kmls = cz.find_kmls()
    assert(len(kmls) == 1)
    (kml,) = kmls

    kml_xml = xml.dom.minidom.parse( cz.get_file(kml) )

    model_node = getXmlChildByName(kml_xml, 'Model')

    loc_node = getXmlChildByName(kml_xml, 'Location')
    loc_lat = float(getXmlContents(getXmlChildByName(loc_node, 'latitude')))
    loc_long = float(getXmlContents(getXmlChildByName(loc_node, 'longitude')))
    loc_alt = float(getXmlContents(getXmlChildByName(loc_node, 'altitude')))

    orient_node = getXmlChildByName(kml_xml, 'Orientation')
    orient_heading = float(getXmlContents(getXmlChildByName(orient_node, 'heading')))
    orient_tilt = float(getXmlContents(getXmlChildByName(orient_node, 'tilt')))
    orient_roll = float(getXmlContents(getXmlChildByName(orient_node, 'roll')))

    scale_node = getXmlChildByName(kml_xml, 'Scale')
    sx = float(getXmlContents(getXmlChildByName(scale_node, 'x')))
    sy = float(getXmlContents(getXmlChildByName(scale_node, 'y')))
    sz = float(getXmlContents(getXmlChildByName(scale_node, 'z')))

    return  ModelLocation(ModelPosition(loc_lat, loc_long, loc_alt), ModelOrientation(orient_heading, orient_tilt, orient_roll), ModelScale(sx, sy, sz))

def average(l):
    return sum(l) / float(len(l))

class WarehouseScene:
    def __init__(self, name, max_rate=10):
        self._work_dir = name
        self._max_rate = max_rate

        self._setup_work_dir()
        self._load_db()

    def _setup_work_dir(self):
        if not os.path.exists(self._work_dir):
            os.makedirs(self._work_dir)

    def _db_file(self):
        return os.path.join(self._work_dir, 'db')

    def _load_db(self):
        # Our 'database' is just pickled summary data
        if os.path.exists(self._db_file()):
            f = open(self._db_file(), 'r')
            self._db = pickle.load(f)
            f.close()
            del f
            return
        # Or start fresh
        self._db = set()

    def _save_db(self):
        f = open(self._db_file(), 'w')
        pickle.dump(self._db, f)
        f.close()
        del f

    def add(self, place):
        objects = fetch_model_list('chinatown new york new york')
        self._db = self._db | objects;
        self._save_db()

    def download(self):
        print 'Downloading...'
        idx = 1
        for objid in self._db:
            print objid, '(%d of %d)' % (idx, len(self._db))
            idx += 1
            if have_model(objid, self._work_dir): continue
            download_model(objid, self._work_dir)
            time.sleep(self._max_rate)

    def generateCSV(self, fname):
        '''Generates a CSV scene file for this WarehouseScene.'''

        # First, we need to extract all the basic information available in the kml files
        objdata = {}
        for objid in self._db:
            objdata[objid] = extract_kml_info(objid, self._work_dir)

        # Next, figure out where to center things
        avg_lat = average([obj.pos.lat for obj in objdata.values()])
        avg_long = average([obj.pos.long for obj in objdata.values()])

        # Based on our central point, we need to compute where
        # everything is in relation to it. We also need to take care
        # of rotating everything so that the specified lat,long's
        # normal is y-up
        earth_rad = 6371000.0 # meters

        print avg_lat, avg_long

        avg_theta = math.radians(avg_long)
        avg_phi = math.radians(90 - avg_lat)

        # Generate quaternion for rotating points into the central point's frame
        rot_center = \
            euclid.Quaternion.new_rotate_axis(-avg_phi, euclid.Vector3(0, 0, -1)) * \
            euclid.Quaternion.new_rotate_axis(-avg_theta, euclid.Vector3(0, -1, 0))

        # Now we have all the info we need to create the scene
        with open(fname, 'w') as fout:
            print >>fout, '"objtype","pos_x","pos_y","pos_z","orient_x","orient_y","orient_z","orient_w","meshURI","scale"'
            # In these scenes, we always need lights added
            print >>fout, '"mesh",0,0,0,0,0,0,1,"meerkat:///ewencp/DirectionalLights.dae",1'
            for objid in self._db:
                obj = objdata[objid]

                # Get in spherical coordinates
                theta = math.radians(obj.pos.long)
                phi = math.radians(90 - obj.pos.lat)
                # Get in cartesian coords
                pos = euclid.Vector3(
                    x = earth_rad * math.cos(theta) * math.sin(phi),
                    y = earth_rad * math.cos(phi),
                    z = earth_rad * math.sin(theta) * math.sin(phi)
                    )

                # Rotate into position
                pos = rot_center * pos
                # And translate to the "center of the earth," i.e. the origin,
                # in order to get things in a sane coordinate system
                pos.y -= earth_rad

                rot = [0,0,0,1]
                mesh = "meerkat:///ewencp/%s.dae" % (objid)
                size = min(obj.scale.x, obj.scale.y, obj.scale.z)
                print >>fout, '"mesh",%f,%f,%f,%f,%f,%f,%f,"%s",%f' % (pos[0], pos[1], pos[2], rot[0], rot[1], rot[2], rot[3], mesh, size)



if __name__ == '__main__':
    scene = WarehouseScene('chinatown')
    #scene.add('chinatown new york new york')
    #scene.download()
    scene.generateCSV('chinatown.scene.db')
