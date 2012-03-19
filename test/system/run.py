#!/usr/bin/env python

import sys
from framework.tee import Tee
from framework.manager import Manager
import tests

if __name__ == "__main__":

    saveOutput = False
    output = sys.stdout

    testsToRun = []
    for s in range (1, len(sys.argv)):
        if (sys.argv[s] == '--saveOutput'):
            saveOutput=True
        elif sys.argv[s].startswith('--log='):
            output = Tee( open( sys.argv[s].split('=', 1)[1], 'w'), sys.stdout )
        else:
            testsToRun.append(sys.argv[s])

    # Search everywhere in this module for tests
    manager = Manager()
    manager.collect( sys.modules[__name__] )
    if len(testsToRun):
        manager.run(testsToRun, output=output, saveOutput=saveOutput)
    else:
        manager.run(output=output, saveOutput=saveOutput)
