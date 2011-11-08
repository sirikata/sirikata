import execnet
import sys
import os
import fnmatch

def process_item(channel):
    import time
    import random
    import StringIO
    import Image
    import numpy
    import itertools
    from colormath.color_objects import LabColor, RGBColor
    
    channel.send("ready")
    for x in channel:
        if x is None:
            break
        
        level, filedata1, filedata2 = x
        file1 = StringIO.StringIO(filedata1)
        file2 = StringIO.StringIO(filedata2)
        
        base_image = Image.open(file1)
        compared_image = Image.open(file2)
        
        delta_Es = [RGBColor(rgb_base[0], rgb_base[1], rgb_base[2]).convert_to('lab').delta_e(
                      RGBColor(rgb_cur[0], rgb_cur[1], rgb_cur[2]).convert_to('lab'))
                    for rgb_base, rgb_cur
                    in itertools.izip(base_image.getdata(), compared_image.getdata())
                    if not(len(rgb_base) > 3 and len(rgb_cur) > 3 and rgb_base[3] == 0 and rgb_cur[3] == 0)]
        
        channel.send((level, delta_Es))

def main():
    if len(sys.argv) != 2:
        print >> sys.stderr, 'Usage: python screenshot_analyze.py directory'
        sys.exit(1)
    
    subdirs = os.listdir(sys.argv[1])
    
    prog_levels = []
    for level in range(0,101,10):
        prog_levels.append([])
    
    for meshdir in subdirs:
        meshfiles = os.listdir(os.path.join(sys.argv[1], meshdir))
        
        orig_ss = None
        prog_ss_levels = []
        
        for meshfile in meshfiles:
            if fnmatch.fnmatch(meshfile, '*orig.zip.png'):
                orig_ss = os.path.join(sys.argv[1], meshdir, meshfile)
        for level in range(0,101,10):
            for meshfile in meshfiles:
                if fnmatch.fnmatch(meshfile, '*progressive.%d.png' % level):
                    prog_ss_levels.append(os.path.join(sys.argv[1], meshdir, meshfile))
              
        if orig_ss is None or len(prog_ss_levels) != 11:
            print 'SKIPPING', meshdir
    
        for i, prog_ss in enumerate(prog_ss_levels):
            prog_levels[i].append((orig_ss, prog_ss))
    
    tasks = []
    for i, level in enumerate(prog_levels):
        for orig_ss, prog_ss in level:
            tasks.append((i, open(orig_ss, 'rb').read(), open(prog_ss, 'rb').read()))
    
    group = execnet.Group()
    
    todo = range(26,45)
    
    for i in todo:
        for core in range(8):
            group.makegateway("ssh=sns%d.cs.princeton.edu//id=sns%d-%d" % (i,i,core))
    mch = group.remote_exec(process_item)
    
    results_files = []
    for i in range(0,101,10):
        results_files.append(open('results-%d.txt' % i, 'w'))
    
    q = mch.make_receive_queue(endmarker=-1)
    terminated = 0
    while True:
        if tasks and tasks != -1:
            print 'Tasks left', len(tasks), 'terminated', terminated
        channel, item = q.get()
        if item == -1:
            terminated += 1
            print "terminated %s" % channel.gateway.id
            if terminated == len(mch):
                print "got all results, terminating"
                break 
            continue
        if item != "ready":
            print "other side %s returned" % (channel.gateway.id)
            level, data = item
            for d in data:
                results_files[level].write("%f\n" % d)
            results_files[level].write("\n")
        if not tasks:
            print "no tasks remain, sending termination request to all"
            mch.send_each(None)
            tasks = -1
        if tasks and tasks != -1:
            task = tasks.pop()
            channel.send(task)
            print "sent task to %s" % (channel.gateway.id)
    
if __name__ == '__main__':
    main()