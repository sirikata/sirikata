""" csv_converter -- utility for converting csv to sqlite """
"""  Sirikata utility scripts
 *  csv_converter.py
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""
import math, sys
noisy=False
try:
    try:
        import sqlite3
    except:
        from pysqlite2 import dbapi2 as sqlite3
    import os
    import csv
    import math
    import random
    import time
    import uuid
    from urllib import unquote_plus
    import shutil
except:
    print "Missing library: ", sys.exc_info()[0],", not generating scene.db"
    sys.exit(0);
basepath=''
if len(sys.argv):
    where1=sys.argv[0].rfind("/tools/oh/")
    where2=sys.argv[0].rfind("\\tools\\oh")
    if where1!=-1:
        if where2!=-1 and where2>where1:
            where1=where2
    else:
        where1=where2
    if where1!=-1:
        basepath=sys.argv[0][0:where1]+'/'
print basepath
sys.path.append(basepath+'liboh/scripts/ironpython')
sys.path.append(basepath+'liboh/scripts/ironpython/site-packages')

if __name__=='__main__':
    sqlfile = 'scene.db'
    if len(sys.argv) > 1:
        if len(sys.argv) == 2:
            csvfiles = [sys.argv[1]]
        else:
            csvfiles = []
            for i in sys.argv[1:-1]:
                csvfiles.append(i)
            sqlfile = sys.argv[-1]
    else:
        csvfiles = ['scene.csv']

    try:
        os.rename(sqlfile, sqlfile+'.bak')
    except OSError:
        pass

    csvfile = csvfiles[0]
    print "converting:", csvfile, "to:", sqlfile
    shutil.copy(csvfile, sqlfile)
