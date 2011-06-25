#!/usr/bin/python

import sys, os, os.path, subprocess
import re

class Settings:
    def bin(self):
        return self._bin

class LinuxSettings(Settings):
    def __init__(self, dump_syms_dir):
        self._bin = os.path.join(dump_syms_dir, 'dump_syms')

    def offset(self, path):
        return path

    def run_file_command(self, f):
        return subprocess.Popen(['file', '-Lb', f], stdout=subprocess.PIPE).communicate()[0]

    def should_process(self, f):
        return (f.endswith('.so') or  os.access(f, os.X_OK)) and self.run_file_command(f).startswith("ELF")

    def canonicalize_bin_name(self, path):
        return path


class Win32Settings(Settings):
    def __init__(self, dump_syms_dir):
        self._bin = os.path.join(dump_syms_dir, 'dump_syms.exe')

    def should_process(self, f):
        sans_ext = os.path.splitext(f)[0]
        return f.endswith('.pdb') and (os.path.isfile(sans_ext + '.exe') or os.path.isfile(sans_ext + '.dll'))

    def offset(self, path):
        return os.path.join(path, 'RelWithDebInfo')

    def canonicalize_bin_name(self, path):
        return re.sub("\.pdb$", "", path)


def process_file(settings, f):
    dump_syms_bin = settings.bin()

    bin_name = os.path.basename(f)

    syms_dump = subprocess.Popen([dump_syms_bin, f], stdout=subprocess.PIPE).communicate()[0]
    (bin_hash,bin_file_name) = syms_dump.split()[3:5] #MODULE Linux x86_64 HASH binary
    canonical_bin_name = settings.canonicalize_bin_name(bin_file_name)

    bin_dir = os.path.join('symbols', bin_file_name, bin_hash)
    if not os.path.exists(bin_dir): os.makedirs(bin_dir)

    syms_filename = os.path.join(bin_dir, canonical_bin_name + '.sym')
    syms_file = open(syms_filename, 'wb')
    syms_file.write(syms_dump)
    syms_file.close()

def generate_syms(settings, path = None):
    """Scans through files, generating a symbols directory with breakpad symbol files for them."""

    path = path or os.getcwd()
    path = settings.offset(path)

    generate_for = [os.path.join(path, path_file) for path_file in os.listdir(path)]
    generate_for = [path_file for path_file in generate_for if settings.should_process(path_file)]

    for bin_idx in range(len(generate_for)):
        bin = generate_for[bin_idx]
        process_file(settings, bin)
        print bin_idx+1, '/', len(generate_for)

if __name__ == '__main__':
    # Default offset for symbol dumper is just the offset from
    # build/cmake to the dump_syms binary. Without specifying the
    # second parameter, it will use cwd, so run from with build/cmake.

    platform_settings = {
        'win32' : Win32Settings,
        'linux2' : LinuxSettings
        }
    generate_syms(
        platform_settings[sys.platform]('../../dependencies/installed-breakpad/bin')
        )
