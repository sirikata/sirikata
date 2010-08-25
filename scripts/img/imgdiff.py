#!/usr/bin/python

# imgdiff.py - Computes error values for a pair of images or a set of
# images against a reference image.

import Image
import sys


def filter_name(orig):
    filtered = ''
    for x in orig:
        if x.isdigit():
            filtered = filtered + x
    return filtered

args = sys.argv

opt_filter_name = False
if '--filter-name' in args:
    args = [x for x in args if x != '--filter-name']
    opt_filter_name = True

im_name_ref = args[-1]
im_name_others = args[1:-1]

im_ref = Image.open(im_name_ref)

for im_name in im_name_others:
    im_other = Image.open(im_name)

    assert( im_ref.size[0] == im_other.size[0] and im_ref.size[1] == im_other.size[1] )

    num_wrong = 0
    ref_data = im_ref.getdata()
    other_data = im_other.getdata()
    for i in range(0, im_ref.size[0]*im_ref.size[1]):
        if ref_data[i] != other_data[i]:
            num_wrong += 1

    if opt_filter_name:
        print filter_name(im_name), float(num_wrong) / float(im_ref.size[0]*im_ref.size[1])
    else:
        print im_name, num_wrong, float(num_wrong) / float(im_ref.size[0]*im_ref.size[1])
