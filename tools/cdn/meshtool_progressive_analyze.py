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
from matplotlib import rc, rcParams
from matplotlib.lines import Line2D
from matplotlib.font_manager import FontProperties
import json
import base64
import numpy

rc('text', usetex=True)
rc('font', family='serif')

SERVER = 'http://open3dhub.com'
DOWNLOAD = SERVER + '/download'
DNS = SERVER + '/dns'

#DATA_OUT = """71076005 805416474.359 11.3317634321 8.19903 27.240254 1.018211
#68951003 674983010.068 9.78931387072 6.577926 24.032948 0.841197
#68889178 602343056.445 8.74365283391 5.031103 23.3255493 0.695605
#68849516 553289057.267 8.03620837751 4.1197845 22.475401 0.615508
#68827972 527585146.555 7.66527229009 3.575544 21.6788579 0.567449
#68812736 515550764.675 7.49208351017 3.3456605 21.284081 0.491539
#68799250 513001945.94 7.45650491743 3.313815 21.284081 0.481598
#68782764 510430178.859 7.420902406 3.2873305 21.2206367 0.475875
#68764632 508582444.54 7.39598874811 3.269563 21.0836329 0.472489
#68735994 506220674.383 7.36471017474 3.2495345 21.058182 0.468956
#68696465 500867618.747 7.29102463638 3.201587 20.8095526 0.459344"""

DATA_OUT = """78467628 898050454.086 11.4448528263 6.4171475 29.961459 0.0
76386666 704819299.397 9.22699387609 4.4762075 25.274798 0.0
76336783 596506038.71 7.81413645254 3.218848 22.7660624 0.0
76298845 520640735.77 6.82370402554 2.369781 20.417026 0.0
76278624 477993045.209 6.2664088593 1.974794 18.943688 0.0
76264341 457567769.496 5.99976035322 1.810844 18.221164 0.0
76251447 451546784.839 5.92181266853 1.810844 17.96473 0.0
76235306 447831899.655 5.87433727432 1.810844 17.7626415 0.0
76217359 445059006.946 5.839339132 1.810844 17.606025 0.0
76188961 442180199.453 5.80373053589 1.810844 17.471591 0.0
76149991 434765830.794 5.70933528795 1.797515 17.134251 0.0"""

#DATA_OUT = """70284092 689604708.536 9.81167557142 5.213484 26.077159 0.630733
#68144821 562464543.412 8.25395877717 3.719422 22.770836 0.487914
#68072036 500691444.857 7.35531760586 2.838925 21.171333 0.400694
#68032818 463120050.852 6.80730365825 2.301201 20.064436 0.312665
#68009114 443307531.219 6.51835474903 2.0310935 19.541012 0.27483
#67991523 436375269.535 6.41808346513 1.92669 19.4096412 0.27483
#67986204 433214083.378 6.37208812803 1.9088815 19.224709 0.274139
#67973623 429874390.479 6.32413532643 1.900266 19.067663 0.272124
#67945586 426371368.669 6.27518862916 1.888988 18.825753 0.272124
#67909171 423025343.437 6.22928151246 1.877646 18.591086 0.272124
#67855016 414884585.299 6.11428026631 1.84367 18.07165 0.272124"""

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
    num_triangles = []
    num_images = []
    ram_usage = []
    
    num_to_check = len([f for f in cdn_list if 'original' in f['metadata']['types'] and 'progressive' in f['metadata']['types']])
    size_table = numpy.zeros((num_to_check, 5, 5), dtype=numpy.float32)

    table_index = 0
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
        
        gzip_progressive_stream_size = get_gzip_size(base_dir, types['progressive']['progressive_stream']) if types['progressive']['progressive_stream'] else 0
        
        mipmap_levels = next(types['progressive']['mipmaps'].itervalues())['byte_ranges']
        for stream_percentage in range(5):
            for mipmap_level in range(5):
                current_stream_size = gzip_progressive_stream_size * (stream_percentage / 4.0)
            
                current_mipmap_size = [128,256,512,1024,2048][mipmap_level]
                all_mipmaps_size = 0
                for mipmap in mipmap_levels:
                    all_mipmaps_size += mipmap['length']
                    if mipmap['width'] >= current_mipmap_size or mipmap['height'] >= current_mipmap_size:
                        break
                
                entrysize = gzip_progressive_size + current_stream_size + all_mipmaps_size
                size_table[table_index, stream_percentage, mipmap_level] = entrysize / gzip_orig_size
        
        progressive_mipmap_size = 0
        for mipmap in mipmap_levels:
            progressive_mipmap_size = mipmap['length']
            if mipmap['width'] >= 128 or mipmap['height'] >= 128:
                break
        gzip_progressive_size += progressive_mipmap_size
        
        size_pairs.append((gzip_orig_size, gzip_progressive_size))
        
        draw_calls_original.append(types['original']['metadata']['num_draw_calls'])
        num_triangles.append(types['original']['metadata']['num_triangles'])
        num_images.append(types['original']['metadata']['num_images'])
        ram_usage.append(types['original']['metadata']['texture_ram_usage'])
    
        table_index += 1


    #print numpy.mean(size_table, axis=0)
    #print numpy.min(size_table, axis=0)
    #print numpy.max(size_table, axis=0)
    size_table = numpy.median(size_table, axis=0)
    print 'Relative Median Size (rows=0,25,50,75,100, cols=128,256,512,1024,2048)'
    print size_table
    
    
    reductions = [(p[1] - p[0]) / 1024.0 for p in size_pairs]
    reductions = sorted(reductions)
    x_vals = numpy.arange(0, 1, 1.0 / len(reductions))
    fig = plt.figure(figsize=(4,1.5))
    ax = fig.add_subplot(1,1,1)
    ax.set_yscale('symlog')
    ax.scatter(x_vals,reductions,color='b', marker='+', s=1)
    plt.ylim(min(reductions),max(reductions))
    plt.xlim(0, 1)
    #ax.set_title('Change in Download Size for First Display')
    ax.set_xlabel('Mesh')
    ax.set_ylabel('Change in KB')
    plt.gcf().subplots_adjust(bottom=0.26, left=0.18, right=0.95, top=0.98)
    plt.savefig('mesh_size_change.pdf', format='pdf', dpi=150)



    hist, bins = numpy.histogram(draw_calls_original, bins=[1,2,3,4,5,6,7,8,9,10,15,20,25,100,max(draw_calls_original)])
    bin_labels = [1,2,3,4,5,6,7,8,9,10,'11-15','16-20','21-25','26+']
    fig = plt.figure(figsize=(4,2))
    ax = fig.add_subplot(1,1,1)
    #ax.set_xscale('symlog')
    #ax.hist(draw_calls_original, bins=[0,1,2,3,4,5,10,15,20,25,100,1500])
    hist = hist / float(numpy.sum(hist))
    ax.bar(range(1,len(hist)+1), hist, align='center')
    #plt.ylim(0,max(draw_calls_original))
    plt.xlim(0,len(hist)+1)
    plt.xticks(range(1,len(hist)+1), bin_labels, rotation='vertical')
    plt.yticks([0,0.05,0.10,0.15,0.20], ['0','.05','.10','.15','.20'])
    #ax.set_title('Original Number of Draw Calls to Display Mesh')
    ax.set_xlabel('Number of Draw Calls')
    ax.set_ylabel('Frequency')
    plt.gcf().subplots_adjust(bottom=0.35, left=0.15, right=0.95, top=0.96)
    plt.savefig('num_draw_calls_hist.pdf', format='pdf', dpi=150)


    hist, bins = numpy.histogram(num_triangles, bins=[0,1000,10000,100000,max(num_triangles)])
    bin_labels = ['\\textless1K','\\textless10K','\\textless100K','100K+']
    fig = plt.figure(figsize=(4,2))
    ax = fig.add_subplot(1,1,1)
    #ax.set_xscale('symlog')
    #ax.hist(draw_calls_original, bins=[0,1,2,3,4,5,10,15,20,25,100,1500])
    hist = hist / float(numpy.sum(hist))
    ax.bar(range(1,len(hist)+1), hist, align='center')
    #plt.ylim(0,max(draw_calls_original))
    plt.xlim(0.4,len(hist)+0.6)
    plt.xticks(range(1,len(hist)+1), bin_labels) #, rotation=17)
    plt.yticks([0,0.05,0.10,0.15,0.20,0.25,0.3,0.35,0.4], ['0','.05','.10','.15','.20','.25','.30','.35','.40'])
    #ax.set_title('Original Number of Draw Calls to Display Mesh')
    ax.set_xlabel('Number of Triangles')
    ax.set_ylabel('Frequency')
    plt.gcf().subplots_adjust(bottom=0.21, left=0.15, right=0.95, top=0.96)
    plt.savefig('num_triangles_hist.pdf', format='pdf', dpi=150)



    
    hist, bins = numpy.histogram(ram_usage, bins=[0,1,100*1024,1024*1024,10*1024*1024,max(ram_usage)])
    bin_labels = ['0','\\textless100KB','\\textless1MB','\\textless10MB','10MB+']
    fig = plt.figure(figsize=(4,2))
    ax = fig.add_subplot(1,1,1)
    #ax.set_xscale('symlog')
    #ax.hist(draw_calls_original, bins=[0,1,2,3,4,5,10,15,20,25,100,1500])
    hist = hist / float(numpy.sum(hist))
    ax.bar(range(1,len(hist)+1), hist, align='center')
    #plt.ylim(0,max(draw_calls_original))
    plt.xlim(0.4,len(hist)+0.6)
    plt.xticks(range(1,len(hist)+1), bin_labels, rotation=17)
    plt.yticks([0,0.05,0.10,0.15,0.20,0.25,0.3,0.35], ['0','.05','.10','.15','.20','.25','.30','.35'])
    #ax.set_title('Original Number of Draw Calls to Display Mesh')
    ax.set_xlabel('Texture RAM Usage')
    ax.set_ylabel('Frequency')
    plt.gcf().subplots_adjust(bottom=0.3, left=0.15, right=0.95, top=0.96)
    plt.savefig('texture_ram_hist.pdf', format='pdf', dpi=150)




    chart_error_dir = os.path.join(os.path.dirname(__file__), 'chart_errors')
    for errdir in os.listdir(chart_error_dir):

        if errdir != 'duck':
            continue

        errfile = os.path.join(chart_error_dir, errdir, 'err.txt')
        maxerrfile = os.path.join(chart_error_dir, errdir, 'maxerr.txt')

        chart_errors = numpy.fromfile(errfile, sep='\n')  
        chart_errors = numpy.sort(chart_errors)
        max_errors = numpy.fromfile(maxerrfile, sep='\n')  
        max_errors = numpy.sort(max_errors)
        heuristic = numpy.log(chart_errors+1) / numpy.log(max_errors+1)
        stop_point = numpy.where(heuristic >= 0.90)[0][0]
        
        x_vals = numpy.arange(0, len(chart_errors))
        fig = plt.figure(figsize=(4,3))
        ax = fig.add_subplot(1,1,1)
        ax.set_yscale('symlog')
        
        ax.scatter(x_vals, chart_errors, color='b', marker='+', s=1)
        ax.scatter(x_vals, max_errors, color='r', marker='x', s=1)
        #ax.set_title('Change in Download Size for First Display')
        ax.set_xlabel('Merge Step')
        ax.set_ylabel('Error Value')
        plt.ylim(min(chart_errors),max(chart_errors))
        
        axR = fig.add_subplot(1,1,1, sharex=ax, frameon=False)
        axR.yaxis.tick_right()
        axR.yaxis.set_label_position("right")
        axR.scatter(x_vals, heuristic, color='g', marker='s', s=1)
        axR.set_ylabel("Heuristic Value")
        
        plt.axvline(stop_point, linestyle='-', color='#333333')
        
        plt.xticks([0,1000,2000,3000,4000])
        plt.xlim(0, len(chart_errors))
        
        p1 = Line2D([0],[1], color="g", linewidth=2)
        p2 = Line2D([0],[1], color="r", linewidth=2)
        p3 = Line2D([0],[1], color="b", linewidth=2)
        prop = FontProperties(size=11)
        plt.legend((p1, p2, p3), ['Heuristic', 'Maximum', 'Current'], loc=0, prop=prop)
        plt.gcf().subplots_adjust(bottom=0.14, left=0.13, right=0.87, top=0.98)
        plt.savefig('chart_merge_error_%s.pdf' % errdir, format='pdf', dpi=150)




    
    percep_lines = DATA_OUT.split("\n")
    y_vals = []
    y_vals2 = []
    perc90_vals = []
    perc10_vals = []
    for line in percep_lines:
        ct, tot, avg, median, perc90, perc10 = map(float, line.split(" "))
        y_vals.append(avg)
        y_vals2.append(median)
        perc10_vals.append(perc10)
        perc90_vals.append(perc90)
    x_vals = range(0,101,10)
    
    fig = plt.figure(figsize=(4,2))
    ax = fig.add_subplot(1,1,1)
    #ax.set_yscale('symlog')
    ax.plot(x_vals, y_vals, color='b', marker='s', label='Mean')
    ax.plot(x_vals, y_vals2, color='r', marker='o', label='Median')
    plt.ylim(0, max(y_vals) * 1.05)
    plt.xlim(0, 100)
    plt.xticks(range(0, 101, 10))
    #ax.set_title('Perceptual Difference from Original Mesh')
    ax.set_xlabel('Progressive Stream %')
    ax.set_ylabel('Delta E (CIE2000)')
    ax.legend()
    plt.gcf().subplots_adjust(bottom=0.2, left=0.13, right=0.95, top=0.96)
    plt.savefig('progressive_perceptual_difference.pdf', format='pdf', dpi=150)

if __name__ == "__main__":
    main()
