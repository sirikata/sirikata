import list
import pprint
import sys
import os
import os.path
import urllib2
import shutil
from meshtool.filters import factory
import collada
import unicodedata

SERVER = 'http://open3dhub.com'
DOWNLOAD = SERVER + '/download'

def decode(str):
    return unicodedata.normalize('NFKD', str).encode('ascii','ignore')

def main():
    if len(sys.argv) != 2 or not os.path.isdir(sys.argv[1]):
        print 'usage: python meshtool_regression.py directory'
        sys.exit(1)
    base_dir = sys.argv[1]
    
    diffs = []
    cdn_list = list.grab_list(SERVER, 1000, '', '', 'dump')
    for f in cdn_list:
        base_name = decode(f['base_name'])
        base_path = decode(f['base_path'])
        zip_hash = decode(f['metadata']['types']['original']['zip'])
        
        f_dir = base_path.replace('/','_')
        f_dir = os.path.join(base_dir, f_dir)
        if not os.path.isdir(f_dir):
            print 'making dir', f_dir
            os.mkdir(f_dir)
        
        orig_zip = os.path.join(f_dir, base_name + '.orig.zip')
        if not os.path.isfile(orig_zip):
            print 'downloading original zip file'
            f_data = urllib2.urlopen(DOWNLOAD + '/' + zip_hash).read()
            orig_zip_file = open(orig_zip, 'wb')
            orig_zip_file.write(f_data)
            orig_zip_file.close()
            
        screenshot = factory.getInstance('save_screenshot')
        optimizations = factory.getInstance('full_optimizations')
        save = factory.getInstance('save_collada_zip')
        
        orig_ss = orig_zip + '.png'
        opt_zip = os.path.join(f_dir, base_name + '.opt.zip')
        opt_ss = opt_zip + '.png'
        
        if not os.path.isfile(orig_ss) or not os.path.isfile(opt_zip) or not os.path.isfile(opt_ss):
            
            mesh = collada.Collada(orig_zip, zip_filename=base_name)
            
            if not os.path.isfile(orig_ss):
                print 'generating original screenshot'
                screenshot.apply(mesh, orig_ss)
                
            
            if not os.path.isfile(opt_zip):
                print 'generating optimized zip'
                optimizations.apply(mesh)
                save.apply(mesh, opt_zip)
            
            if not os.path.isfile(opt_ss):
                print 'generating optimized screenshot'
                mesh = collada.Collada(opt_zip)
                screenshot.apply(mesh, opt_ss)
            
        orig_size = os.stat(orig_zip)[6]
        opt_size = os.stat(opt_zip)[6]
        diff = opt_size - orig_size
        diffs.append(diff)
        print "%s\t%d bytes different" % (base_path, diff)
        print
        
    diffs = sorted(diffs)
    print diffs
    print sum(diffs)
            
if __name__ == "__main__":
    main()
