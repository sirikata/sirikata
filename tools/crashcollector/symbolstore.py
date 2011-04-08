#!/usr/bin/python

import sys, os, os.path, subprocess

def run_file_command(f):
    return subprocess.Popen(['file', '-Lb', f], stdout=subprocess.PIPE).communicate()[0]

def should_process(f):
    return (f.endswith('.so') or  os.access(f, os.X_OK)) and run_file_command(f).startswith("ELF")

def process_file(dump_syms_bin, f):
    bin_name = os.path.basename(f)
    syms_dump = subprocess.Popen([dump_syms_bin, f], stdout=subprocess.PIPE).communicate()[0]
    bin_hash = syms_dump.split()[3] #MODULE Linux x86_64 HASH

    bin_dir = os.path.join('symbols', bin_name, bin_hash)
    if not os.path.exists(bin_dir): os.makedirs(bin_dir)

    syms_filename = os.path.join(bin_dir, bin_name + '.sym')
    syms_file = open(syms_filename, 'wb')
    syms_file.write(syms_dump)
    syms_file.close()

def generate_syms(dump_syms_bin, path = None):
    """Scans through files, generating a symbols directory with breakpad symbol files for them."""

    path = path or os.getcwd()

    generate_for = [path_file for path_file in os.listdir(path) if should_process(path_file)]

    for bin_idx in range(len(generate_for)):
        bin = generate_for[bin_idx]
        process_file(dump_syms_bin, bin)
        print bin_idx+1, '/', len(generate_for)

if __name__ == '__main__':
    # Default offset for symbol dumper is just the offset from
    # build/cmake to the dump_syms binary. Without specifying the
    # second parameter, it will use cwd, so run from with build/cmake.
    generate_syms('../../dependencies/installed-breakpad/bin/dump_syms')
