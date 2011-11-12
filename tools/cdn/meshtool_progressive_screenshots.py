import list
import pprint
import sys
import os
import os.path
import urllib2
import shutil
from meshtool.filters import factory
from meshtool.filters.print_filters.print_render_info import getRenderInfo
import collada
import unicodedata
import math
import traceback
import itertools
from multiprocessing import Process, Pool
from multiprocessing import current_process
from Queue import Queue
from threading import Thread, Lock
import tarfile

SERVER = 'http://open3dhub.com'
DOWNLOAD = SERVER + '/download'

#this will set number of workers equal to cpus in the system
NUM_PROCS = None

def decode(str):
    return unicodedata.normalize('NFKD', str).encode('ascii','ignore')

def save_screenshot(mesh_file, screenshot_file, zip_filename=None):
    screenshot = factory.getInstance('save_screenshot')
    mesh = collada.Collada(mesh_file, zip_filename=zip_filename)
    screenshot.apply(mesh, screenshot_file)

def progressive_screenshot(prog_dae, prog_pdae, percentage, screenshot_file, full_path):
    mesh = collada.Collada(prog_dae)
    
    if os.path.isfile(prog_pdae):
        add_pm = factory.getInstance('add_back_pm')
        try:
            mesh = add_pm.apply(mesh, prog_pdae, percentage)
        except collada.DaeError:
            print 'FAILED', full_path
            return
    
    screenshot = factory.getInstance('save_screenshot')
    screenshot.apply(mesh, screenshot_file)

def process_file(base_dir, f):
    current_process().daemon = False
    
    base_name = decode(f['base_name'])
    full_path = decode(f['full_path'])
    zip_hash = decode(f['metadata']['types']['original']['zip'])
    
    f_dir = full_path.replace('/','_')
    f_dir = os.path.join(base_dir, f_dir)
    if not os.path.isdir(f_dir):
        os.mkdir(f_dir)
    
    orig_zip = os.path.join(f_dir, base_name + '.orig.zip')
    if not os.path.isfile(orig_zip):
        f_data = urllib2.urlopen(DOWNLOAD + '/' + zip_hash).read()
        orig_zip_file = open(orig_zip, 'wb')
        orig_zip_file.write(f_data)
        orig_zip_file.close()
    
    orig_ss = orig_zip + '.png'
    
    mesh = collada.Collada(orig_zip, zip_filename=base_name)

    if not os.path.isfile(orig_ss):
        p = Process(target=save_screenshot, args=(orig_zip, orig_ss), kwargs={'zip_filename':base_name})
        p.start()
        p.join()
    
    progressive_dae_hash = f['metadata']['types']['progressive']['hash']
    progressive_pdae_hash = f['metadata']['types']['progressive']['progressive_stream']
    progressive_mipmaps_hash = f['metadata']['types']['progressive']['mipmaps']['./atlas.jpg']['hash']
    
    prog_dae = os.path.join(f_dir, base_name + '.progressive.dae')
    if not os.path.isfile(prog_dae):
        f_data = urllib2.urlopen(DOWNLOAD + '/' + progressive_dae_hash).read()
        prog_dae_file = open(prog_dae, 'wb')
        prog_dae_file.write(f_data)
        prog_dae_file.close()
        
    prog_pdae = os.path.join(f_dir, base_name + '.progressive.pdae')
    if not os.path.isfile(prog_pdae) and progressive_pdae_hash is not None:
        f_data = urllib2.urlopen(DOWNLOAD + '/' + progressive_pdae_hash).read()
        prog_pdae_file = open(prog_pdae, 'wb')
        prog_pdae_file.write(f_data)
        prog_pdae_file.close()
        
    prog_tar = os.path.join(f_dir, base_name + '.progressive.tar')
    if not os.path.isfile(prog_tar):
        f_data = urllib2.urlopen(DOWNLOAD + '/' + progressive_mipmaps_hash).read()
        prog_tar_file = open(prog_tar, 'wb')
        prog_tar_file.write(f_data)
        prog_tar_file.close()
        
    base_range = None
    more_ranges = []
    for byte_range in f['metadata']['types']['progressive']['mipmaps']['./atlas.jpg']['byte_ranges']:
        this_range = '%dx%d.jpg' % (byte_range['width'], byte_range['height'])
        if not(byte_range['width'] > 128 or byte_range['height'] > 128):
            base_range = this_range
        else:
            more_ranges.append(this_range)
    more_ranges.insert(0, base_range)
    
    mipmap_archive = tarfile.open(prog_tar)
    
    for prog_level in range(0,101,10):
        if len(more_ranges) > 0:
            mipmap_level = more_ranges.pop(0)
            mipmap_file = mipmap_archive.extractfile(mipmap_level)
            staged_texture = os.path.join(f_dir, 'atlas.jpg')
            staged_file = open(staged_texture, 'wb')
            staged_file.write(mipmap_file.read())
            staged_file.close()
            
        level_ss = os.path.join(f_dir, base_name + '.progressive.%d.png' % prog_level)
        if not os.path.isfile(level_ss):
            p = Process(target=progressive_screenshot, args=(prog_dae, prog_pdae, prog_level, level_ss, full_path))
            p.start()
            p.join()

def main():
    if len(sys.argv) != 2 or not os.path.isdir(sys.argv[1]):
        print 'usage: python meshtool_regression.py directory'
        sys.exit(1)
    base_dir = sys.argv[1]
    
    cdn_list = list.grab_list(SERVER, 10000, '', '', 'dump')
    #print 'got', len(cdn_list), 'results'
    
    pool = Pool(processes=NUM_PROCS)
    
    results = []
    for f in cdn_list:
        if 'original' in f['metadata']['types'] and 'progressive' in f['metadata']['types']:
            results.append((f['full_path'], pool.apply_async(process_file, args=(base_dir, f))))

    for r in results:
        r[1].wait()
        if not r[1].successful():
            print 'FAILURE', r[0]
            
            
if __name__ == "__main__":
    main()
