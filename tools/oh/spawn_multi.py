import subprocess
import argparse
import time
import multiprocessing
import os
import os.path
import sys
import datetime

def main():
    parser = argparse.ArgumentParser(description = 'Spawn multiple copies of an object host on this machine',
                                     formatter_class = argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--num-procs', type=int, default=multiprocessing.cpu_count(),
                        help='Number of processes to run. Default is the number of CPUs available.')
    parser.add_argument('--delay', type=int, default=0, help='Number of seconds to delay between spawning subprocesses')
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
    delay_timedelta = datetime.timedelta(seconds=args.delay)
    last_creation = datetime.datetime.now() - delay_timedelta
    while True:
        subprocs = [p for p in subprocs if p.poll() is None]
        print 'Processes Alive: %d out of %d' % (len(subprocs), args.num_procs)
        if len(subprocs) < args.num_procs and datetime.datetime.now() - last_creation > delay_timedelta:
            print 'Starting new subprocess'
            last_creation = datetime.datetime.now()
            p = subprocess.Popen([args.cppoh_path] + args.arguments, cwd=workdir)
            subprocs.append(p)
        time.sleep(1)
    
if __name__ == '__main__':
    main()
