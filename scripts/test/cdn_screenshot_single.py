import sys
import os
from colormath.color_objects import LabColor, RGBColor
from PIL import Image

dir = sys.argv[1]
files = os.listdir(dir)
files.sort()

base_file = os.path.join(dir, files[-1])
compared_file = os.path.join(dir, files[int(sys.argv[2])])

base_image = Image.open(base_file)
compared_image = Image.open(compared_file)

print \
    sum( \
         RGBColor(rgb_base[0], rgb_base[1], rgb_base[2]) \
            .convert_to('lab') \
            .delta_e( \
                RGBColor(rgb_cur[0], rgb_cur[1], rgb_cur[2]) \
                .convert_to('lab') \
            ) \
         for (rgb_base, rgb_cur) \
         in zip(base_image.getdata(), compared_image.getdata())\
    )
    