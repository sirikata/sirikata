import subprocess
import argparse
import time
import multiprocessing
import os
import os.path
import sys

def main():
    parser = argparse.ArgumentParser(description = 'Spawn multiple copies of an object host on this machine',
                                     formatter_class = argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--num-procs', type=int, default=multiprocessing.cpu_count(),
                        help='Number of processes to run. Default is the number of CPUs available.')
    parser.add_argument('cppoh_path', help='path to cppoh')
    parser.add_argument('arguments', nargs='*', help='arguments to pass to cppoh')
    args = parser.parse_args()
    
    if not os.path.exists(args.cppoh_path):
        print >> sys.stderr, 'Path given to cppoh does not exist'
        sys.exit(1)
    if not os.access(args.cppoh_path, os.X_OK):
        print >> sys.stderr, 'Path given to cppoh is not executable'
        sys.exit(1)
    
    workdir = os.path.dirname(args.cppoh_path)
    subprocs = []
    for i in range(args.num_procs):
        p = subprocess.Popen([args.cppoh_path] + args.arguments, cwd=workdir)
        subprocs.append(p)
    
    while True:
        numalive = 0
        numdead = 0
        for p in subprocs:
            retcode = p.poll()
            if retcode is None:
                numalive += 1
            else:
                numdead += 1
        print 'Processes Alive: %d, Processes Dead: %d' % (numalive, numdead)
        if numalive < 1:
            break
        time.sleep(1)
    
if __name__ == '__main__':
    main()
