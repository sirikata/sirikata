#!/usr/bin/python
#
# prepare_collada.py username@cdn.com:/path/to/cdn/prep/dir collada_dir output_dir
#
# Prepares a collada file for upload to the CDN. Tries to validate
# that all referenced resources (currently just images) are available,
# renames resources appropriately, replaces filenames with URLs in the
# COLLADA file, and uploads the content to the CDN.  The content
# prepared for upload is copied or saved to the specified output
# directory.
#
# Note that this assumes a somewhat standard layout for collada
# directories, with models/ and images/ directories.

import sys
import os
import os.path
import glob
import xml.dom.minidom
import subprocess
import zipfile
import tempfile
import shutil

if sys.argv < 4:
    print "Usage: prepare_collada.py username@cdn.com:/path/to/cdn/prep/dir collada_zip_or_dir [output_dir]"
    sys.exit(-1)

remote_path, collada_dir = sys.argv[1:3]
if len(sys.argv) > 4:
    output_dir = sys.argv[4]
    cleanup_output = False
else:
    output_dir = tempfile.mkdtemp()
    cleanup_output = True

# Get just username+server and remote directory individually
username_at_server, remote_dir = remote_path.split(':')
# Get username and server individually
username, server = username_at_server.split('@')
print username, server
# Just do some sanity clean up on directories to make sure we have the
# directory itself (e.g. models/tower rather models/tower/) since
# os.path will return empty basenames for trailing slashes.
def remove_empty_filename(path):
    if (len(os.path.basename(path)) == 0):
        path = os.path.dirname(path)
    return path
collada_dir = remove_empty_filename(collada_dir)
output_dir = remove_empty_filename(output_dir)


# Make sure we have the output directory
if not os.path.exists(output_dir):
    os.makedirs(output_dir)


# Define classes to abstract away the particular source of the data -
# could be a zip file or an actual directory on disk
class ColladaDirectory:
    def __init__(self, cdir):
        self.cdir = cdir

    def find_daes(self):
        results = []
        for root, dirs, files in os.walk(collada_dir):
            results.extend( [os.path.join(root, name) for name in files] )
        results = [x for x in results if x.endswith('.dae')]
        return results

    def get_file(self, fname):
        return open(fname, 'r')

    def copy_file(self, fname, dest):
        shutil.copy(fname, dest)

class ColladaZip:
    def __init__(self, zf):
        self.zf = zipfile.ZipFile(zf)

    def find_daes(self):
        return [x for x in self.zf.namelist() if x.endswith('.dae')]

    def get_file(self, fname):
        return self.zf.open(fname)

    def copy_file(self, fname, dest):
        data = self.zf.read(os.path.normpath(fname))
        with file(dest, 'w') as f:
            f.write(data)

# Find the collada file
if os.path.isdir(collada_dir):
    collada = ColladaDirectory(collada_dir)
else:
    collada = ColladaZip(collada_dir)

daes = collada.find_daes()
if len(daes) == 0:
    print "Couldn't find any .dae files."
    exit(-1)
if len(daes) > 1:
    print "Too man daes"
    exit(-1)
(dae,) = daes
# Get the actual directory for the collada file
dae_dir = os.path.dirname(dae)

# Parse, looking for image files
dae_xml = xml.dom.minidom.parse( collada.get_file(dae) )

# Get the name we'll use as a base. For this we use the name of the
# output directory specified
def get_title(instr):
    split_instr = os.path.basename(instr).rsplit('.', 1);
    if len(split_instr) == 1:
        return split_instr[0]
    if len(split_instr[1]) < 4: # probably an extension
        return split_instr[0]
    # If nothing else works, use the whole thing
    return instr

if cleanup_output: # If its a random name, use the input path
    dae_name = get_title(collada_dir)
else:
    dae_name = get_title(output_dir)

def filter_images(node):
    if node.nodeType == node.ELEMENT_NODE and node.tagName == 'image':
        for child in node.getElementsByTagName('init_from'):
            image_node = child.childNodes[0]
            image_fname = image_node.data
            image_fname_base = os.path.basename(image_fname)
            renamed_image_base = dae_name + '__' + image_fname_base
            # Overwrite name
            local_url = './' + renamed_image_base
            image_node.data = local_url
            # Copy data
            local_file = os.path.join(dae_dir, image_fname)
            local_copy = os.path.join(output_dir, renamed_image_base)
            collada.copy_file(local_file, local_copy)

    for child in node.childNodes:
        filter_images(child)

# Perform actual filtering on xml data
filter_images(dae_xml)

# Store the modified .dae file
dae_xml.writexml( open(os.path.join(output_dir, dae_name + '.dae'), 'w') )



# Part 2 - Pushing to the CDN.  We've got all the files in the output
# directory, so now we just need to push them all up.
cdn_files = glob.glob( os.path.join(output_dir, '*') )
cdn_files_base = [os.path.basename(x) for x in cdn_files]

scp_cmd = ['scp']
scp_cmd.extend(cdn_files)
scp_cmd.append(username_at_server + ':' + remote_dir)
subprocess.call(scp_cmd);

add_cmd = ['ssh', username_at_server]
add_cmd_cmds = ['./add_to_cdn %s %s' % (username, x) for x in cdn_files_base]
add_cmd_cmds.insert(0, 'cd ' + remote_dir)
add_cmd.append('; '.join(add_cmd_cmds))
subprocess.call(add_cmd);

# And clean up
if cleanup_output:
    shutil.rmtree(output_dir)
