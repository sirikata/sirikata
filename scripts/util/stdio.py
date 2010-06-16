#/usr/bin/python
#
# stdio.py - Defines a simple class which provides 3 file-like objects to
#            act as stdin, stdout, and stderr.  This allows redirecting
#            input and output.  If one is not provided, these will just
#            redirect to their normal variants.

import sys
import StringIO

class StdIO:
    def __init__(self, stdin=sys.stdin, stdout=sys.stdout, stderr=sys.stderr):
        """Construct a StdIO set.

        Keyword arguments:
        stdin -- file-like object to use as stdin
        stdout -- file-like object to use as stdout
        stderr -- file-like object to use as stderr

        """
        self.stdin = stdin
        self.stdout = stdout
        self.stderr = stderr

def MemoryIO():
    return StdIO(stdin=None, stdout=StringIO.StringIO(), stderr=StringIO.StringIO())
