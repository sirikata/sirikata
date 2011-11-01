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
import gzip
import matplotlib.pyplot as plt
import json
import base64
import numpy

SERVER = 'http://open3dhub.com'
DOWNLOAD = SERVER + '/download'
DNS = SERVER + '/dns'

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

def get_gzip_size(base_dir, hash):
    file_path = os.path.join(base_dir, hash)
    if not os.path.isfile(file_path):
        f_data = urllib2.urlopen(DOWNLOAD + '/' + hash).read()
        f = gzip.open(file_path, 'wb')
        f.write(f_data)
        f.close()
    
    return os.path.getsize(file_path)

def main():
    if len(sys.argv) != 2 or not os.path.isdir(sys.argv[1]):
        print 'usage: python meshtool_regression.py directory'
        sys.exit(1)
    base_dir = sys.argv[1]
    
    diffs = []
    cdn_list = list.grab_list(SERVER, 2000, '', '', 'dump')
    
    size_pairs = []
    draw_calls_original = []
    
    for f in cdn_list:
        types = f['metadata']['types']
        if 'original' not in types or 'progressive' not in types:
            continue
        
        gzip_orig_size = get_gzip_size(base_dir, types['original']['hash'])
        gzip_progressive_size = get_gzip_size(base_dir, types['progressive']['hash'])

        for subfile in types['original']['subfiles']:
            mesh_path = f['base_path']
            subfile_split = subfile.split('/')
            subfile_name = subfile_split[-2]
            version_num = f['version_num']
            subfile_url = '%s%s/%s/%s/%s' % (DNS, mesh_path, 'original', version_num, subfile_name)
            hash_cache_name = base64.urlsafe_b64encode(subfile_url)
            hash_cache_file = os.path.join(base_dir, hash_cache_name)
            if not os.path.isfile(hash_cache_file):
                try:
                    subfile_data = urllib2.urlopen(subfile_url).read()
                except urllib2.HTTPError:
                    continue
                sf = open(hash_cache_file, 'w')
                sf.write(subfile_data)
            else:
                sf = open(hash_cache_file, 'r')
                subfile_data = sf.read()
            sf.close()
            subfile_info = json.loads(subfile_data)
            subfile_hash = subfile_info['Hash']
            subfile_size = get_gzip_size(base_dir, subfile_hash)
            gzip_orig_size += subfile_size
        
        mipmap_levels = next(types['progressive']['mipmaps'].itervalues())['byte_ranges']
        progressive_mipmap_size = 0
        for mipmap in mipmap_levels:
            progressive_mipmap_size = mipmap['length']
            if mipmap['width'] >= 128 or mipmap['height'] >= 128:
                break
        gzip_progressive_size += progressive_mipmap_size
        
        size_pairs.append((gzip_orig_size, gzip_progressive_size))
        
        draw_calls_original.append(types['original']['metadata']['num_draw_calls'])
            
    reductions = [(p[1] - p[0]) / 1024.0 for p in size_pairs]
    reductions = sorted(reductions)

    x_vals = range(len(reductions))

    fig = plt.figure()
    ax = fig.add_subplot(1,1,1)
    ax.set_yscale('symlog')
    ax.scatter(x_vals,reductions,color='b', marker='+', s=1)
    plt.ylim(min(reductions),max(reductions))
    plt.xlim(0, len(reductions))
    ax.set_title('Change in Download Size for First Display')
    ax.set_xlabel('Mesh Number')
    ax.set_ylabel('Change in KiB')
    plt.savefig('mesh_size_change.pdf', format='pdf')

    hist, bins = numpy.histogram(draw_calls_original, bins=[1,2,3,4,5,6,7,8,9,10,15,20,25,100,max(draw_calls_original)])

    bin_labels = [1,2,3,4,5,6,7,8,9,10,'11-15','16-20','21-25','26+']

    fig = plt.figure()
    ax = fig.add_subplot(1,1,1)
    #ax.set_xscale('symlog')
    #ax.hist(draw_calls_original, bins=[0,1,2,3,4,5,10,15,20,25,100,1500])
    ax.bar(range(1,len(hist)+1), hist, align='center')
    #plt.ylim(0,max(draw_calls_original))
    plt.xlim(0,len(hist)+1)
    plt.xticks(range(1,len(hist)+1), bin_labels, rotation='vertical')
    ax.set_title('Original Number of Draw Calls to Display Mesh')
    ax.set_xlabel('Number of Draw Calls')
    ax.set_ylabel('Frequency')
    plt.gcf().subplots_adjust(bottom=0.15)
    plt.savefig('num_draw_calls_hist.pdf', format='pdf')

if __name__ == "__main__":
    main()
