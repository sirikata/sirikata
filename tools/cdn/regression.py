#!/usr/bin/env python
#
# regression.py - Checks for visual regressions in content loaded from
# the CDN by rendering before and after versions and comparing them.
#
# regression.py [-n num_assets] [-t type] url command
#
# regression.py is a utility for finding visual regressions in
# meshes. It can be used to check for regressions when making bug
# fixes to the model loading code, model rendering code, or different
# processed versions of models from the CDN (e.g. original
# vs. optimized).
#
# The basic approach is to grab a listing of content from the CDN (you
# pick how many models to evaluate) and two images of each. This is
# split into two phases so you don't have to be able to generate both
# versions at the same time (e.g. if you are updating the rendering
# code) and so you can create baseline images once and continue to
# compare against them.
#
# The command always takes two basic arguments, the url of the CDN to
# work with and a command. There are 4 commands:
#
#   baseline: generate baseline images
#
#   new: generate images based on new code (or by specifying a
#        different -t type)
#
#   compare: generate a report comparing the baseline and new images
#
#   rebaseline: replace the current baseline images with the current
#        new images, accepting any differences as fixes rather than
#        errors
#
# There are also these additional options:
#
#  -n num_assets - The number of asset listings to grab from the
#     CDN. Defaults to 10.
#
#  -t type - The type of mesh to get.
#     original - the original uploaded mesh [default]
#     optimized - version optimized for streaming
#

from list import grab_list
import sys, os.path, subprocess, shutil
import Image, ImageChops, ImageStat

def usage():
    print 'regression.py [-n num_assets] [-t type] url command'
    exit(-1)

def name_from_url(url):
    name = url.replace('meerkat:///', '')
    idx = max(name.find('original'), name.find('optimized'))
    name = name[:idx-1]
    name = name.replace('/', '_').replace('.', '_')
    return name

def data_directory():
    return 'regression'

def filename_from_url(url, klass):
    return os.path.join(data_directory(), name_from_url(url) + '.' + klass + '.' + 'png')

def generate_images(items, klass):
    if not os.path.exists(data_directory()): os.makedirs(data_directory())

    for item in items:
        sshot = filename_from_url(item, klass)
        # FIXME requiring running from a specific directory sucks,
        # currently required to be in root directory
        # FIXME how to decide between debug/release meshview?
        subprocess.Popen(['build/cmake/meshview_d', '--mesh=' + item, '--screenshot=' + sshot]).communicate()

def compute_diffs(items):
    for item in items:
        sshot_baseline = filename_from_url(item, 'baseline')
        sshot_new = filename_from_url(item, 'new')
        sshot_diff = filename_from_url(item, 'diff')

        im_base = Image.open(sshot_baseline)
        im_new = Image.open(sshot_new)

        # Note that this comparison isn't great, its RGB based but something like LAB would be preferable.
        im_diff = ImageChops.difference(im_base, im_new)
        diff_stats = ImageStat.Stat(im_diff)
        print item, max(diff_stats.rms) # max of rgb RMSs
        if max([channel[1] for channel in diff_stats.extrema]) > 0: # max of RGBs maxes
            im_diff.save(sshot_diff)

def rebaseline(items):
    # This is simple, just copy over the new images to baseline
    for item in items:
        sshot_baseline = filename_from_url(item, 'baseline')
        sshot_new = filename_from_url(item, 'new')
        shutil.copy(sshot_new, sshot_baseline)

def main():
    if len(sys.argv) < 3:
        usage()

    url = sys.argv[-2]
    command = sys.argv[-1]
    args = sys.argv[:-2]

    if command not in ['baseline', 'new', 'compare', 'rebaseline']:
        usage()

    num = 10
    model_type = 'original'

    x = 1
    while x < len(args):
        if args[x] == '-n':
            x += 1
            num = int(args[x])
        elif args[x] == '-t':
            x += 1
            model_type = args[x]
        else:
            usage()
        x += 1

    items = grab_list(url, num, 'model', model_type, 'meerkat')

    if command in ['baseline', 'new']:
        generate_images(items, command)
    elif command == 'compare':
        compute_diffs(items)
    elif command == 'rebaseline':
        rebaseline(items)

if __name__ == "__main__":
    main()
