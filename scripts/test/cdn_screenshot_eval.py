import sys
import os
from colormath.color_objects import LabColor, RGBColor
from PIL import Image

def rgb_to_lab(img_data):
    return [RGBColor(r,b,g).convert_to('lab') for (r,g,b) in img_data]

def compute_difference(lab1, lab2):
    return sum([l1.delta_e(l2, mode='cie1976') for (l1, l2) in zip(lab1, lab2)])

dir = sys.argv[1]
files = os.listdir(dir)
files.sort()

end_file = files[-1]
end_image = Image.open(os.path.join(dir, end_file))
end_lab = rgb_to_lab(end_image.getdata())

for f in files:
    tokens = f.split('_')
    index = tokens[0]
    byte_count = tokens[1]
    if(int(byte_count) > 0):
        cur_image = Image.open(os.path.join(dir, f))
        print index, byte_count, sum(RGBColor(rgbcur[0], rgbcur[1], rgbcur[2]).convert_to('lab').delta_e(labend) for (rgbcur, labend) in zip(cur_image.getdata(), end_lab))
    