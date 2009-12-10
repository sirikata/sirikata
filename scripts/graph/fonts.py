#!/usr/bin/python

# graphs/fonts.py - Utilities for setting fonts on graphs

import matplotlib.pyplot as plt

def set_legend_fontsize(legend, fontsize):
    for t in legend.get_texts():
        t.set_fontsize(fontsize)
        t.set_fontname('serif')
