#!/opt/local/bin/python2.5
#!/usr/bin/python

# imgdiff.py - Computes error values for a pair of images or a set of
# images against a reference image.

import Image
import sys

im_name_ref = sys.argv[-1]
im_name_others = sys.argv[1:-1]

im_ref = Image.open(im_name_ref)

for im_name in im_name_others:
    im_other = Image.open(im_name)

    assert( im_ref.size[0] == im_other.size[0] and im_ref.size[1] == im_other.size[1] )

    num_wrong = len([1 for a,b in zip(im_ref.getdata(),im_other.getdata()) if a != b])
    print im_name, num_wrong, float(num_wrong) / float(im_ref.size[0]*im_ref.size[1])
