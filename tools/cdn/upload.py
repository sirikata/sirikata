#!/usr/bin/python

""" upload.py """
"""  Sirikata
 *  upload.py
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

import os, sys

print "upload.py -- upload content to sirikata website"
print "this software is released under the BSD open source license.  See sirikata.com for more details"

WINDOWS = False             #cygwin is not windows!
SCP = "scp"

## shedskin windows works in console, not cygwin!

if "win32" in sys.platform or "shedskin" in sys.platform:
    WINDOWS = True
    SCP = "pscp -i id_rsa_private.ppk"

nulldev = "NUL" if WINDOWS else "/dev/null"

def fixsysline(s):
    if WINDOWS:
        s=s.replace("/", "\\")
    return s

def system(s):
    ##print "cmd-->" + fixsysline(s) + "<--"
    return os.system(fixsysline(s))

def checkhttpfile(path):
    ## return file length or zero for not on server
    cmd = "wget -S --spider -o " + nulldev + " " + path
    ##print "os.cmd:", cmd
    err = os.system(cmd)
    if err:
        return 0
    return 1

hashes = set()
def main():
    print "creating or clearing temporary directory"
    if "tempSirikataUpload" in os.listdir("."):
        if not os.listdir("tempSirikataUpload") == []:
            if WINDOWS:
                os.system("del /Q tempSirikataUpload\*")
            else:
                os.system("rm tempSirikataUpload/*")
    else:
        system("mkdir tempSirikataUpload")
    print "creating name files:"
    f = open("Staging/names.txt")
    for i in f.readlines():
        name, hash = i.strip().split()
        print name
        clean = hash[-64:]
        hashes.add(clean)
        fo = open("tempSirikataUpload/"+name, "w")
        fo.write(hash)
        fo.close()
    f.close()

    print "copying name files:"
    cmd = SCP + fixsysline(" tempSirikataUpload/*") + " henrikbennetsen@delorean.dreamhost.com:sirikata.com/content/names/."
    ##print "os.cmd:", cmd
    os.system(cmd)
    print "done"
    print "----------------------------------------------------------"
    print "copying asset files:"
    assetlist = os.listdir("Cache")
    assetlist2 = os.listdir("Staging")
    for i in assetlist+assetlist2:
        if i in hashes:
            if checkhttpfile("http://www.sirikata.com/content/assets/" + i):
                print i, "found on server, not copying"
            else:
                cachedir = "Staging/" if (i in assetlist2) else "Cache/"
                cmd = fixsysline(SCP+" " + cachedir + i) + " henrikbennetsen@delorean.dreamhost.com:sirikata.com/content/assets/."
                ##print "os.cmd:", cmd
                os.system(cmd)

    print "done"

main()
