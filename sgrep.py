#!/usr/bin/python

import subprocess
import sys

#DIRECTORIES_SEARCH = ['analysis/', 'bench/','build/','cdn/','cppoh/','cseg/','demo/','doc/','externals/','libcore/','libproxyobject/','libspace/','pinto/','scripts/','simoh/','space/','tools/'];

#Excludes build and externals and scripts
DIRECTORIES_SEARCH = ['analysis/', 'bench/','cdn/','cppoh/','cseg/','demo/','doc/','libcore/','liboh','libproxyobject/','libspace/','pinto/','simoh/','space/','tools/'];


if __name__ == "__main__":

    if (len (sys.argv) != 2):
        print("\n\nIncorrect usage: add in what you're grepping for\n\n");

    
    cmd = ['grep','-R', sys.argv[1]];
    cmd += DIRECTORIES_SEARCH;
    
    subprocess.call(cmd);
    
