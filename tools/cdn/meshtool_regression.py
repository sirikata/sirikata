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
        
    optimizations = factory.getInstance('full_optimizations')
    save = factory.getInstance('save_collada_zip')
    
    orig_ss = orig_zip + '.png'
    opt_zip = os.path.join(f_dir, base_name + '.opt.zip')
    opt_ss = opt_zip + '.png'
    
    mesh = collada.Collada(orig_zip, zip_filename=base_name)
    orig_render_info = getRenderInfo(mesh)

    if not os.path.isfile(orig_ss):
        p = Process(target=save_screenshot, args=(orig_zip, orig_ss), kwargs={'zip_filename':base_name})
        p.start()
        p.join()
    
    if not os.path.isfile(opt_zip):
        optimizations.apply(mesh)
        save.apply(mesh, opt_zip)
        mesh = None
    
    optmesh = collada.Collada(opt_zip)
    opt_render_info = getRenderInfo(optmesh)
    optmesh = None
    
    if not os.path.isfile(opt_ss):
        p = Process(target=save_screenshot, args=(opt_zip, opt_ss))
        p.start()
        p.join()
        
    orig_ss_copy = f_dir + '.orig.png'
    opt_ss_copy = f_dir + '.opt.png'
    if not os.path.isfile(orig_ss_copy):
        shutil.copy2(orig_ss, orig_ss_copy)
    if not os.path.isfile(opt_ss_copy):
        shutil.copy2(opt_ss, opt_ss_copy)
        
    orig_size = os.stat(orig_zip)[6]
    opt_size = os.stat(opt_zip)[6]
    return (f_dir, orig_size, opt_size, orig_render_info, opt_render_info)

def queue_worker():
    while True:
        args = queue.get()
        try:
            process_file(*args)
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            traceback.print_exc()
        queue.task_done()

def main():
    if len(sys.argv) != 2 or not os.path.isdir(sys.argv[1]):
        print 'usage: python meshtool_regression.py directory'
        sys.exit(1)
    base_dir = sys.argv[1]
    
    diffs = []
    cdn_list = list.grab_list(SERVER, 1000, '', '', 'dump')
    
    pool = Pool(processes=NUM_PROCS)
    
    results = []
    for f in cdn_list:
        results.append(pool.apply_async(process_file, args=(base_dir, f)))

    for r in results:
        r.wait()

    print '\t'.join(['id',
                     'orig_size',
                     'orig_num_triangles',
                     'orig_num_draw_raw',
                     'orig_num_draw_with_instances',
                     'orig_num_draw_with_batching',
                     'orig_texture_ram',
                     'opt_size',
                     'opt_num_triangles',
                     'opt_num_draw_raw',
                     'opt_num_draw_with_instances',
                     'opt_num_draw_with_batching',
                     'opt_texture_ram'])
    for r in results:
        unique_id, orig_size, opt_size, orig_render_info, opt_render_info = r.get()
        print '\t'.join(map(str,[
                          unique_id,
                          orig_size,
                          orig_render_info['num_triangles'],
                          orig_render_info['num_draw_raw'],
                          orig_render_info['num_draw_with_instances'],
                          orig_render_info['num_draw_with_batching'],
                          orig_render_info['texture_ram'],
                          opt_size,
                          opt_render_info['num_triangles'],
                          opt_render_info['num_draw_raw'],
                          opt_render_info['num_draw_with_instances'],
                          opt_render_info['num_draw_with_batching'],
                          opt_render_info['texture_ram']
                          ]))
            
if __name__ == "__main__":
    main()
